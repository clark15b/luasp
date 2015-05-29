t1=
{

    t2=
    {
	name='hello'
    }
}

var1='hello world'

function t1.func1(a,b) return a+b,2,3 end



my_mt={}

function my_mt.f(tab,s)
    print(tab.value..' '..(s or ''))
end

my_mt.__index=my_mt
    
obj=setmetatable({value='hello world'},my_mt)
obj.value='hi'
--obj:f('msg')

function func2(a) return a end

