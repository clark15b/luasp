#ifndef __LUAMCACHE_H
#define __LUAMCACHE_H


#ifdef __cplusplus 
extern "C" {
#endif /*__cplusplus*/ 

#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>  

int luaopen_luamcache(lua_State* L);

#ifndef LUA_MODULE
void luaclose_luamcache(void);
#endif /*LUA_MODULE*/


#ifdef __cplusplus 
}
#endif /*__cplusplus*/ 


#endif /*__LUAMCACHE_H*/
