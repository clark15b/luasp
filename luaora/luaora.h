#ifndef __LUAORA_H
#define __LUAORA_H


#ifdef __cplusplus 
extern "C" {
#endif /*__cplusplus*/ 

#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>  

int luaopen_luaora(lua_State* L);

#ifndef LUA_MODULE
void luaclose_luaora(void);
#endif /*LUA_MODULE*/


#ifdef __cplusplus 
}
#endif /*__cplusplus*/ 


#endif /*__LUAORA_H*/
