function example(arg1,arg2,arg3,arg4,arg5)
    print("LUA part:")

    print("arg1..")
    for i,j in pairs(arg1) do
	print(i,j)
    end

    print("arg2..")
    for i,j in pairs(arg2) do
	print(i,j)
    end
    
    print("arg3="..arg3,"arg4="..arg4,"arg5="..arg5)

    return {key1="val1"}, {[0]="val1","val2",3}, "Hello from LUA"
end


function example2()
    print(string.format("\n%s %s","hello","w o r l d").."\n")
end
