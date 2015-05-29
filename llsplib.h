
/*  
 * Copyright (C) Anton Burdinuk 
 */


#ifndef __LLSPLIB_H
#define __LLSPLIB_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include <lua.h> 
#include <lauxlib.h>
#include <lualib.h>

/*
    implement:
	- content_type
	- log
	- set_out_header
	- get_in_header
	- uuid_gen

    call first:
	- luaL_lsp_set_io
    
    fill env:
	- luaL_lsp_setargs
	- luaL_lsp_setargs_post
	- luaL_lsp_setenv
	- luaL_lsp_session_init
    
    lsp code may return HTTP code as return value! use 'lua_pcall(...,LUA_MULTRET,...) and lua_pop(...,lua_gettop())'
*/


typedef int (*lsp_puts_t) (void* ctx,const char* s);
typedef int (*lsp_putc_t) (void* ctx,int c);
typedef int (*lsp_write_t) (void* ctx,const char* s,size_t len);

struct lsp_io
{
    void* lctx;
    lsp_puts_t lputs;
    lsp_putc_t lputc;
    lsp_write_t lwrite;
};


int luaopen_lualsp(lua_State *L);

int luaL_load_lsp_file(lua_State* L,const char* filename);

int luaL_do_lsp_file(lua_State* L,const char* filename);

int luaL_lsp_set_io(lua_State *L,lsp_io *io);
void* luaL_lsp_get_io_ctx(lua_State *L);
int luaL_lsp_setargs(lua_State *L,const char* p,size_t len);
int luaL_lsp_chdir_to_file(lua_State *L,const char* path);
int luaL_lsp_chdir_restore(lua_State *L);
int luaL_lsp_session_init(lua_State *L,const char* cookie_name,int days,const char* cookie_path);
int luaL_lsp_setenv(lua_State *L,const char* name,const char* value);
int luaL_lsp_setenv_len(lua_State *L,const char* name,const char* value,size_t len);
int luaL_lsp_setenv_buf(lua_State *L,const char* name,luaL_Buffer* buf);
int luaL_lsp_setfield(lua_State *L,const char* name,const char* value);	/* lua_getfield(L,LUA_GLOBALSINDEX,...); luaL_lsp_setfield(..);  lua_pop(L,1) */


#ifdef __cplusplus
}
#endif /*__cplusplus*/


#endif /*__LLSPLIB_H*/
