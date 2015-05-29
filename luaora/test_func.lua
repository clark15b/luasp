require('luaora')

db=oracle.open('user:pass@dbase')

stmt=db:open('BEGIN :result:=my_function(:param); END;')

stmt:bind(':result')
stmt:bind(':param','number')

stmt:at(2,10)

stmt:execute()

print(stmt:at(1))

stmt:at(2,11)

stmt:execute()

print(stmt:at(1))
