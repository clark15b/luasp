require('luajson')

-- json.no_unicode_escape(0)

t=
{
    f1='\nПривет123',
    f2=5.5,
    f3={1,2,3},
}


print(json.encode(t))

print(json.encode(json.decode('1, {\"f1\" : [1,-2.5,true,\"Hello\", [1]], \"f2\":\"hello\", \"f3\":-.5, \"f4\":true, \"f5\":{\"f\":555} },2')))

