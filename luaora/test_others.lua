require('luaora')

db=oracle.open('user:pass@dbase')

--db:query('insert into MY_TABLE(COLUMN1,COLUMN2) values(1,\'test\')')
--db:query('update MY_TABLE set COLUMN2=\'zzz\' where COLUMN1=1')
--db:query('delete from MY_TABLE')

stmt=db:open('insert into MY_TABLE(COLUMN1,COLUMN2,COLUMN3) values(:i,\'test\',:d)')

stmt:bind(':d','date')
stmt:bind(':i')

stmt:at(1,'2005-01-02 03:04:05')
stmt:at(2,5)

stmt:execute()
db:commit()

stmt:at(2,6)
stmt:execute()

db:rollback()

