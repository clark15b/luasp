
/* 
 * Copyright (C) Anton Burdinuk
 */

#include "luamcache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <linux/un.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>

namespace libmcache
{
    struct LMCACHE
    {
	FILE* fp;
    };

    static const char LUA_MCACHE[]="LMCACHE*";

    static const char err_socket_path_too_long[]=	"mcache: socket path too long";
    static const char err_errno[]=			"mcache: %s";
    static const char err_invalid_address_format[]=	"mcache: invalid address format";
    static const char err_invalid_address_length[]=	"mcache: invalid address length";
    static const char err_host_is_not_found[]=		"mcache: host %s is not found";
    static const char err_closed[]=			"mcache: closed";
    static const char err_io[]=				"mcache: I/O error";
    static const char err_invalid_server_response[]=	"mcache: invalid server response \"%s\"";


    int lua_mcache_open(lua_State* L);
    int lua_mcache_close(lua_State* L);
    int lua_mcache_gc(lua_State* L);
    int lua_mcache_tostring(lua_State* L);
    int lua_mcache_set(lua_State* L);
    int lua_mcache_get(lua_State* L);
    int lua_mcache_del(lua_State* L);

    void lua_mcache_close(LMCACHE* mcache);

    char* lua_mcache_find_substring_by_id(char* p,int id);
}


void luaclose_luamcache(void)
{
}


int luaopen_luamcache(lua_State* L)
{
    static const luaL_Reg lib[]=
    {
        {"open",libmcache::lua_mcache_open},
        {"close",libmcache::lua_mcache_close},
        {"set",libmcache::lua_mcache_set},
        {"get",libmcache::lua_mcache_get},
        {"del",libmcache::lua_mcache_del},
        {0,0}
    };

    static const luaL_Reg clib[]=
    {
	{"__gc",libmcache::lua_mcache_gc},
        {"__tostring",libmcache::lua_mcache_tostring},
        {"close",libmcache::lua_mcache_close},
        {"set",libmcache::lua_mcache_set},
        {"get",libmcache::lua_mcache_get},
        {"del",libmcache::lua_mcache_del},
        {0,0}
    };					

    luaL_newmetatable(L,libmcache::LUA_MCACHE);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");
    luaL_register(L,0,clib);
    luaL_register(L,"mcache",lib);

#ifdef LUA_MODULE
    atexit(luaclose_luamcache);
#endif /*LUA_MODULE*/

    return 0;
}


void libmcache::lua_mcache_close(LMCACHE* mcache)
{
    if(mcache->fp)
    {
	fclose(mcache->fp);
	mcache->fp=0;
    }
}

int libmcache::lua_mcache_close(lua_State* L)
{
    LMCACHE* mcache=(LMCACHE*)luaL_checkudata(L,1,LUA_MCACHE);

    lua_mcache_close(mcache);

    return 0;
}

int libmcache::lua_mcache_gc(lua_State* L)
{
    return lua_mcache_close(L);
}

int libmcache::lua_mcache_tostring(lua_State* L)
{
    LMCACHE* mcache=(LMCACHE*)luaL_checkudata(L,1,LUA_MCACHE);

    lua_pushstring(L,mcache->fp?"opened":"closed");
	    
    return 1;
}

int libmcache::lua_mcache_open(lua_State* L)
{
    size_t l=0;

    const char* p=luaL_checklstring(L,1,&l);
    
    int fd=-1;

    if(*p=='/')
    {
	if(l>=UNIX_PATH_MAX)
	    return luaL_error(L,err_socket_path_too_long);

        sockaddr_un sun;
        sun.sun_family=AF_UNIX;
	strcpy(sun.sun_path,p);
	
	fd=socket(sun.sun_family,SOCK_STREAM,0);
	
	if(fd==-1)
	    return luaL_error(L,err_errno,strerror(errno));

	if(connect(fd,(sockaddr*)&sun,sizeof(sun)))
	{
	    close(fd);
	    return luaL_error(L,err_errno,strerror(errno));
	}				    
    }else
    {
	const char* p2=strchr(p,':');
	
	if(!p2)
	    return luaL_error(L,err_invalid_address_format);
	
	char tmp[128];
	
	size_t l=p2-p;
	
	if(l>sizeof(tmp)-1)
	    return luaL_error(L,err_invalid_address_length);
	
	memcpy(tmp,p,l);
	tmp[l]=0;
	
	sockaddr_in sin;
	sin.sin_family=AF_INET;
	sin.sin_port=htons(atoi(p2+1));
	sin.sin_addr.s_addr=inet_addr(tmp);

	if(sin.sin_addr.s_addr==INADDR_NONE)
        {
	    hostent* he=gethostbyname(tmp);
            if(!he)
		return luaL_error(L,err_host_is_not_found,tmp);
            memcpy((char*)&sin.sin_addr.s_addr,he->h_addr,sizeof(sin.sin_addr.s_addr));
        }

	fd=socket(sin.sin_family,SOCK_STREAM,0);
	
	if(fd==-1)
	    return luaL_error(L,err_errno,strerror(errno));

	if(connect(fd,(sockaddr*)&sin,sizeof(sin)))
	{
	    close(fd);
	    return luaL_error(L,err_errno,strerror(errno));
	}				    

    }

    int on=1;
    setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&on,sizeof(on));
    
    FILE* fp=fdopen(fd,"r+");
    
    if(!fp)
    {
	close(fd);
        return luaL_error(L,err_errno,strerror(errno));
    }

    LMCACHE* mcache=(LMCACHE*)lua_newuserdata(L,sizeof(LMCACHE));

    luaL_getmetatable(L,LUA_MCACHE);
    lua_setmetatable(L,-2);

    mcache->fp=fp;
	    
    return 1;
}

int libmcache::lua_mcache_set(lua_State* L)
{
    LMCACHE* mcache=(LMCACHE*)luaL_checkudata(L,1,LUA_MCACHE);
    
    if(!mcache->fp)
        return luaL_error(L,err_closed);
    
    const char* key=luaL_checkstring(L,2);
    
    size_t value_len=0;
    const char* value=luaL_checklstring(L,3,&value_len);
    
    u_int32_t exp=0;
    
    if(lua_gettop(L)>3)
	exp=luaL_checkinteger(L,4);

    fprintf(mcache->fp,"set %s 0 %i %i\r\n",key,exp,value_len);
    
    size_t l=0;
    while(l<value_len)
    {
	size_t n=fwrite(value+l,1,value_len-l,mcache->fp);
	if(!n)
	    break;
	l+=n;
    }
    
    if(l!=value_len)
    {
	lua_mcache_close(mcache);
        return luaL_error(L,err_io);
    }

    fprintf(mcache->fp,"\r\n");
    
    fflush(mcache->fp);

    char result[128];
    if(!fgets(result,sizeof(result),mcache->fp))
    {
	lua_mcache_close(mcache);
        return luaL_error(L,err_io);
    }
    
    char* p=strchr(result,'\r');
    if(p)
	*p=0;

    if(strcmp(result,"STORED"))
	return luaL_error(L,err_errno,result);

    return 0;
}


char* libmcache::lua_mcache_find_substring_by_id(char* p,int id)
{
    int flag=0;
    int n=0;

    while(*p)
    {
	if(!flag)
	{
	    if(*p!=' ')
	    {
		if(n>=id)
		    return p;

		flag=1;
		n++;
	    }
	}else
	{
	    if(*p==' ') { flag=0; }
	}
	p++;
    }
    
    return 0;
}


int libmcache::lua_mcache_get(lua_State* L)
{
    int top=lua_gettop(L);

    LMCACHE* mcache=(LMCACHE*)luaL_checkudata(L,1,LUA_MCACHE);
    
    if(!mcache->fp)
        return luaL_error(L,err_closed);
    
    const char* key=luaL_checkstring(L,2);

    fprintf(mcache->fp,"get %s\r\n",key);

    fflush(mcache->fp);

    char result[512];

    static const char end_tag[]="END";
    static const char value_tag[]="VALUE";
    
    for(;;)
    {    
	if(!fgets(result,sizeof(result),mcache->fp))
	{
	    lua_mcache_close(mcache);
	    return luaL_error(L,err_io);
	}
    
	char* p=strchr(result,'\r');
	if(p)
	    *p=0;

	if(!strncmp(result,end_tag,sizeof(end_tag)-1))
	    break;
	
	if(strncmp(result,value_tag,sizeof(value_tag)-1))
	{
	    lua_mcache_close(mcache);
	    return luaL_error(L,err_invalid_server_response,result);
	}
	

	char* len=lua_mcache_find_substring_by_id(result+(sizeof(value_tag)-1),2);
	
	if(!len)
	{
	    lua_mcache_close(mcache);
	    return luaL_error(L,err_invalid_server_response,result);
	}
	
	p=strchr(len,' ');
	if(p)
	    *p=0;
	
	size_t length=atol(len);

	luaL_Buffer buf;
	luaL_buffinit(L,&buf);
	
	size_t l=0;
	
	while(l<length)
	{
	    size_t m=length-l;

	    size_t n=fread(result,1,m<sizeof(result)?m:sizeof(result),mcache->fp);
	    
	    if(!n)
		break;

	    luaL_addlstring(&buf,result,n);

	    l+=n;
	}
	
	luaL_pushresult(&buf);
	
	if(l!=length || !fgets(result,sizeof(result),mcache->fp))
	{
	    lua_mcache_close(mcache);
	    return luaL_error(L,err_io);
	}
    }

    return lua_gettop(L)-top;
}

int libmcache::lua_mcache_del(lua_State* L)
{
    LMCACHE* mcache=(LMCACHE*)luaL_checkudata(L,1,LUA_MCACHE);
    
    if(!mcache->fp)
        return luaL_error(L,err_closed);
    
    const char* key=luaL_checkstring(L,2);

    fprintf(mcache->fp,"delete %s\r\n",key);
    
    fflush(mcache->fp);

    char result[128];
    if(!fgets(result,sizeof(result),mcache->fp))
    {
	lua_mcache_close(mcache);
        return luaL_error(L,err_io);
    }
    
    return 0;
}
