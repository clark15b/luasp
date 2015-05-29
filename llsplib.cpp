
/*  
 * Copyright (C) Anton Burdinuk 
 */
  

#include "llsplib.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef CUT_CR
#define CUT_CR
#endif /*CUT_CR*/

namespace llsplib
{
    enum { use_cache=0 };				// 0 - not use cache future
							// 1 - use cache file if exist (no mtime different check)
							// 2 - build cache file if not exist or invalid

    enum { buf_reserve=64 };
    enum { max_buf_len=1024-buf_reserve };

    enum
    {
	st_unknown=0,
	st_echo_chars1,
	st_echo_chars2,
	st_echo_chars3,
	st_echo_chars4,
	st_stmt1,
	st_stmt2,
	st_stmt3,
	st_stmt12,
	st_stmt13,
	st_comment1,
	st_comment2,
#ifdef CUT_CR
	st_cr1,
	st_cr2
#else
	st_cr1=st_echo_chars1
#endif
    };

    struct lsp_State
    {
	FILE* fp;					// input file
	int st;						// current state
	int line;					// file line number

	unsigned char buf[max_buf_len+buf_reserve];	// output buffer
	int buf_offset;

	void add_char(int ch);
	void add_beg_echo(void);
	void add_end_echo(void);
	void add_beg_s_echo(void);
	void add_end_s_echo(void);
	
	int open(const char* filename);
	int close(void);
    };

    
    enum { chtype_len=256 };

    static const unsigned char chtype[chtype_len]=
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x61, 0x62, 0x74, 0x6e, 0x76, 0x66, 0x72, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };


    int luaL_lsp_error(lua_State* L);

    int lua_include(lua_State *L);

    int lua_echo(lua_State *L);
    int lua_print(lua_State *L);
    
    const char* lsp_reader(lua_State* L,void* ud,size_t* size);

    int lsp_writer(lua_State* L,const void* b,size_t size,void* B);


    const char lsp_io_type[]="lio";

    int lsp_def_puts(void* ctx,const char* s) { return fputs(s,(FILE*)ctx); }

    int lsp_def_putc(void* ctx,int c) { return fputc(c,(FILE*)ctx); }

    int lsp_def_write(void* ctx,const char* s,size_t len) { return fwrite(s,1,len,(FILE*)ctx); }
    
#ifndef THREAD_SAFE
    lsp_io lio={lctx:stdout, lputs:lsp_def_puts, lputc:lsp_def_putc, lwrite:lsp_def_write};
#endif
}

int luaL_lsp_set_io(lua_State *L,lsp_io *io)
{
#ifndef THREAD_SAFE
    memcpy((char*)&llsplib::lio,(char*)io,sizeof(lsp_io));
#else
    lua_getfield(L,LUA_REGISTRYINDEX,llsplib::lsp_io_type);
    lsp_io* _io=(lsp_io*)lua_touserdata(L,-1);

    memcpy((char*)_io,(char*)io,sizeof(lsp_io));

    lua_pop(L,1);
#endif

    return 0;
}

void* luaL_lsp_get_io_ctx(lua_State *L)
{
#ifndef THREAD_SAFE
    return llsplib::lio.lctx;
#else
    lua_getfield(L,LUA_REGISTRYINDEX,llsplib::lsp_io_type);

    lsp_io* io=(lsp_io*)lua_touserdata(L,-1);

    void* ctx=io->lctx;

    lua_pop(L,1);
    
    return ctx;
#endif
}


int llsplib::luaL_lsp_error(lua_State* L)
{
    lua_pushstring(L,"syntax error");

    return lua_error(L);
}

int llsplib::lsp_State::open(const char* filename)
{
    st=st_echo_chars1;
    buf_offset=0;
    line=1;

    fp=fopen(filename,"r");
    
    if(!fp)
	return -1;
    
    return 0;
}

int llsplib::lsp_State::close(void)
{
    if(fp)
    {
	fclose(fp);
	fp=0;
    }
	
    return 0;
}

void llsplib::lsp_State::add_char(int ch)
{
    unsigned char c=chtype[ch];

    switch(c)
    {
    case 0x00: buf[buf_offset++]=ch; break;
    case 0xff: buf[buf_offset++]='.'; break;
    default: buf[buf_offset++]='\\'; buf[buf_offset++]=c; break;
    }
}

void llsplib::lsp_State::add_beg_echo(void)
{
    static const char echo_beg_tag[]="echo('";

    strncpy((char*)buf+buf_offset,echo_beg_tag,sizeof(echo_beg_tag)-1);
    buf_offset+=(sizeof(echo_beg_tag)-1);
}
void llsplib::lsp_State::add_end_echo(void)
{
    static const char echo_end_tag[]="') ";

    strncpy((char*)buf+buf_offset,echo_end_tag,sizeof(echo_end_tag)-1);
    buf_offset+=(sizeof(echo_end_tag)-1);
}
void llsplib::lsp_State::add_beg_s_echo(void)
{
    static const char s_echo_beg_tag[]="echo(";

    strncpy((char*)buf+buf_offset,s_echo_beg_tag,sizeof(s_echo_beg_tag)-1);
    buf_offset+=(sizeof(s_echo_beg_tag)-1);
}
void llsplib::lsp_State::add_end_s_echo(void)
{
    static const char s_echo_end_tag[]=") ";

    strncpy((char*)buf+buf_offset,s_echo_end_tag,sizeof(s_echo_end_tag)-1);
    buf_offset+=(sizeof(s_echo_end_tag)-1);
}

const char* llsplib::lsp_reader(lua_State* L,void* ud,size_t* size)
{
    lsp_State* lst=(lsp_State*)ud;
    
    while(lst->buf_offset<max_buf_len)
    {    
	int ch=fgetc(lst->fp);
	
	if(ch=='\n')
	    lst->line++;

	if(ch==EOF)
	{
	    switch(lst->st)
	    {
	    case st_unknown:
	    case st_echo_chars1:
#ifdef CUT_CR
	    case st_cr1:
	    case st_cr2:
#endif
		break;
	    case st_echo_chars2:
		lst->add_end_echo();
		break;
	    default:
		luaL_lsp_error(L);
		break;
	    }
	    lst->st=st_unknown;
	    break;
	}
	    
	switch(lst->st)
	{
	case st_echo_chars1:
	    if(ch=='<')
		lst->st=st_echo_chars4;
	    else
	    {
		lst->add_beg_echo();
		lst->add_char(ch);
		lst->st=st_echo_chars2;
	    }
	    break;
	case st_echo_chars2:
	    if(ch=='<')
		lst->st=st_echo_chars3;
	    else
		lst->add_char(ch);
	    break;
	case st_echo_chars3:
	    if(ch!='?')
	    {
		lst->buf[lst->buf_offset++]='<';
		lst->add_char(ch);
		lst->st=st_echo_chars2;
	    }else
	    {
		lst->add_end_echo();
		lst->st=st_stmt1;
	    }
	    break;
	case st_echo_chars4:
	    if(ch=='?')
		lst->st=st_stmt1;
	    else
	    {
		lst->add_beg_echo();
		lst->buf[lst->buf_offset++]='<';
		lst->add_char(ch);
		lst->st=st_echo_chars2;
	    }
	    break;
	case st_stmt1:
	    if(ch=='=')
	    {
		lst->add_beg_s_echo();
		lst->st=st_stmt2;
	    }else if(ch=='#')
		lst->st=st_comment1;
	    else
	    {
		lst->buf[lst->buf_offset++]=ch;
		lst->st=st_stmt12;
	    }
	    break;
	case st_stmt2:
	    if(ch=='?')
		lst->st=st_stmt3;
	    else
		lst->buf[lst->buf_offset++]=ch;
	    break;
	case st_stmt3:
	    if(ch=='>')
	    {
		lst->add_end_s_echo();
		lst->st=st_cr1;
	    }else if(ch=='?')
		lst->buf[lst->buf_offset++]='?';
	    else
	    {
		lst->buf[lst->buf_offset++]='?';
		lst->buf[lst->buf_offset++]=ch;
		lst->st=st_stmt2;
	    }
	    break;
	case st_stmt12:
	    if(ch=='?')
		lst->st=st_stmt13;
	    else
		lst->buf[lst->buf_offset++]=ch;
	    break;
	case st_stmt13:
	    if(ch=='>')
	    {
		lst->buf[lst->buf_offset++]=' ';
		lst->st=st_cr1;
	    }else if(ch=='?')
		lst->buf[lst->buf_offset++]='?';
	    else
	    {
		lst->buf[lst->buf_offset++]='?';
		lst->buf[lst->buf_offset++]=ch;
		lst->st=st_stmt12;
	    }
	    break;
	case st_comment1:
	    if(ch=='?')
		lst->st=st_comment2;
	    break;
	case st_comment2:
	    if(ch=='>')
		lst->st=st_cr1;
	    else if(ch!='?')
		lst->st=st_comment1;
	    break;
#ifdef CUT_CR
	case st_cr1:
	    if(ch=='\r')
		lst->st=st_cr2;
	    else
	    {
		if(ch!='\n')
		    ungetc(ch,lst->fp);
		lst->st=st_echo_chars1;
	    }
	    break;
	case st_cr2:
	    if(ch!='\n')
		ungetc(ch,lst->fp);
	    lst->st=st_echo_chars1;
	    break;
#endif
	}
    }
    
    *size=lst->buf_offset;
    lst->buf_offset=0;

    return (const char*)lst->buf;
}



int luaL_load_lsp_file(lua_State* L,const char* filename)
{
    llsplib::lsp_State ctx;
    
    if(ctx.open(filename))
    {
	lua_pushfstring(L,"cannot open %s: %s",filename,strerror(errno));
	return LUA_ERRERR;
    }

    int status=lua_load(L,llsplib::lsp_reader,&ctx,filename);

    int line=ctx.line;
    
    ctx.close();

    if(status)
    {
	char tmp[512];

	int n=snprintf(tmp,sizeof(tmp),"%s:%i: %s",filename,line,lua_tostring(L,-1));

	if(n<0 || n>=sizeof(tmp))
	    n=sizeof(tmp)-1;

        lua_pop(L,1);
	
	lua_pushlstring(L,tmp,n);
    }    

    return status;
}

int llsplib::lsp_writer(lua_State* L,const void* b,size_t size,void* B)
{
    if(write(*((int*)B),(char*)b,size)!=size)
	return -1;
    return 0;
}      

int luaL_do_lsp_file(lua_State* L,const char* filename)
{
    int status;

    if(!llsplib::use_cache)
    {
	if((status=luaL_load_lsp_file(L,filename)) || (status=lua_pcall(L,0,0,0)))
	    return status;
		
	return 0;
    }

    char cachefile[512];

    int n=snprintf(cachefile,sizeof(cachefile),"%s.luac",filename);
    if(n<0 || n>=0)
	cachefile[sizeof(cachefile)-1]=0;

    time_t mtime1=0;
    time_t mtime2=0;
    
    {
	struct stat st;
	if(!lstat(filename,&st))
	    mtime1=st.st_mtime;
	if(!lstat(cachefile,&st))
	    mtime2=st.st_mtime;
    }
    
    if(llsplib::use_cache==1)
    {
	if(mtime2)
	{
	    if((status=luaL_loadfile(L,cachefile)) || (status=lua_pcall(L,0,0,0)))
		return status;
	}else
	{
	    if((status=luaL_load_lsp_file(L,filename)) || (status=lua_pcall(L,0,0,0)))
		return status;
	}
	return 0;
    }


    if(!mtime2 || mtime2<mtime1)
    {

	if((status=luaL_load_lsp_file(L,filename)))
	    return status;
	    
        char tmp[]="/tmp/lsp_cache.XXXXXX";

	int fd=mkstemp(tmp);
	if(fd!=-1)
	{
	    status=lua_dump(L,llsplib::lsp_writer,&fd);
	    close(fd);
	    
	    if(!status)
		rename(tmp,cachefile);
	}

	if((status=lua_pcall(L,0,0,0)))
	    return status;

	return 0;
    }

    if((status=luaL_loadfile(L,cachefile)) || (status=lua_pcall(L,0,0,0)))
	return status;

    return 0;
}


int llsplib::lua_include(lua_State *L)
{
    const char* filename=luaL_checkstring(L,1);

    if(luaL_do_lsp_file(L,filename))
	lua_error(L);

    return 0;
}


int llsplib::lua_echo(lua_State *L)
{
    int n=lua_gettop(L);

#ifndef THREAD_SAFE
    lsp_io* io=&lio;
#else
    lua_getfield(L,LUA_REGISTRYINDEX,lsp_io_type);
    lsp_io* io=(lsp_io*)lua_touserdata(L,-1);
#endif
    
    int i;

    for(i=1;i<=n;i++)
    {
	size_t len=0;

	const char* s=lua_tolstring(L,i,&len);


	size_t ll=0;
	
	while(ll<len)
	{
	    size_t n=io->lwrite(io->lctx,s+ll,len-ll);
	    if(!n)
		break;
	    ll+=n;
	}

	if(i<n)
	    io->lputc(io->lctx,' ');
    }

#ifdef THREAD_SAFE
    lua_pop(L,1);	// userdata
#endif

    return 0;
}

int llsplib::lua_print(lua_State *L)
{
    int n=lua_gettop(L);
    
#ifndef THREAD_SAFE
    lsp_io* io=&lio;
#else
    lua_getfield(L,LUA_REGISTRYINDEX,lsp_io_type);
    lsp_io* io=(lsp_io*)lua_touserdata(L,-1);
#endif
    
    int i;
    lua_getglobal(L,"tostring");

    for(i=1;i<=n;i++)
    {
	const char *s;
	lua_pushvalue(L,-1);
	lua_pushvalue(L,i);
	lua_call(L,1,1);
	
	size_t len=0;
	
	s=lua_tolstring(L,-1,&len);

	if(!s)
	    return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));


	size_t ll=0;
	
	while(ll<len)
	{
	    size_t n=io->lwrite(io->lctx,s+ll,len-ll);
	    if(!n)
		break;
	    ll+=n;
	}

	if(i<n)
	    io->lputc(io->lctx,'\t');

	lua_pop(L,1);
    }

    io->lputc(io->lctx,'\n');

#ifndef THREAD_SAFE
    lua_pop(L,1);	// tostring
#else
    lua_pop(L,2);	// userdata, tostring
#endif

    return 0;
}

int luaopen_lualspaux(lua_State *L);

int luaopen_lualsp(lua_State *L)
{
#ifdef THREAD_SAFE
    lsp_io* io=(lsp_io*)lua_newuserdata(L,sizeof(lsp_io));
    memset((char*)io,0,sizeof(lsp_io));

    io->lctx=(void*)stdout;
    io->lputs=llsplib::lsp_def_puts;
    io->lputc=llsplib::lsp_def_putc;
    io->lwrite=llsplib::lsp_def_write;

    lua_setfield(L,LUA_REGISTRYINDEX,llsplib::lsp_io_type);
#endif
    
    lua_register(L,"dotmpl",llsplib::lua_include);
    lua_register(L,"dofile_lsp",llsplib::lua_include);
    lua_register(L,"echo",llsplib::lua_echo);
    lua_register(L,"write",llsplib::lua_echo);
    lua_register(L,"print",llsplib::lua_print);
    
    luaopen_lualspaux(L);
    
    return 0;
}


