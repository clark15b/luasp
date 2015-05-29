
/*  
 * Copyright (C) Anton Burdinuk 
 */
  

#include "llsplib.h"
#include <string.h>
#include <unistd.h>
#include <time.h>

namespace llspaux
{
    int urldecode(lua_State* L,const char* s,size_t len);
    int args(lua_State* L,const char* p,size_t len);

    int lua_urldecode(lua_State* L);
    int lua_args(lua_State* L);
    
    static const char cwd_regkey[]="lsp_cwd";
}

int luaopen_lualspaux(lua_State *L)
{
    lua_register(L,"url_decode",llspaux::lua_urldecode);
    lua_register(L,"args_decode",llspaux::lua_args);

    lua_newtable(L);
    lua_setfield(L,LUA_GLOBALSINDEX,"env");

    return 0;
}

int llspaux::lua_urldecode(lua_State* L)
{
    size_t len=0;

    const char* p=lua_tolstring(L,1,&len);

    return urldecode(L,p,len);
}

int llspaux::urldecode(lua_State* L,const char* s,size_t len)
{
    static const unsigned char hex[256]=
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    luaL_Buffer buf;
    
    luaL_buffinit(L,&buf);

    for(size_t i=0;i<len;++i)
    {
	switch(s[i])
	{
	case '+': luaL_addchar(&buf,' '); break;
	case '%':
	    if(s[i+1])
	    {
		i++;

		unsigned char c1=hex[s[i]];

		if(s[i+1])
		{		
		    i++;
		    
		    unsigned char c2=hex[s[i]];

		    if(c1!=0xff && c2!=0xff)
			luaL_addchar(&buf,((c1<<4)&0xf0)|(c2&0x0f));
		    else
			luaL_addchar(&buf,'.');
		}
	    }
	    break;
	default: luaL_addchar(&buf,s[i]); break;
	}	
    }
    
    luaL_pushresult(&buf);

    return 1;
}

int llspaux::lua_args(lua_State* L)
{
    size_t len=0;

    const char* p=lua_tolstring(L,1,&len);

    return args(L,p,len);
}

int llspaux::args(lua_State* L,const char* p,size_t len)
{
    lua_newtable(L);
    
    size_t offset1=0,length1=0;
    size_t offset2=0,length2=0;

    size_t i;

    for(i=0;i<len;++i)
    {    
	if(p[i]=='=')
	{
	    length1=i-offset1;
	    offset2=i+1;
	}else if(p[i]=='&')
	{
	    length2=i-offset2;
	    
	    if(length1 && length2)
	    {	    
		lua_pushlstring(L,p+offset1,length1);
		urldecode(L,p+offset2,length2);
		lua_rawset(L,-3);
	    }
	    
	    offset1=i+1;
	    length1=offset2=length2=0;
	}
    }

    length2=i-offset2;

    if(length1 && length2)
    {	    
	lua_pushlstring(L,p+offset1,length1);
	urldecode(L,p+offset2,length2);
	lua_rawset(L,-3);
    }

    return 1;
}

int luaL_lsp_setargs(lua_State *L,const char* p,size_t len)
{
    llspaux::args(L,p,len);

    lua_setglobal(L,"args");

    return 0;
}

int luaL_lsp_chdir_to_file(lua_State *L,const char* path)
{
    char tmp[PATH_MAX]="";

    const char* p=strrchr(path,'/');
    if(p)
    {
	if(getcwd(tmp,sizeof(tmp)))
	{
	    lua_pushstring(L,tmp);
	    lua_setfield(L,LUA_REGISTRYINDEX,llspaux::cwd_regkey);
	}

	int n=p-path;
	strncpy(tmp,path,n);
	tmp[n]=0;
	
	chdir(tmp);
    }

    return 0;   
}

int luaL_lsp_chdir_restore(lua_State *L)
{
    lua_getfield(L,LUA_REGISTRYINDEX,llspaux::cwd_regkey);

    const char* p=lua_tostring(L,-1);

    if(p)
	chdir(p);
    
    lua_pop(L,1);
    
    return 0;
}



int luaL_lsp_setfield(lua_State *L,const char* name,const char* value)
{
    if(value)
    {
	lua_pushstring(L,value);
	lua_setfield(L,-2,name);
    }
    
    return 0;
}


int luaL_lsp_setenv(lua_State *L,const char* name,const char* value)
{
    if(value)
    {
	lua_getfield(L,LUA_GLOBALSINDEX,"env");
	lua_pushstring(L,value);
	lua_setfield(L,-2,name);
	lua_pop(L,1);
    }
    
    return 0;
}

int luaL_lsp_setenv_len(lua_State *L,const char* name,const char* value,size_t len)
{
    if(value || len>0)
    {
	lua_getfield(L,LUA_GLOBALSINDEX,"env");
	lua_pushlstring(L,value,len);
	lua_setfield(L,-2,name);
	lua_pop(L,1);
    }
    
    return 0;
}

int luaL_lsp_setenv_buf(lua_State *L,const char* name,luaL_Buffer* buf)
{
    lua_getfield(L,LUA_GLOBALSINDEX,"env");
    luaL_pushresult(buf);
    lua_setfield(L,-2,name);
    lua_pop(L,1);
    
    return 0;
}


static const char* get_gmt_date(time_t timestamp,char* buf)
{
    tm* t=gmtime(&timestamp);
    
    static const char* wd[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char* m[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	        
    sprintf(buf,"%s, %i %s %.4i %.2i:%.2i:%.2i GMT",wd[t->tm_wday],t->tm_mday,m[t->tm_mon],t->tm_year+1900,
        t->tm_hour,t->tm_min,t->tm_sec);

    return buf;
}
				

int luaL_lsp_session_init(lua_State *L,const char* cookie_name,int days,const char* cookie_path)
{
    char tmp[256];

    const char* session="";

    // find session id in cookie

    const char* p="";

    int n=lua_gettop(L);

    lua_getglobal(L,"get_in_header");
    if(lua_isfunction(L,-1))
    {
	lua_pushstring(L,"Cookie");
	if(!lua_pcall(L,1,1,0))
	    p=lua_tostring(L,-1);
    }

    for(const char* p1=p,*p2;p1;p1=p2)
    {
	while(*p1 && *p1==' ')
	    p1++;

	int l=0;
	
	p2=strchr(p1,';');
	if(p2) { l=p2-p1; p2++; }
	else l=strlen(p1);
        
	if(l>=sizeof(tmp))
	    l=sizeof(tmp)-1;
	memcpy(tmp,p1,l);
	tmp[l]=0;

	char* p3=strchr(tmp,'=');
	if(p3)
	{
	    *p3=0;
	    if(!strcmp(tmp,cookie_name))
	    {
		session=p3+1;
		break;
	    }
	}
    }
    lua_pop(L,lua_gettop(L)-n);

    // gen new session if empty and push it to headers
    if(!*session)
    {
	session=tmp;
	*tmp=0;

	int n=lua_gettop(L);
	lua_getglobal(L,"uuid_gen");
	if(lua_isfunction(L,-1))
	{
	    if(!lua_pcall(L,0,1,0))
	    {
		size_t l=0;
		const char* p=lua_tolstring(L,-1,&l);
		
		if(p)
		{
		    if(l>=sizeof(tmp))
			l=sizeof(tmp)-1;
		    memcpy(tmp,p,l);
		    tmp[l]=0;
		}
	    }
	}
	lua_pop(L,lua_gettop(L)-n);

	if(*session)
	{
	    int n=lua_gettop(L);

	    lua_getglobal(L,"set_out_header");
	    if(lua_isfunction(L,-1))
	    {
		lua_pushstring(L,"Set-Cookie");
		
		if(days>0)
		{
		    char d[64];
		    time_t t=0;
		    time(&t);
		    get_gmt_date(t+(days*24*60*60),d);
		    if(cookie_path && *cookie_path)
			lua_pushfstring(L,"%s=%s; expires=%s; path=%s",cookie_name,session,d,cookie_path);
		    else
			lua_pushfstring(L,"%s=%s; expires=%s",cookie_name,session,d);
		}
		else
		{
		    if(cookie_path && *cookie_path)
			lua_pushfstring(L,"%s=%s; path=%s",cookie_name,session,cookie_path);
		    else
			lua_pushfstring(L,"%s=%s",cookie_name,session);
		}

	        lua_pcall(L,2,0,0);
	    }

	    lua_pop(L,lua_gettop(L)-n);
	}
    }
    
    lua_getglobal(L,"env");
    if(lua_istable(L,-1))
    {
        lua_pushstring(L,session);
	lua_setfield(L,-2,"session");
    }    
    lua_pop(L,1);
    
    return 0;
}
