require('luaora')

db=oracle.open('user:pass@dbase')

stmt=db:open('select * from MY_TABLE where SERIAL>:param1')

stmt:bind(':param1','number')

stmt:at(1,498)

stmt:execute()

for row in stmt:rows() do
    print(unpack(row))
end
    
print(stmt:table()[1][1])

print('tmpl: '..stmt:at(1))
