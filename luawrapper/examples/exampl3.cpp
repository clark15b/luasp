#include "luawrapper.h"
#include <stdio.h>

int main(void)
{
    try
    {
	lua::vm vm;

	// initialize	
	vm.initialize();
	
	// load and execute Lua script
	vm.load_file("exampl3.lua");
	
	
	// read comments in ../test.cpp

	std::map<std::string,std::string> arg1;
	std::vector<int> arg2;
	
	arg1["key1"]="val1";
	arg1["key2"]="val2";
	arg1["key3"]="val3";

	for(int i=0;i<10;i++)	
	    arg2.push_back(i*10);
	
	std::map<std::string,std::string> result1;
	std::vector<std::string> result2;
	std::string result3;

	lua::transaction(vm)<<lua::lookup("example")<<arg1<<arg2<<"hello"<<5<<6.7<<lua::invoke>>result1>>result2>>result3<<lua::end;

	printf("\nC++ part:\n");

	printf("result1..\n");
	for(std::map<std::string,std::string>::const_iterator i=result1.begin();i!=result1.end();++i)
	    printf("%s=\"%s\"\n",i->first.c_str(),i->second.c_str());

	printf("result2..\n");
	for(int i=0;i<result2.size();++i)
	    printf("%i=\"%s\"\n",i,result2[i].c_str());
	
	printf("result3=\"%s\"\n",result3.c_str());
	
	
	lua::transaction(vm)<<lua::lookup("example2")<<lua::invoke<<lua::end;
	
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
