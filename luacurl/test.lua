require('luacurl')

--curl.log_file('./curl.log')
--curl.timeout(15)
--curl.proxy('192.168.6.2:8080')

--[[
req=curl.open('POST','http://yandex.ru/yandsearch')
req:set_request_header('Content-Type','application/x-form-urlencoded')
content=req:send('text='..req:escape('k4vv'))
]]

req=curl.open('GET','http://yandex.ru/yandsearch?text='..curl.escape('k4vv'))

content=req:send()

if req:status()==200 then
    print(content)
else
    print('ERR: '..req:status())
end
