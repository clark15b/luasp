#ifndef __LUACURL_H
#define __LUACURL_H


#ifdef __cplusplus 
extern "C" {
#endif /*__cplusplus*/ 

#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>  

int luaopen_luacurl(lua_State* L);

#ifndef LUA_MODULE
void luaclose_luacurl(void);
#endif /*LUA_MODULE*/


#ifdef __cplusplus 
}
#endif /*__cplusplus*/ 


#endif /*__LUACURL_H*/
