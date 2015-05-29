#ifndef __LUAMYSQL_H
#define __LUAMYSQL_H


#ifdef __cplusplus 
extern "C" {
#endif /*__cplusplus*/ 

#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>  

int luaopen_luamysql(lua_State* L);

#ifndef LUA_MODULE
void luaclose_luamysql(void);
#endif /*LUA_MODULE*/


#ifdef __cplusplus 
}
#endif /*__cplusplus*/ 


#endif /*__LUAMYSQL_H*/
