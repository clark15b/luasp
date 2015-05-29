#ifndef __LUAJSON_H
#define __LUAJSON_H


#ifdef __cplusplus 
extern "C" {
#endif /*__cplusplus*/ 

#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>  

int luaopen_luajson(lua_State* L);

#ifndef LUA_MODULE
void luaclose_luajson(void);
#endif /*LUA_MODULE*/


#ifdef __cplusplus 
}
#endif /*__cplusplus*/ 


#endif /*__LUAJSON_H*/
