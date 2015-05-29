
/*  
 * Copyright (C) Anton Burdinuk 
 */


#include "luamysql.h"
#include <mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace libmysql
{
    static const char LUA_MYSQL[]="LMYSQL*";
    static const char LUA_MYSQL_RESULT[]="LMYSQL_RESULT*";
    
    class membuf
    {
    public:
	char* ptr;
	size_t len;
	
	membuf(size_t _len):ptr((char*)malloc(_len)),len(_len) {}
	~membuf(void) {if(ptr) free(ptr);}
    };

    struct LMYSQL
    {
	int online;
	MYSQL svc;
    };
    
    struct LMYSQL_RESULT
    {
	MYSQL_RES* result;
	int fnum;
    };
    
    int lua_mysql_open(lua_State* L);
    int lua_mysql_close(lua_State* L);
    int lua_mysql_gc(lua_State* L);
    int lua_mysql_tostring(lua_State* L);
    int lua_mysql_escape(lua_State* L);
    int lua_mysql_affected_rows(lua_State* L);
    int lua_mysql_insert_id(lua_State* L);
    int lua_mysql_query(lua_State* L);

    int lua_mysql_result_close(lua_State* L);
    int lua_mysql_result_gc(lua_State* L);
    int lua_mysql_result_tostring(lua_State* L);
    int lua_mysql_result_field_count(lua_State* L);
    int lua_mysql_result_rows_next(lua_State* L);
    int lua_mysql_result_rows(lua_State* L);
    int lua_mysql_result_table(lua_State* L);
}

void luaclose_luamysql(void)
{
}

int luaopen_luamysql(lua_State* L)
{
    static const luaL_Reg lib[]=
    {
        {"open",libmysql::lua_mysql_open},
        {"close",libmysql::lua_mysql_close},
	// connection functions
        {"escape",libmysql::lua_mysql_escape},
	{"affected_rows",libmysql::lua_mysql_affected_rows},
	{"insert_id",libmysql::lua_mysql_insert_id},
        {"query",libmysql::lua_mysql_query},
	// rowset functions
        {"fields_count",libmysql::lua_mysql_result_field_count},
        {"rows",libmysql::lua_mysql_result_rows},
        {"table",libmysql::lua_mysql_result_table},
        {0,0}
    };

    static const luaL_Reg slib[]=
    {
        {"__gc",libmysql::lua_mysql_gc},
        {"__tostring",libmysql::lua_mysql_tostring},
        {"close",libmysql::lua_mysql_close},
        {"escape",libmysql::lua_mysql_escape},
	{"affected_rows",libmysql::lua_mysql_affected_rows},
	{"insert_id",libmysql::lua_mysql_insert_id},
        {"query",libmysql::lua_mysql_query},
        {0,0}
    };

    static const luaL_Reg rlib[]=
    {
        {"__gc",libmysql::lua_mysql_result_gc},
        {"__tostring",libmysql::lua_mysql_result_tostring},
        {"close",libmysql::lua_mysql_result_close},
        {"fields_count",libmysql::lua_mysql_result_field_count},
        {"rows",libmysql::lua_mysql_result_rows},
        {"table",libmysql::lua_mysql_result_table},
        {0,0}
    };

    luaL_newmetatable(L,libmysql::LUA_MYSQL);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");
    luaL_register(L,0,slib);

    luaL_newmetatable(L,libmysql::LUA_MYSQL_RESULT);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");
    luaL_register(L,0,rlib);


    luaL_register(L,"mysql",lib);

#ifdef LUA_MODULE
    atexit(luaclose_luamysql);
#endif /*LUA_MODULE*/

    return 0;
}


int libmysql::lua_mysql_open(lua_State* L)
{
    if(lua_gettop(L)!=2)
        return luaL_error(L,"\"mysql.open\" bad params");

    const char* condata=luaL_checkstring(L,1);
    const char* schema=luaL_checkstring(L,2);

    LMYSQL* mysql=(LMYSQL*)lua_newuserdata(L,sizeof(LMYSQL));

    luaL_getmetatable(L,LUA_MYSQL);
    lua_setmetatable(L,-2);

    mysql->online=0;
    
    if(!mysql_init(&mysql->svc))
	return luaL_error(L,"mysql_init");

    char temp[1024];
    int n=snprintf(temp,sizeof(temp),"%s",condata);
    if(n<0 || n>=sizeof(temp))
	temp[sizeof(temp)-1]=0;
    
    static char null[]="";
        
    char* user=temp;
    while(*user && *user==' ') user++;

    char* host=strchr(user,'@');
    if(!host) { host=user; user=null; } else { *host=0; host++; }
    
    char* pass=strchr(user,':');
    if(!pass) pass=null; else { *pass=0; pass++; }
    
    char* port=strchr(host,':');
    if(!port) port=null; else { *port=0; port++; }
    
    int _port=atoi(port);
    if(_port<0) _port=0;

    char* unix_socket=null;
    if(*host=='/') { unix_socket=host; host=null; _port=0; }
    
    if(!mysql_real_connect(&mysql->svc,(*host)?host:0,(*user)?user:0,(*pass)?pass:0,schema,_port,(*unix_socket)?unix_socket:0,0))
    {
	lua_pushstring(L,mysql_error(&mysql->svc));
	mysql_close(&mysql->svc);
	return lua_error(L);
    }
    
    mysql->online=1;

    return 1;
}

int libmysql::lua_mysql_close(lua_State* L)
{
    LMYSQL* mysql=(LMYSQL*)luaL_checkudata(L,1,LUA_MYSQL);

    if(mysql->online)
    {
	mysql_close(&mysql->svc);
	mysql->online=0;
    }
		
    return 0;
}

int libmysql::lua_mysql_gc(lua_State* L)
{
    return lua_mysql_close(L);
}

int libmysql::lua_mysql_tostring(lua_State* L)
{
    LMYSQL* mysql=(LMYSQL*)luaL_checkudata(L,1,LUA_MYSQL);

    lua_pushstring(L,mysql->online?"opened":"closed");

    return 1;
}

int libmysql::lua_mysql_escape(lua_State* L)
{
    LMYSQL* mysql=(LMYSQL*)luaL_checkudata(L,1,LUA_MYSQL);
    
    size_t length=0;

    const char* from=luaL_checklstring(L,2,&length);	    
    
    if(!mysql->online)
    {
	lua_pushstring(L,"");
	return 1;
    }

    membuf buf(length*2+1);

    length=mysql_real_escape_string(&mysql->svc,buf.ptr,from,length);

    lua_pushlstring(L,buf.ptr,length);
    return 1;
}

int libmysql::lua_mysql_affected_rows(lua_State* L)
{
    LMYSQL* mysql=(LMYSQL*)luaL_checkudata(L,1,LUA_MYSQL);

    lua_pushinteger(L,mysql->online?mysql_affected_rows(&mysql->svc):0);

    return 1;
}    

int libmysql::lua_mysql_insert_id(lua_State* L)
{
    LMYSQL* mysql=(LMYSQL*)luaL_checkudata(L,1,LUA_MYSQL);

    lua_pushinteger(L,mysql->online?mysql_insert_id(&mysql->svc):0);

    return 1;
}

int libmysql::lua_mysql_query(lua_State* L)
{
    LMYSQL* mysql=(LMYSQL*)luaL_checkudata(L,1,LUA_MYSQL);

    size_t len=0;

    const char* stmt=luaL_checklstring(L,2,&len);

    if(!mysql->online)
	return 0;

    if(mysql_real_query(&mysql->svc,stmt,len))
    {
	lua_pushstring(L,mysql_error(&mysql->svc));
	return lua_error(L);
    }

    MYSQL_RES* result=mysql_store_result(&mysql->svc);
    
    if(!result)
	return 0; 
    
    int fnum=mysql_field_count(&mysql->svc);

    if(!fnum)
    {
	mysql_free_result(result);
	return 0;
    }


    LMYSQL_RESULT* mysql_result=(LMYSQL_RESULT*)lua_newuserdata(L,sizeof(LMYSQL_RESULT));
    
    luaL_getmetatable(L,LUA_MYSQL_RESULT);
    lua_setmetatable(L,-2);

    mysql_result->result=result;
    mysql_result->fnum=fnum;

    return 1;
}


int libmysql::lua_mysql_result_close(lua_State* L)
{
    LMYSQL_RESULT* mysql_result=(LMYSQL_RESULT*)luaL_checkudata(L,1,LUA_MYSQL_RESULT);

    if(mysql_result->result)
    {
	mysql_free_result(mysql_result->result);
	mysql_result->result=0;
    }

    return 0;
}

int libmysql::lua_mysql_result_gc(lua_State* L)
{
    return lua_mysql_result_close(L);
}

int libmysql::lua_mysql_result_tostring(lua_State* L)
{
    LMYSQL_RESULT* mysql_result=(LMYSQL_RESULT*)luaL_checkudata(L,1,LUA_MYSQL_RESULT);

    char tmp[32]="(nil)";
    
    if(mysql_result->result)
	sprintf(tmp,"%i",mysql_result->fnum);

    lua_pushstring(L,tmp);

    return 1;
}

int libmysql::lua_mysql_result_field_count(lua_State* L)
{
    LMYSQL_RESULT* mysql_result=(LMYSQL_RESULT*)luaL_checkudata(L,1,LUA_MYSQL_RESULT);

    lua_pushinteger(L,mysql_result->result?mysql_result->fnum:0);

    return 1;
}


int libmysql::lua_mysql_result_rows_next(lua_State* L)
{
    LMYSQL_RESULT* mysql_result=(LMYSQL_RESULT*)luaL_checkudata(L,lua_upvalueindex(1),LUA_MYSQL_RESULT);

    if(!mysql_result->result)
	return 0;

    MYSQL_ROW row=mysql_fetch_row(mysql_result->result);
    
    if(!row)
	return 0;

    lua_newtable(L);
    
    for(int i=0;i<mysql_result->fnum;i++)
    {
	const char* p=row[i];
	lua_pushstring(L,p?p:"");
	lua_rawseti(L,-2,i+1);
    }
    
    return 1;
}

int libmysql::lua_mysql_result_rows(lua_State* L)
{
    LMYSQL_RESULT* mysql_result=(LMYSQL_RESULT*)luaL_checkudata(L,1,LUA_MYSQL_RESULT);
    
    if(!mysql_result->result)
	return 0;

    mysql_data_seek(mysql_result->result,0);

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,lua_mysql_result_rows_next,1);

    return 1;
}


int libmysql::lua_mysql_result_table(lua_State* L)
{
    LMYSQL_RESULT* mysql_result=(LMYSQL_RESULT*)luaL_checkudata(L,1,LUA_MYSQL_RESULT);

    if(!mysql_result->result)
	return 0;

    MYSQL_ROW row;
    
    lua_newtable(L);
    int j=1;

    mysql_data_seek(mysql_result->result,0);

    while((row=mysql_fetch_row(mysql_result->result)))
    {
	lua_newtable(L);

	for(int i=0;i<mysql_result->fnum;i++)
	{
	    const char* p=row[i];
	    lua_pushstring(L,p?p:"");
	    lua_rawseti(L,-2,i+1);
	}

	lua_rawseti(L,-2,j++);
    }
    
    
    return 1;
}
