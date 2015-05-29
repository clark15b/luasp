require('luaora')

db=oracle.open('user:pass@dbase')

stmt=db:query('select * from MY_TABLE')


for row in stmt:rows() do
    print(unpack(row))
end
