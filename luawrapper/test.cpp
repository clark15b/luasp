#include "luawrapper.h"
#include <stdio.h>

int main(void)
{
    try
    {
	lua::vm vm;
	
	// initialize
	vm.initialize();
	
	// load and execute file
	vm.load_file("test.lua");
	
	int a=0,b=0,c=0;
	
	// call function: "a,b,c=t1.func1(5,6)"
	lua::transaction(vm)<<lua::lookup("t1")<<lua::lookup("func1")<<5<<6<<lua::invoke>>a>>b>>c<<lua::end;
	printf("%i,%i,%i\n",a,b,c);

	// call function as method: "obj:f('msg')"
	lua::transaction(vm)<<lua::lookup("obj")<<lua::lookup("f")<<"msg"<<lua::m_invoke<<lua::end;

	{
	    std::string s;

	    lua::transaction bind(vm);
	    bind.lookup("t1");
	    bind.lookup("t2");
	    lua::table t2=bind.table();
	    
	    // "t1.t2.name=5"
	    t2.update("name",5);
	    
	    // "print(t1.t2.name)"
	    t2.query("name",s);
	    printf("%s\n",s.c_str());

	    // end transaction!!!
	    bind.end();
	}


	{
	    std::string s;

	    // "print('_G.var1')"
	    lua::table g(vm);
	    g.query("var1",s);
	    printf("%s\n",s.c_str());
	}


	// "m2=func2( {n1='v1',n2='v2',n3='v3'} )"
	std::map<std::string,std::string> m1,m2;
	m1["n1"]="v1";
	m1["n2"]="v2";
	m1["n3"]="v3";
	
	lua::transaction(vm)<<lua::lookup("func2")<<m1<<lua::invoke>>m2<<lua::end;
	
	for(std::map<std::string,std::string>::iterator i=m2.begin();i!=m2.end();++i)
	    printf("%s=%s\n",i->first.c_str(),i->second.c_str());


	// "v2=func2( {[0]='aaa',[1]='bbb'} )"
	std::vector<std::string> v1,v2;
	v1.push_back("aaa");
	v1.push_back("bbb");
	
	lua::transaction(vm)<<lua::lookup("func2")<<v1<<lua::invoke>>v2<<lua::end;
	
	for(int i=0;i<v2.size();++i)
	    printf("%i=%s\n",i,v2[i].c_str());

	
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
