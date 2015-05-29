extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "md5.h"
#include <time.h>
#include <arpa/inet.h>
#include <string.h>

static void to_hex(const unsigned char* md,char* dst,int len)
{
   static const char t[]="0123456789abcdef";

    for(int i=0,j=0;i<len;i++,j+=2)
    {
        dst[j]=t[(md[i]>>4)&0x0f];
        dst[j+1]=t[md[i]&0x0f];
    }
}

static int from_hex(const char* src,unsigned char* md,int len)
{
    if(!len || len%2)
        return -1;

    unsigned char t[]=
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    for(int i=0,j=0;i<len;i+=2,j++)
    {
        unsigned char c1=t[src[i]];
        unsigned char c2=t[src[i+1]];

        if(c1==0xff || c2==0xff)
            return -1;

        md[j]=((c1<<4)&0xf0)|(c2&0x0f);
    }

    return 0;
}

static int lua_md5(lua_State* L)
{
    size_t l=0;

    const char* s=luaL_checklstring(L,1,&l);

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx,(unsigned char*)s,l);

    unsigned char md[16];
    MD5_Final(md,&ctx);

    char buf[sizeof(md)*2];
    to_hex(md,buf,sizeof(md));
 
    lua_pushlstring(L,buf,sizeof(buf));

    return 1;
}

/*
На вход берет:
IP адрес клиента для которого формируется пропуск - строка
Секретная последовательность известная только серверу - строка
Срок действия пропуска в часах - целое

На выходе:
Пропуск - строка длиной 40 символов, первые 8 которой содержат время окончания пропуска в секундах в шестнадцатеричном виде (big-endian)
*/

static int lua_moz_calc_pass(lua_State* L)
{
    size_t ip_len=0;
    const char* ip=luaL_checklstring(L,1,&ip_len);

    size_t secret_len=0;
    const char* secret=luaL_checklstring(L,2,&secret_len);

    int hours=luaL_checkint(L,3);

    u_int32_t exp_date=(u_int32_t)htonl(time(0)+hours*3600);         // 4 bytes!

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx,(unsigned char*)ip,ip_len);
    MD5_Update(&ctx,(unsigned char*)secret,secret_len);
    MD5_Update(&ctx,(unsigned char*)&exp_date,sizeof(exp_date));

    unsigned char md[sizeof(exp_date)+16];

    *((u_int32_t*)md)=exp_date;
    MD5_Final(md+sizeof(exp_date),&ctx);

    char buf[sizeof(md)*2];
    to_hex(md,buf,sizeof(md));

    lua_pushlstring(L,buf,sizeof(buf));

    return 1;
}

/*
На вход берет:
Пропуск полученный от клиента - строка
IP адрес клиента полученный от web-сервера - строка
Секретная последовательность известная только серверу - строка

На выходе:
1 - пропуск валидный
0 - пропуск невалидный
*/

static int lua_moz_validate_pass(lua_State* L)
{
    size_t pass_len=0;
    const char* pass=luaL_checklstring(L,1,&pass_len);

    size_t ip_len=0;
    const char* ip=luaL_checklstring(L,2,&ip_len);

    size_t secret_len=0;
    const char* secret=luaL_checklstring(L,3,&secret_len);

    int rc=0;

    if(pass_len==(sizeof(u_int32_t)+16)*2)
    {
        u_int32_t exp_date=0;
        
        if(!from_hex(pass,(unsigned char*)&exp_date,sizeof(u_int32_t)*2))
        {
            MD5_CTX ctx;
            MD5_Init(&ctx);
            MD5_Update(&ctx,(unsigned char*)ip,ip_len);
            MD5_Update(&ctx,(unsigned char*)secret,secret_len);
            MD5_Update(&ctx,(unsigned char*)&exp_date,sizeof(exp_date));

            unsigned char md[16];
            MD5_Final(md,&ctx);

            char buf[sizeof(md)*2];
            to_hex(md,buf,sizeof(md));

            if(!memcmp(buf,pass+sizeof(u_int32_t)*2,16*2))
            {
                exp_date=ntohl(exp_date);

#ifndef IGNORE_EXP_DATE                         // позволяет игнорировать срок действия пропуска
                if(time(0)<exp_date)
#endif
                    rc=1;
            }
        }
    }

    lua_pushinteger(L,rc);

    return 1;
}

extern "C" int luaopen_luamoz(lua_State* L)
{
    lua_register(L,"md5_hex",lua_md5);
    lua_register(L,"moz_calc_pass",lua_moz_calc_pass);
    lua_register(L,"moz_validate_pass",lua_moz_validate_pass);

    return 0;
}
