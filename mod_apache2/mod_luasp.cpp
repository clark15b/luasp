
/*  
 * Copyright (C) Anton Burdinuk 
 */
  

/*
**
**  Activate it in Apache's 2.x apache2.conf file for instance
**    #   apache2.conf
**    LoadModule luasp_module modules/mod_luasp.so
**    AddHandler lsp .lsp
**    AddHandler lua .lua
**
**  Then after restarting Apache via
**
**    $ apachectl restart
**
*/ 

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "httpd.h" 
#include "http_config.h" 
#include "http_protocol.h" 
#include "http_log.h" 
#include "util_script.h" 
#include "apr_strings.h" 

#include "llsplib.h"
#include "luamysql.h"
#include "luacurl.h"
#include "luajson.h"
#include "luamcache.h"

#include <unistd.h>
#include <uuid/uuid.h>


#ifndef VERSION
#define VERSION ""
#endif /*VERSION*/

extern module AP_MODULE_DECLARE_DATA luasp_module;


namespace lsp
{
    // lsp handlers
    enum
    {
	handler_type_unknown=0,
	handler_type_lsp=1,
	handler_type_lua=2
    };
    

    // lua container
    struct LUABAG
    {
	lua_State* L;
    };

    
    // lsp global config structure
    struct lsp_srv_conf
    {
	char* init_script;
    };


    // lsp per directory config structure
    struct lsp_dir_conf
    {
	char* cookie_name;
	int cookie_days;
	char* cookie_path;
	int show_exception;
    };


    // apache config support
    
    void* config_dir_alloc(apr_pool_t* p,char*);
    void* config_dir_merge(apr_pool_t* p,void* c1,void* c2);
    
    void* config_srv_alloc(apr_pool_t* p,server_rec*);
    void* config_srv_merge(apr_pool_t* p,void* c1,void* c2);
    

    const char* lsp_cookie_name(cmd_parms* cmd,lsp_dir_conf* conf,char* val)
    {
	conf->cookie_name=apr_pstrdup(cmd->pool,val);
	return NULL;
    }

    const char* lsp_cookie_expires(cmd_parms* cmd,lsp_dir_conf* conf,char* val)
    {
	conf->cookie_days=atoi(val);
	return NULL;
    }

    const char* lsp_cookie_path(cmd_parms* cmd,lsp_dir_conf* conf,char* val)
    {
	conf->cookie_path=apr_pstrdup(cmd->pool,val);
	return NULL;
    }

    const char* lsp_show_exceptions(cmd_parms* cmd,lsp_dir_conf* conf,char* val)
    {
	conf->show_exception=atoi(val);
	return NULL;
    }

    const char* lsp_init_script(cmd_parms* cmd,void*,char* val)
    {
	lsp_srv_conf* conf=(lsp_srv_conf*)ap_get_module_config(cmd->server->module_config,&luasp_module);

	conf->init_script=apr_pstrdup(cmd->pool,val);

	return NULL;
    }
			    

    // apache config commands
    const command_rec cmds[]=
    {
        AP_INIT_TAKE1("LSPCookieName",(cmd_func)lsp::lsp_cookie_name,NULL,ACCESS_CONF,"change Cookie name for LSP session feature"),
        AP_INIT_TAKE1("LSPCookieExpires",(cmd_func)lsp::lsp_cookie_expires,NULL,ACCESS_CONF,"change Cookie expiration days LSP"),
        AP_INIT_TAKE1("LSPCookiePath",(cmd_func)lsp::lsp_cookie_path,NULL,ACCESS_CONF,"change Cookie path LSP"),
        AP_INIT_TAKE1("LSPShowExceptions",(cmd_func)lsp::lsp_show_exceptions,NULL,ACCESS_CONF,"show LSP run-time exceptions to client"),
        AP_INIT_TAKE1("LSPInitScript",(cmd_func)lsp::lsp_init_script,NULL,RSRC_CONF,"LSP init script"),
	{NULL}
    };

    // name of lua container type
    static const char LUABAG_TYPE[]="LSPBAG";

    // cleanup
    apr_status_t luabag_cleanup(void* p);

    // implementation of lsp host functions
    int lua_log(lua_State *L);
    int lua_content_type(lua_State *L);
    int lua_set_out_header(lua_State *L);
    int lua_get_in_header(lua_State *L);
    int lua_uuid_gen(lua_State *L);
    int lua_version(lua_State *L);

    // apache support functions
    int handler(request_rec *r);
    void register_hooks(apr_pool_t *p);


    // lsp support
    int read_request_data(lua_State *L);

    int ap_def_puts(void* ctx,const char* s) { return ap_rputs(s,(request_rec*)ctx); }
    int ap_def_putc(void* ctx,int c) { return ap_rputc(c,(request_rec*)ctx); }
    int ap_def_write(void* ctx,const char* s,size_t len) { return ap_rwrite(s,len,(request_rec*)ctx); }
}

void* lsp::config_srv_alloc(apr_pool_t* p,server_rec*)
{
    lsp_srv_conf* conf=(lsp_srv_conf*)apr_pcalloc(p,sizeof(lsp_srv_conf));

    return (void*)conf;
}
void* lsp::config_srv_merge(apr_pool_t* p,void* c1,void* c2)
{
    lsp_srv_conf* conf1=(lsp_srv_conf*)c1;
    lsp_srv_conf* conf2=(lsp_srv_conf*)c2;

    lsp_srv_conf* conf=(lsp_srv_conf*)apr_pcalloc(p,sizeof(lsp_srv_conf));
    
    conf->init_script=conf2->init_script?conf2->init_script:conf1->init_script;

    return (void*)conf;
}


void* lsp::config_dir_alloc(apr_pool_t* p,char*)
{
    lsp_dir_conf* conf=(lsp_dir_conf*)apr_pcalloc(p,sizeof(lsp_dir_conf));
    if(conf)
    {
	conf->cookie_name=0;
	conf->cookie_days=-1;
	conf->cookie_path=0;
	conf->show_exception=-1;
    }
    return (void*)conf;
}

void* lsp::config_dir_merge(apr_pool_t* p,void* c1,void* c2)
{
    lsp_dir_conf* conf1=(lsp_dir_conf*)c1;
    lsp_dir_conf* conf2=(lsp_dir_conf*)c2;

    lsp_dir_conf* conf=(lsp_dir_conf*)apr_pcalloc(p,sizeof(lsp_dir_conf));

    conf->cookie_name=conf2->cookie_name?conf2->cookie_name:conf1->cookie_name;
    conf->cookie_path=conf2->cookie_path?conf2->cookie_path:conf1->cookie_path;
    conf->cookie_days=conf2->cookie_days>=0?conf2->cookie_days:conf1->cookie_days;
    conf->show_exception=conf2->show_exception>=0?conf2->show_exception:conf1->show_exception;

    return (void*)conf;
}


void lsp::register_hooks(apr_pool_t *p)
{
    ap_hook_handler(lsp::handler,NULL,NULL,APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA luasp_module=
{
    STANDARD20_MODULE_STUFF, 
    lsp::config_dir_alloc, /* create per-dir    config structures */
    lsp::config_dir_merge, /* merge  per-dir    config structures */
    lsp::config_srv_alloc, /* create per-server config structures */
    lsp::config_srv_merge, /* merge  per-server config structures */
    lsp::cmds,             /* table of config file commands       */
    lsp::register_hooks	   /* register hooks                      */
};



apr_status_t lsp::luabag_cleanup(void* p)
{
    LUABAG* luabag=(LUABAG*)p;

    if(luabag && luabag->L)
    {
	luaclose_luamysql();
	luaclose_luacurl();
	luaclose_luajson();
	luaclose_luamcache();
	lua_close(luabag->L);
	luabag->L=0;
    }

    return APR_SUCCESS;
}


int lsp::lua_log(lua_State *L)
{
    request_rec* apr=(request_rec*)luaL_lsp_get_io_ctx(L);

    const char* s=luaL_checkstring(L,1);

    ap_log_rerror(APLOG_MARK,APLOG_NOTICE,0,apr,"%s",s);
    
    return 0;
}

int lsp::lua_content_type(lua_State *L)
{
    request_rec* apr=(request_rec*)luaL_lsp_get_io_ctx(L);

    const char* s=luaL_checkstring(L,1);

    apr->content_type=apr_pstrdup(apr->pool,s);
    
    return 0;
}

int lsp::lua_set_out_header(lua_State *L)
{
    request_rec* apr=(request_rec*)luaL_lsp_get_io_ctx(L);

    const char* s1=luaL_checkstring(L,1);
    const char* s2=0;
    
    if(lua_gettop(L)>1)
	s2=luaL_checkstring(L,2);

    if(!s2 || !*s2)
	apr_table_unset(apr->headers_out,s1);
    else
	apr_table_set(apr->headers_out,s1,s2);

    return 0;
}

int lsp::lua_get_in_header(lua_State *L)
{
    request_rec* apr=(request_rec*)luaL_lsp_get_io_ctx(L);

    const char* s1=luaL_checkstring(L,1);

    const char* s2=apr_table_get(apr->headers_in,s1);

    if(!s2)
	s2="";

    lua_pushstring(L,s2);
 
    return 1;
}


int lsp::lua_uuid_gen(lua_State *L)
{
    char tmp[64];

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid,tmp);
    
    lua_pushstring(L,tmp);
 
    return 1;
}

int lsp::lua_version(lua_State *L)
{
    lua_pushstring(L,VERSION);
 
    return 1;
}


int lsp::handler(request_rec *r)
{
    lsp_srv_conf* srv_conf=(lsp_srv_conf*)ap_get_module_config(r->server->module_config,&luasp_module);
    lsp_dir_conf* dir_conf=(lsp_dir_conf*)ap_get_module_config(r->per_dir_config,&luasp_module);

    int handler_type=handler_type_unknown;

    if(r->handler && srv_conf && dir_conf)
    {
	if(!strcmp(r->handler,"lsp")) handler_type=handler_type_lsp;
	else if(!strcmp(r->handler,"lua")) handler_type=handler_type_lua;
	else
	    return DECLINED;
    }else
	return DECLINED;

    if(!r->filename)
	return HTTP_INTERNAL_SERVER_ERROR;

    if(r->finfo.filetype==APR_NOFILE)
	return HTTP_NOT_FOUND;
    if(r->finfo.filetype!=APR_REG && r->finfo.filetype!=APR_LNK)
	return HTTP_FORBIDDEN;

    if (r->header_only)
	return OK;


    LUABAG* luabag=0;
    
    apr_pool_userdata_get((void**)&luabag,LUABAG_TYPE,r->connection->pool); 
    
    if(!luabag)
    {
	luabag=(LUABAG*)apr_pcalloc(r->connection->pool,sizeof(LUABAG));

	luabag->L=lua_open();
	
	luaL_openlibs(luabag->L);
	
	luaopen_lualsp(luabag->L);
	
	luaopen_luamysql(luabag->L);

	luaopen_luacurl(luabag->L);
	
	luaopen_luajson(luabag->L);
	
	luaopen_luamcache(luabag->L);
	
	lua_register(luabag->L,"module_version",lua_version);
	lua_register(luabag->L,"log",lua_log);
	lua_register(luabag->L,"uuid_gen",lua_uuid_gen);

	if(srv_conf->init_script)
	{
	    if(luaL_loadfile(luabag->L,srv_conf->init_script) || lua_pcall(luabag->L,0,0,0))
	    {
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"%s",lua_tostring(luabag->L,-1));
		lua_pop(luabag->L,1);
	    }	    
	}
	
	lua_register(luabag->L,"content_type",lua_content_type);
	lua_register(luabag->L,"set_out_header",lua_set_out_header);
	lua_register(luabag->L,"get_in_header",lua_get_in_header);
	
	apr_pool_userdata_set(luabag,LUABAG_TYPE,luabag_cleanup,r->connection->pool);
    }

    lsp_io lio={lctx:r, lputs:ap_def_puts, lputc:ap_def_putc, lwrite:ap_def_write};

    luaL_lsp_set_io(luabag->L,&lio);


    if(r->method_number==M_POST)
    {
	int rc;
	if((rc=read_request_data(luabag->L))!=OK)
    	    return rc;
    }

    luaL_lsp_setargs(luabag->L,r->args,r->args?strlen(r->args):0);
    

    lua_getfield(luabag->L,LUA_GLOBALSINDEX,"env");    
    luaL_lsp_setfield(luabag->L,"server_admin",r->server->server_admin);
    luaL_lsp_setfield(luabag->L,"server_hostname",r->server->server_hostname);
    luaL_lsp_setfield(luabag->L,"remote_ip",r->connection->remote_ip);
    luaL_lsp_setfield(luabag->L,"remote_host",r->connection->remote_host);
    luaL_lsp_setfield(luabag->L,"local_ip",r->connection->local_ip);
    luaL_lsp_setfield(luabag->L,"local_host",r->connection->local_host);
    luaL_lsp_setfield(luabag->L,"hostname",r->hostname);
    luaL_lsp_setfield(luabag->L,"method",r->method);
    luaL_lsp_setfield(luabag->L,"handler",r->handler);
    luaL_lsp_setfield(luabag->L,"uri",r->uri);
    luaL_lsp_setfield(luabag->L,"filename",r->filename);
    luaL_lsp_setfield(luabag->L,"args",r->args);
    lua_pop(luabag->L,1);
    
    luaL_lsp_chdir_to_file(luabag->L,r->filename);
    
    luaL_lsp_session_init(luabag->L,
	dir_conf->cookie_name?dir_conf->cookie_name:"LSPSESSID",
	dir_conf->cookie_days>0?dir_conf->cookie_days:7,
	dir_conf->cookie_path?dir_conf->cookie_path:"/");
    
    int status=0;
    
    switch(handler_type)
    {
    case handler_type_lsp: status=luaL_load_lsp_file(luabag->L,r->filename); r->content_type="text/html"; break;
    case handler_type_lua: status=luaL_loadfile(luabag->L,r->filename); r->content_type="text/plain"; break;
    }

    if(status)
    {
	const char* e=lua_tostring(luabag->L,-1);

	ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"%s",e);

        lua_pop(luabag->L,1);

	luaL_lsp_chdir_restore(luabag->L);

	return HTTP_INTERNAL_SERVER_ERROR;
    }

    status=lua_pcall(luabag->L,0,LUA_MULTRET,0);
    
    if(status)
    {
	const char* e=lua_tostring(luabag->L,-1);

	ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"%s",e);

	if(dir_conf->show_exception)	// if not 0
	    ap_rputs(e,r);

        lua_pop(luabag->L,1);
    }
    

    int rnum=lua_gettop(luabag->L);

    int result=OK;
    
    if(rnum>0)
    {
	result=lua_tointeger(luabag->L,-1);
	
	if(!result || result==200)
	    result=OK;
	
	lua_pop(luabag->L,rnum);
    }

    luaL_lsp_chdir_restore(luabag->L);

    if(result==OK)
	ap_rflush(r);

    return result;
}



int lsp::read_request_data(lua_State *L)
{
    request_rec* apr=(request_rec*)luaL_lsp_get_io_ctx(L);

    const char* p=apr_table_get(apr->headers_in,"Content-Length");
    int content_length=p?atoi(p):-1;

    if(content_length<0)
            return HTTP_LENGTH_REQUIRED;

    if(content_length>4096)
            return HTTP_BAD_REQUEST;

    int retval=OK;

    if((retval=ap_setup_client_block(apr,REQUEST_CHUNKED_ERROR))!=OK)
	return retval;

    luaL_Buffer buf;
    luaL_buffinit(L,&buf);

    if(ap_should_client_block(apr))
    {
	char tmp[1024];
        int len=0;

	
        while(len<content_length)
        {
            int n=content_length-len;

            n=ap_get_client_block(apr,tmp,n>sizeof(tmp)?sizeof(tmp):n);
            if(n<=0)
                break;

            len+=n;
	    
	    luaL_addlstring(&buf,tmp,n);
        }
	
	
	if(len!=content_length)
            retval=HTTP_REQUEST_TIME_OUT;
    }



    const char* content_type=apr_table_get(apr->headers_in,"Content-Type");

    int n=lua_gettop(L);

    if(content_type && !strcmp(content_type,"application/x-www-form-urlencoded"))
    {
	lua_getglobal(L,"args_decode");
	luaL_pushresult(&buf);

	if(lua_isfunction(L,-2) && !lua_pcall(L,1,1,0))
	    lua_setglobal(L,"args_post");
    }else
    {
	lua_getfield(L,LUA_GLOBALSINDEX,"env");
	luaL_pushresult(&buf);

	if(lua_istable(L,-2))
	    lua_setfield(L,-2,"content");
    }

    lua_pop(L,lua_gettop(L)-n);

    
    return retval;
}


