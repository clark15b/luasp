require('luamcache')

cache=mcache.open('127.0.0.1:11111')
--cache=mcache.open('/var/tmp/memcached')

--cache:set('my_key','Hello world',10)

cache:set('my_key','Hello world')

print(cache:get('my_key'))

--cache:del('my_key')
