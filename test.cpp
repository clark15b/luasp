#include "llsplib.h"
#include <stdio.h>


int main(int argc,char** argv)
{
    if(argc<2)
	return 0;

    lua_State* L=lua_open();

    luaL_openlibs(L);

    luaopen_lualsp(L);
    
    if(luaL_do_lsp_file(L,argv[1]))
    {
	const char* e=lua_tostring(L,-1);
	
	printf("%s\n",e);
	
        lua_pop(L,1);
    }
    
    lua_close(L);

    return 0;
}
