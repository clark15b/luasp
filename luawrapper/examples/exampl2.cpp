#include "luawrapper.h"
#include <stdio.h>

int main(void)
{
    try
    {
	lua::vm vm;
	
	// initialize
	vm.initialize();

	// "_G.variable1='Hello from C++'"
	lua::table g(vm);	
	g.update("variable1","Hello from C++");
	
	// load and execute Lua script
	vm.load_file("exampl2.lua");
	
	
	// show _G.variable2 value
	std::string s;
	g.query("variable2",s);
	
	printf("variable2=%s\n",s.c_str());
    }
    catch(const std::exception& e)
    {
	fprintf(stderr,"%s\n",e.what());
    }
    catch(...)
    {
	fprintf(stderr,"exception\n");
    }
    
    return 0;
}
