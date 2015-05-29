require('luamysql')

db=mysql.open('user:pass@localhost','dbase')

res=db:query('select * from MY_TABLE')

for row in res:rows() do
    print(unpack(row))
end

-- print(res:table()[3][1])
