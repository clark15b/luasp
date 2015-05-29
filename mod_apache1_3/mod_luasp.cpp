
/*  
 * Copyright (C) Anton Burdinuk 
 */
  

/*
**  Activate it in Apache's 1.3 apache.conf file for instance
**    #   apache.conf
**    LoadModule luasp_module modules/mod_luasp.so
**    AddType application/x-httpd-lsp .lsp
**    AddType application/x-httpd-lua .lua
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
#include "ap_config.h" 

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


namespace lsp
{
    enum
    {
	handler_type_unknown=0,
	handler_type_lsp=1,
	handler_type_lua=2
    };
    
    const char cookie_name[]="LSPSESSID";
    const int cookie_days=7;
    const char cookie_path[]="/";
    
    int show_exception=1;

    lua_State* L=0;
    
    request_rec* apr;

    int lua_log(lua_State *L);
    int lua_content_type(lua_State *L);
    int lua_set_out_header(lua_State *L);
    int lua_get_in_header(lua_State *L);
    int lua_uuid_gen(lua_State *L);
    int lua_version(lua_State *L);

    int handler_lsp(request_rec *r);
    int handler_lua(request_rec *r);
    int handler(request_rec *r,int handler_type);

    int read_request_data(lua_State *L);
    
    
    int data_sent=0;

    int ap_def_puts(void*,const char* s)
    {
	if(!data_sent)
	{
	    ap_send_http_header(apr);
	    data_sent++;
	}
	return ap_rputs(s,apr);
    }
    
    int ap_def_putc(void*,int c)
    {
	if(!data_sent)
	{
	    ap_send_http_header(apr);
	    data_sent++;
	}
	return ap_rputc(c,apr);
    }
    
    int ap_def_write(void*,const char* s,size_t len)
    {
	if(!data_sent)
	{
	    ap_send_http_header(apr);
	    data_sent++;
	}
	return ap_rwrite(s,len,apr);
    }
}


handler_rec lsp_handlers[] =
{
    {"application/x-httpd-lsp",lsp::handler_lsp},
    {"application/x-httpd-lua",lsp::handler_lua},
    {NULL}
};

static void lsp_init(server_rec* s,pool* p)
{
    lsp::L=lua_open();
	
    luaL_openlibs(lsp::L);
	
    luaopen_lualsp(lsp::L);		

    luaopen_luamysql(lsp::L);
	
    luaopen_luacurl(lsp::L);
    
    luaopen_luajson(lsp::L);
    
    luaopen_luamcache(lsp::L);

    lua_register(lsp::L,"module_version",lsp::lua_version);
    lua_register(lsp::L,"log",lsp::lua_log);
    lua_register(lsp::L,"content_type",lsp::lua_content_type);
    lua_register(lsp::L,"set_out_header",lsp::lua_set_out_header);
    lua_register(lsp::L,"get_in_header",lsp::lua_get_in_header);
    lua_register(lsp::L,"uuid_gen",lsp::lua_uuid_gen);
	

    lsp_io lio={lctx:0, lputs:lsp::ap_def_puts, lputc:lsp::ap_def_putc, lwrite:lsp::ap_def_write};
    
    luaL_lsp_set_io(lsp::L,&lio);
}

static void lsp_done(server_rec* s,pool* p)
{
    if(lsp::L)
    {
	luaclose_luamysql();
        luaclose_luacurl();
	luaclose_luajson();
	luaclose_luamcache();
	lua_close(lsp::L);
	lsp::L=0;
    }
}


module MODULE_VAR_EXPORT luasp_module = {
    STANDARD_MODULE_STUFF, 
    NULL,                  /* module initializer                  */
    NULL,                  /* create per-dir    config structures */
    NULL,                  /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    NULL,                  /* table of config file commands       */
    lsp_handlers,          /* [#8] MIME-typed-dispatched handlers */
    NULL,                  /* [#1] URI to filename translation    */
    NULL,                  /* [#4] validate user id from request  */
    NULL,                  /* [#5] check if the user is ok _here_ */
    NULL,                  /* [#3] check access by host address   */
    NULL,                  /* [#6] determine MIME type            */
    NULL,                  /* [#7] pre-run fixups                 */
    NULL,                  /* [#9] log a transaction              */
    NULL,                  /* [#2] header parser                  */
    lsp_init,              /* child_init                          */
    lsp_done,              /* child_exit                          */
    NULL                   /* [#0] post read-request              */
#ifdef EAPI
   ,NULL,                  /* EAPI: add_module                    */
    NULL,                  /* EAPI: remove_module                 */
    NULL,                  /* EAPI: rewrite_command               */
    NULL                   /* EAPI: new_connection                */
#endif
};




int lsp::lua_log(lua_State *L)
{
    const char* s=luaL_checkstring(L,1);

    ap_log_rerror(APLOG_MARK,APLOG_NOTICE,apr,"%s",s);
    
    return 0;
}

int lsp::lua_content_type(lua_State *L)
{
    const char* s=luaL_checkstring(L,1);

    apr->content_type=ap_pstrdup(apr->pool,s);
    
    return 0;
}

int lsp::lua_set_out_header(lua_State *L)
{
    const char* s1=luaL_checkstring(L,1);
    const char* s2=0;
    
    if(lua_gettop(L)>1)
	s2=luaL_checkstring(L,2);

    if(!s2 || !*s2)
	ap_table_unset(apr->headers_out,s1);
    else
	ap_table_set(apr->headers_out,s1,s2);

    return 0;
}

int lsp::lua_get_in_header(lua_State *L)
{
    const char* s1=luaL_checkstring(L,1);

    const char* s2=ap_table_get(apr->headers_in,s1);

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

int lsp::handler_lsp(request_rec *r)
{
    return handler(r,handler_type_lsp);
}

int lsp::handler_lua(request_rec *r)
{
    return handler(r,handler_type_lua);
}

int lsp::handler(request_rec *r,int handler_type)
{
    apr=r;
    data_sent=0;

    if(!r->filename)
	return HTTP_INTERNAL_SERVER_ERROR;

    if(!r->finfo.st_mode)
	return HTTP_NOT_FOUND;
    if(!S_ISREG(r->finfo.st_mode) && !S_ISLNK(r->finfo.st_mode))
	return HTTP_FORBIDDEN;
			
    if (r->header_only)
    {
	ap_send_http_header(apr);
	return OK;
    }

    if(r->method_number==M_POST)
    {
	int rc;
	if((rc=read_request_data(L))!=OK)
    	    return rc;
    }

    luaL_lsp_setargs(L,apr->args,apr->args?strlen(apr->args):0);

    lua_getfield(L,LUA_GLOBALSINDEX,"env");    
    luaL_lsp_setfield(L,"server_admin",r->server->server_admin);
    luaL_lsp_setfield(L,"server_hostname",r->server->server_hostname);
    luaL_lsp_setfield(L,"remote_ip",r->connection->remote_ip);
    luaL_lsp_setfield(L,"remote_host",r->connection->remote_host);
    luaL_lsp_setfield(L,"local_ip",r->connection->local_ip);
    luaL_lsp_setfield(L,"local_host",r->connection->local_host);
    luaL_lsp_setfield(L,"hostname",r->hostname);
    luaL_lsp_setfield(L,"method",r->method);
    luaL_lsp_setfield(L,"handler",r->handler);
    luaL_lsp_setfield(L,"uri",r->uri);
    luaL_lsp_setfield(L,"filename",r->filename);
    luaL_lsp_setfield(L,"args",r->args);
    lua_pop(L,1);
    
    luaL_lsp_chdir_to_file(L,r->filename);
    
    luaL_lsp_session_init(L,cookie_name,cookie_days,cookie_path);
    
    int status=0;
    
    switch(handler_type)
    {
    case handler_type_lsp: status=luaL_load_lsp_file(L,r->filename); r->content_type="text/html"; break;
    case handler_type_lua: status=luaL_loadfile(L,r->filename); r->content_type="text/plain"; break;
    }
    
    if(status)
    {
	const char* e=lua_tostring(L,-1);

	ap_log_rerror(APLOG_MARK,APLOG_ERR,r,"%s",e);

        lua_pop(L,1);

        luaL_lsp_chdir_restore(L);
	        
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    status=lua_pcall(L,0,LUA_MULTRET,0);
    
    if(status)
    {
	const char* e=lua_tostring(L,-1);

	ap_log_rerror(APLOG_MARK,APLOG_ERR,r,"%s",e);
	
        if(show_exception)
	{
	    if(!data_sent)
	    {
		ap_send_http_header(apr);
		data_sent++;
	    }
            ap_rputs(e,r);
	}

        lua_pop(L,1);
    }
    
    int rnum=lua_gettop(L);

    int result=OK;

    if(rnum>0)
    {
        result=lua_tointeger(L,-1);

        if(!result || result==200)
            result=OK;
			    
	lua_pop(L,rnum);
    }

    luaL_lsp_chdir_restore(L);

    if(result==OK)
    {
	if(!data_sent)
	    ap_send_http_header(apr);

	ap_rflush(apr);
    }

    return result;
}



int lsp::read_request_data(lua_State *L)
{
    const char* p=ap_table_get(apr->headers_in,"Content-Length");
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


    const char* content_type=ap_table_get(apr->headers_in,"Content-Type");

    int n=lua_gettop(L);

    if(content_type && !strcmp(content_type,"application/x-form-urlencoded"))
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


