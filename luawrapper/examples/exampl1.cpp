#include "luawrapper.h"
#include <stdio.h>

int sum(lua_State* L)
{
    lua::stack st(L);
    
    double a=0,b=0;
    
    st.at(1,a);			// begin from 1
    st.at(2,b);
    
    st.push(a+b);
    
    return 1;			// number of return values (st.push)
}


int main(void)
{
    try
    {
	lua::vm vm;
	
	// initialize
	vm.initialize();
	
	// register CFunction
	vm.reg("sum",sum);

	// register package 'lib'
	static const luaL_Reg lib[]=
	{
	    {"sum",sum},
	    {0,0}
	};	
	vm.reg("lib",lib);
	
	// execute Lua statements
	vm.eval("print(\"5+6=\"..sum(5,6))");
	vm.eval("print(\"3+4=\"..lib.sum(3,4))");	
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
