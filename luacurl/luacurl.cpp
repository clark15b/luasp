
/* 
 * Copyright (C) Anton Burdinuk
 */

#include "luacurl.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace libcurl
{
    enum {uri_len=256, path_len=512, hdr_len=256};
    
    enum {m_get=1, m_post=2};
    
    static const char LUA_CURL[]="LCURL*";
    
    struct LCURL
    {
	int method;
	char uri[uri_len];
	CURL* curl;
	luaL_Buffer buf;	// use for response
	curl_slist* hdrs;	// use for response
	long status;
    };

    FILE* fp=0;

    char proxy[uri_len]="";

    int timeout=0;

    int get_conf_param(lua_State* L,const char* name,char* dst,int ndst);
    int get_conf_param(lua_State* L,const char* name,int* dst);

    size_t _write_cb(void* ptr,size_t size,size_t nmemb,void* stream);

    int lua_curl_open(lua_State* L);
    int lua_curl_close(lua_State* L);
    int lua_curl_gc(lua_State* L);
    int lua_curl_tostring(lua_State* L);
    int lua_curl_send(lua_State* L);
    int lua_curl_status(lua_State* L);
    int lua_curl_add_hdr(lua_State* L);
    int lua_curl_get_hdr(lua_State* L);
    int lua_curl_escape(lua_State* L);
    int lua_curl_proxy(lua_State* L);
    int lua_curl_logfile(lua_State* L);
    int lua_curl_timeout(lua_State* L);
}

int libcurl::get_conf_param(lua_State* L,const char* name,char* dst,int ndst)
{
    *dst=0;

    lua_getglobal(L,"config");
    lua_getfield(L,-1,name);

    int n=0;

    const char* s=lua_tostring(L,-1);
    if(s)
    {
	n=snprintf(dst,ndst-1,"%s",s);
	if(n<0 || n>=ndst)
	{
	    n=ndst-1;
	    dst[n]=0;
	}
    }

    lua_pop(L,2);

    return n;
}

int libcurl::get_conf_param(lua_State* L,const char* name,int* dst)
{
    *dst=0;

    lua_getglobal(L,"config");
    lua_getfield (L,-1,name);

    int n=lua_tointeger(L,-1);

    lua_pop(L,2);
    
    if(dst)
	*dst=n;

    return n;
}

int libcurl::lua_curl_proxy(lua_State* L)
{
    const char* p=luaL_checkstring(L,1);
    
    int n=snprintf(proxy,sizeof(proxy),"%s",p);
    if(n<0 || n>=sizeof(proxy))
	proxy[sizeof(proxy)-1]=0;

    return 0;
}
int libcurl::lua_curl_logfile(lua_State* L)
{
    if(fp)
    {
	fclose(fp);
	fp=0;
    }

    const char* file=luaL_checkstring(L,1);

    fp=fopen(file,"a");

    return 0;
}
int libcurl::lua_curl_timeout(lua_State* L)
{
    timeout=luaL_checkint(L,1);
    
    return 0;
}

void luaclose_luacurl(void)
{
    if(libcurl::fp)
    {
	fclose(libcurl::fp);
	libcurl::fp=0;
    }
    curl_global_cleanup();
}

int luaopen_luacurl(lua_State* L)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);


    static const luaL_Reg lib[]=
    {
        {"proxy",libcurl::lua_curl_proxy},
        {"log_file",libcurl::lua_curl_logfile},
        {"timeout",libcurl::lua_curl_timeout},
        {"open",libcurl::lua_curl_open},
        {"close",libcurl::lua_curl_close},
	{"escape",libcurl::lua_curl_escape},
	// connection functions
        {"send",libcurl::lua_curl_send},
        {"status",libcurl::lua_curl_status},
        {"set_request_header",libcurl::lua_curl_add_hdr},
        {"get_response_header",libcurl::lua_curl_get_hdr},
        {0,0}
    };

    static const luaL_Reg clib[]=
    {
        {"__gc",libcurl::lua_curl_gc},
        {"__tostring",libcurl::lua_curl_tostring},
        {"send",libcurl::lua_curl_send},
        {"close",libcurl::lua_curl_close},
        {"status",libcurl::lua_curl_status},
        {"set_request_header",libcurl::lua_curl_add_hdr},
	{"escape",libcurl::lua_curl_escape},
        {"get_response_header",libcurl::lua_curl_get_hdr},
        {0,0}
    };

    luaL_newmetatable(L,libcurl::LUA_CURL);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");
    luaL_register(L,0,clib);
    luaL_register(L,"curl",lib);

#ifdef LUA_MODULE
    atexit(luaclose_luacurl);
#endif /*LUA_MODULE*/

    return 0;
}


size_t libcurl::_write_cb(void* ptr,size_t size,size_t nmemb,void* stream)
{
    if(stream)
    {
	luaL_Buffer* buf=(luaL_Buffer*)stream;
	
	luaL_addlstring(buf,(char*)ptr,size*nmemb);

	return nmemb;
    }
    return 0;
}

int libcurl::lua_curl_open(lua_State* L)
{
    if(lua_gettop(L)!=2)
        return luaL_error(L,"\"curl.open\" bad params");

    const char* method=luaL_checkstring(L,1);
    const char* uri=luaL_checkstring(L,2);

    LCURL* lcurl=(LCURL*)lua_newuserdata(L,sizeof(LCURL));

    luaL_getmetatable(L,LUA_CURL);
    lua_setmetatable(L,-2);

    *lcurl->uri=0;
    lcurl->method=0;
    lcurl->curl=curl_easy_init();
    lcurl->status=0;
    lcurl->hdrs=0;

    if(!lcurl->curl)
	return luaL_error(L,"curl_easy_init");
    
    if(!strcasecmp(method,"GET")) lcurl->method=m_get;
    else if(!strcasecmp(method,"POST")) lcurl->method=m_post;

    int n=snprintf(lcurl->uri,uri_len,"%s",uri);
    if(n<0 || n>=uri_len)
	lcurl->uri[uri_len-1]=0;

    if(timeout>=0)
	curl_easy_setopt(lcurl->curl,CURLOPT_TIMEOUT,(long)timeout);

    curl_easy_setopt(lcurl->curl,CURLOPT_PROXY,proxy);

    if(fp)
    {
	curl_easy_setopt(lcurl->curl,CURLOPT_STDERR,fp);
	curl_easy_setopt(lcurl->curl,CURLOPT_VERBOSE,(long)1);
    }

    curl_easy_setopt(lcurl->curl,CURLOPT_URL,uri);

    curl_easy_setopt(lcurl->curl,CURLOPT_WRITEFUNCTION,_write_cb);
    curl_easy_setopt(lcurl->curl,CURLOPT_WRITEDATA,&lcurl->buf);

    return 1;
}

int libcurl::lua_curl_close(lua_State* L)
{
    LCURL* lcurl=(LCURL*)luaL_checkudata(L,1,LUA_CURL);

    if(lcurl->curl)
    {
	curl_easy_cleanup(lcurl->curl);
	lcurl->curl=0;
    }

    if(lcurl->hdrs)
    {
	curl_slist_free_all(lcurl->hdrs);
	lcurl->hdrs=0;
    }

		
    return 0;
}

int libcurl::lua_curl_gc(lua_State* L)
{
    return lua_curl_close(L);
}

int libcurl::lua_curl_tostring(lua_State* L)
{
    LCURL* lcurl=(LCURL*)luaL_checkudata(L,1,LUA_CURL);

    lua_pushstring(L,(lcurl && *lcurl->uri)?lcurl->uri:"(nil)");

    return 1;
}

int libcurl::lua_curl_status(lua_State* L)
{
    LCURL* lcurl=(LCURL*)luaL_checkudata(L,1,LUA_CURL);

    lua_pushinteger(L,lcurl?lcurl->status:0);

    return 1;
}

int libcurl::lua_curl_escape(lua_State* L)
{
    CURL* curl=0;
    
    size_t l=0;

    const char* s=0;

    if(lua_isuserdata(L,1))
    {
	LCURL* lcurl=(LCURL*)luaL_checkudata(L,1,LUA_CURL);
	curl=lcurl->curl;

	s=luaL_checklstring(L,2,&l);
    }else
	s=luaL_checklstring(L,1,&l);
    
    int rc=0;
    
    if(s && l>0)
    {
	char* s2=curl_easy_escape(curl,s,l);
	if(s2)
	{
	    lua_pushstring(L,s2);
	    curl_free(s2);
	    rc=1;
	}
    }

    return rc;
}

int libcurl::lua_curl_add_hdr(lua_State* L)
{
    if(lua_gettop(L)<3)
        return luaL_error(L,"\"curl:set_request_header\" bad params");

    LCURL* lcurl=(LCURL*)luaL_checkudata(L,1,LUA_CURL);

    if(!lcurl)
        return luaL_error(L,"\"curl:set_request_header\" invalid curl object");

    char h[hdr_len];

    int n=snprintf(h,sizeof(h),"%s: %s",luaL_checkstring(L,2),luaL_checkstring(L,3));
    if(n<0 || n>=sizeof(h))
	h[sizeof(h)-1]=0;

    lcurl->hdrs=curl_slist_append(lcurl->hdrs,h);
    
    return 0;
}

int libcurl::lua_curl_get_hdr(lua_State* L)
{
    if(lua_gettop(L)<3)
        return luaL_error(L,"\"curl:lua_curl_get_hdr\" bad params");

    LCURL* lcurl=(LCURL*)luaL_checkudata(L,1,LUA_CURL);

    if(!lcurl)
        return luaL_error(L,"\"curl:lua_curl_get_hdr\" invalid curl object");

    // implement (callback needed)

    return 0;
}

int libcurl::lua_curl_send(lua_State* L)
{
    if(lua_gettop(L)<1)
        return luaL_error(L,"\"curl:send\" bad params");

    LCURL* lcurl=(LCURL*)luaL_checkudata(L,1,LUA_CURL);

    if(!lcurl)
        return luaL_error(L,"\"curl:send\" invalid curl object");
	
    const char* content="";
    size_t content_length=0;

    if(lua_gettop(L)>1)
	content=luaL_checklstring(L,2,&content_length);

    if(lcurl->method==m_post)
    {
	curl_easy_setopt(lcurl->curl,CURLOPT_POSTFIELDS,content);
	curl_easy_setopt(lcurl->curl,CURLOPT_POSTFIELDSIZE,content_length);
    }

    curl_easy_setopt(lcurl->curl,CURLOPT_HTTPHEADER,lcurl->hdrs);


    luaL_buffinit(L,&lcurl->buf);
    lcurl->status=0;

    CURLcode rc=curl_easy_perform(lcurl->curl);

    if(rc!=CURLE_OK)
	return 0;
    
    curl_easy_getinfo(lcurl->curl,CURLINFO_RESPONSE_CODE,&lcurl->status);

    luaL_pushresult(&lcurl->buf);

    return 1;
}

