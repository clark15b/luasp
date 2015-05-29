require('luaora')

db=oracle.open('user:pass@dbase')

stmt=db:open('select * from MY_TABLE')

stmt:execute()

for row in stmt:rows() do
    print(unpack(row))
end
