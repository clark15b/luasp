
/*  
 * Copyright (C) Anton Burdinuk 
 */
 

#include "luaora.h"
#include <oci.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

namespace libora
{
    OCIEnv* ora_env=0;
    OCIError* ora_err=0;

    static const char LUA_OCI_SVC[]="OCI_SVC*";
    static const char LUA_OCI_STMT[]="OCI_STMT*";
    static const char LUA_LOADLIB[]="_LOADLIB";


    void luaL_push_oci_value(lua_State* L,ub2 type,const char* p,ub4 len);

    struct OCI_PARAM_INFO
    {
        ub2 type;
        ub4 len;
        ub4 offset;

        void clear(void)
        {
            type=0;
            len=0;
            offset=0;
        }
    };

    struct OCI_ROW_DEFINE
    {
        ub4 pn;
        OCI_PARAM_INFO* pinfo;
        char* row;
        int row_len;

        void init(void)
        {
            pn=0;
            pinfo=0;
            row=0;
            row_len=0;
        }

        void copy(OCI_ROW_DEFINE& def)
        {
            pn=def.pn;
            pinfo=def.pinfo;
            row=def.row;
            row_len=def.row_len;
        }

        void clear(void)
        {
            pn=0;
            if(pinfo) { free(pinfo); pinfo=0; }
            if(row) { free(row); row=0; }
            row_len=0;
        }

        void luaL_push(lua_State* L,int i)
        {
            luaL_push_oci_value(L,pinfo[i].type,row+pinfo[i].offset,pinfo[i].len);
        }
    };

    enum {max_bind_count=64};

    struct OCI_BIND_INFO
    {
        ub2 type;
        ub4 len;
        char* ptr;
    };

    struct OCI_BIND_DEFINE
    {
        ub4 pn;
        OCI_BIND_INFO pinfo[max_bind_count];

        void init(void)
        {
            pn=0;
        }

        OCI_BIND_INFO* add(ub2 type,ub4 datalen)
        {
            if(pn>=max_bind_count)
                return 0;

            pinfo[pn].ptr=(char*)calloc(1,datalen);
            if(!pinfo[pn].ptr)
                return 0;

            pinfo[pn].type=type;
            pinfo[pn].len=datalen;

            return &pinfo[pn++];
        }

        void clear(void)
        {
            for(ub4 i=0;i<pn;++i)
            {
                if(pinfo[i].ptr)
                {
                    free(pinfo[i].ptr);
                    pinfo[i].ptr=0;
                }
            }
            pn=0;
        }
    };


    struct OCI_SVC
    {
        OCISvcCtx* svc;
    };


    struct OCI_STMT
    {
        OCISvcCtx* svc;
        OCIStmt* stmt;
        ub2 type;

        int executed;

        OCI_ROW_DEFINE row;
        OCI_BIND_DEFINE binds;

        void init(void)
        {
            svc=0;
            stmt=0;
            type=0;
            executed=0;
            row.init();
            binds.init();
        }

        void clear(void)
        {
            svc=0;
            type=0;

            if(stmt)
            {
                OCIHandleFree(stmt,OCI_HTYPE_STMT);
                stmt=0;
            }
            executed=0;
            row.clear();
            binds.clear();
        }
    };


    int lua_loadlib_gc_stub(lua_State* L);

    int oci_is_error(int n);
    int luaL_oci_error(lua_State* L);
    ub2 oci2c_type(ub2 type);
    int oci_get_error_code(void);
    int oci_define_row(OCIStmt* stmt,OCI_ROW_DEFINE& def);

    int lua_ora_open(lua_State* L);
    int lua_ora_close(lua_State* L);
    int lua_ora_gc(lua_State* L);
    int lua_ora_tostring(lua_State* L);
    int lua_ora_query(lua_State* L);
    int lua_ora_stmt_open(lua_State* L);
    int lua_ora_commit(lua_State* L);
    int lua_ora_rollback(lua_State* L);

    int lua_ora_stmt_close(lua_State* L);
    int lua_ora_stmt_gc(lua_State* L);
    int lua_ora_stmt_type(lua_State* L);
    int lua_ora_stmt_tostring(lua_State* L);
    int lua_ora_stmt_field_count(lua_State* L);
    int lua_ora_stmt_execute(lua_State* L);
    int lua_ora_stmt_rows_next(lua_State* L);
    int lua_ora_stmt_rows(lua_State* L);
    int lua_ora_stmt_table(lua_State* L);
    int lua_ora_stmt_bind(lua_State* L);
    int lua_ora_stmt_bind_count(lua_State* L);
    int lua_ora_stmt_bind_assign(lua_State* L);
    int lua_ora_stmt_bind_query(lua_State* L);
    int lua_ora_stmt_at(lua_State* L);
}


int libora::lua_loadlib_gc_stub(lua_State* L)
{
    void** lib=(void**)luaL_checkudata(L,1,LUA_LOADLIB);

    *lib=0;

    return 0;
}

void luaclose_luaora(void)
{
    if(libora::ora_err)
    {
        OCIHandleFree(libora::ora_err,OCI_HTYPE_ERROR);
        libora::ora_err=0;
    }

    if(libora::ora_env)
    {
        OCIHandleFree(libora::ora_env,OCI_HTYPE_ENV);
        libora::ora_env=0;
    }
}


int luaopen_luaora(lua_State* L)
{
#ifdef LUA_MODULE
    // Fix for OCI bug <bug:2012268> :CORE DUMP ON EXIT OF PRO*C PROGRAM USING DLCLOSE()
    // The Oracle trace code in the client
    // registers an exit handler by calling "atexit()". This registers a
    // function which resides inside libclntsh.so to be called at exit
    // from the process. dlclose() unloads the users library and
    // libclntsh.so. exit() tries to call "epc_exit_handler" which is
    // now unmapped.
    //
    // I hold all C lua modules (shared objects) in memory after garbage collection

    luaL_getmetatable(L,libora::LUA_LOADLIB);
    lua_pushcfunction(L,libora::lua_loadlib_gc_stub);
    lua_setfield(L,-2,"__gc");
    lua_pop(L,1);

    // end of fix
#endif /*LUA_MODULE*/


    if(OCIEnvCreate(&libora::ora_env,OCI_DEFAULT,0,0,0,0,0,0)!=OCI_SUCCESS)
        return luaL_error(L,"\"luaopen_luaora\" OCIEnvCreate fault");

    if(OCIHandleAlloc(libora::ora_env,(void**)&libora::ora_err,OCI_HTYPE_ERROR,0,0)!=OCI_SUCCESS)
    {
        OCIHandleFree(libora::ora_env,OCI_HTYPE_ENV);
        libora::ora_env=0;
        return luaL_error(L,"\"luaopen_luaora\" OCIHandleAlloc(...,OCI_HTYPE_ERROR) fault");
    }

#ifdef LUA_MODULE
    atexit(luaclose_luaora);
#endif /*LUA_MODULE*/

    static const luaL_Reg lib[]=
    {
        {"open",libora::lua_ora_open},
        {"close",libora::lua_ora_close},
        {0,0}
    };

    static const luaL_Reg slib[]=
    {
        {"__gc",libora::lua_ora_gc},
        {"__tostring",libora::lua_ora_tostring},
        {"close",libora::lua_ora_close},
        {"query",libora::lua_ora_query},        // simple mode stmt
        {"open",libora::lua_ora_stmt_open},     // fill stmt
        {"commit",libora::lua_ora_commit},
        {"rollback",libora::lua_ora_rollback},
        {0,0}
    };

    static const luaL_Reg rlib[]=
    {
        {"__gc",libora::lua_ora_stmt_gc},
        {"__tostring",libora::lua_ora_stmt_tostring},
        {"close",libora::lua_ora_stmt_close},
        {"type",libora::lua_ora_stmt_type},
        {"execute",libora::lua_ora_stmt_execute},
        {"field_count",libora::lua_ora_stmt_field_count},
        {"rows",libora::lua_ora_stmt_rows},
        {"table",libora::lua_ora_stmt_table},
        {"bind",libora::lua_ora_stmt_bind},
        {"bind_count",libora::lua_ora_stmt_bind_count},
        {"bind_assign",libora::lua_ora_stmt_bind_assign},
        {"bind_query",libora::lua_ora_stmt_bind_query},
        {"at",libora::lua_ora_stmt_at},
        {0,0}
    };

    luaL_newmetatable(L,libora::LUA_OCI_SVC);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");
    luaL_register(L,0,slib);

    luaL_newmetatable(L,libora::LUA_OCI_STMT);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");
    luaL_register(L,0,rlib);

    luaL_register(L,"oracle",lib);


    return 0;
}


int libora::oci_is_error(int n)
{
    switch(n)
    {
    case OCI_SUCCESS:
    case OCI_SUCCESS_WITH_INFO:
    case OCI_NO_DATA:
        return 0;
    }

    int errcode=0;

    if(OCIErrorGet(ora_err,1,0,&errcode,0,0,OCI_HTYPE_ERROR)==OCI_SUCCESS && errcode==1405)
        return 0;

    return 1;
}

int libora::luaL_oci_error(lua_State* L)
{
    char temp[1024];
    int errcode;

    if(ora_err && OCIErrorGet(ora_err,1,0,&errcode,(OraText*)temp,sizeof(temp),OCI_HTYPE_ERROR)==OCI_SUCCESS)
    {
        lua_pushstring(L,temp);
        return lua_error(L);
    }

    return luaL_error(L,"\"luaL_oci_error\" OCIErrorGet fault");
}

int libora::oci_get_error_code(void)
{
    int errcode=0;

    if(ora_err && OCIErrorGet(ora_err,1,0,&errcode,0,0,OCI_HTYPE_ERROR)==OCI_SUCCESS)
        return errcode;
    return 0;
}


int libora::lua_ora_open(lua_State* L)
{
    if(!ora_env || !ora_err)
        return luaL_error(L,"\"lua_ora_open\" uninitialized");

    if(lua_gettop(L)!=1)
        return luaL_error(L,"\"lua_ora_open\" bad params");

    const char* login=luaL_checkstring(L,1);

    const char* passwd=strchr(login,':');

    if(!passwd)
        return luaL_error(L,"\"lua_ora_open\" no passwd found");

    int login_len=passwd-login;
    passwd++;

    const char* service=strchr(passwd,'@');

    if(!service)
        return luaL_error(L,"\"lua_ora_open\" no service name found");

    int passwd_len=service-passwd;
    service++;

    int service_len=strlen(service);

    OCISvcCtx* svc=0;

    if(OCILogon(ora_env,ora_err,&svc,(OraText*)login,login_len,(OraText*)passwd,passwd_len,
        (OraText*)service,service_len)!=OCI_SUCCESS)
            return luaL_oci_error(L);

    OCI_SVC* ora=(OCI_SVC*)lua_newuserdata(L,sizeof(OCI_SVC));

    luaL_getmetatable(L,LUA_OCI_SVC);
    lua_setmetatable(L,-2);

    ora->svc=svc;

    return 1;
}

int libora::lua_ora_close(lua_State* L)
{
    OCI_SVC* ora=(OCI_SVC*)luaL_checkudata(L,1,LUA_OCI_SVC);

    if(ora->svc)
    {
        OCILogoff(ora->svc,ora_err);
        ora->svc=0;
    }
    return 0;
}

int libora::lua_ora_gc(lua_State* L)
{
    return lua_ora_close(L);
}

int libora::lua_ora_tostring(lua_State* L)
{
    OCI_SVC* ora=(OCI_SVC*)luaL_checkudata(L,1,LUA_OCI_SVC);

    lua_pushstring(L,ora->svc?"opened":"closed");

    return 1;
}
int libora::lua_ora_commit(lua_State* L)
{
    OCI_SVC* ora=(OCI_SVC*)luaL_checkudata(L,1,LUA_OCI_SVC);

    if(ora->svc)
    {
        if(OCITransCommit(ora->svc,ora_err,OCI_DEFAULT)!=OCI_SUCCESS)
            return luaL_oci_error(L);
    }
    return 0;
}
int libora::lua_ora_rollback(lua_State* L)
{
    OCI_SVC* ora=(OCI_SVC*)luaL_checkudata(L,1,LUA_OCI_SVC);

    if(ora->svc)
    {
        if(OCITransRollback(ora->svc,ora_err,OCI_DEFAULT)!=OCI_SUCCESS)
            return luaL_oci_error(L);
    }
    return 0;
}



ub2 libora::oci2c_type(ub2 type)
{
    switch(type)
    {
    case SQLT_NUM:
    case SQLT_INT:
    case SQLT_FLT:
    case SQLT_LNG:
    case SQLT_UIN:
        return SQLT_NUM;
    case SQLT_DAT:
        return SQLT_DAT;
    }
    return SQLT_STR;
}

int libora::oci_define_row(OCIStmt* stmt,OCI_ROW_DEFINE& def)
{
    def.init();

    OCIAttrGet(stmt,OCI_HTYPE_STMT,&def.pn,0,OCI_ATTR_PARAM_COUNT,ora_err);

    if(def.pn>0)
    {
        def.pinfo=(OCI_PARAM_INFO*)calloc(def.pn,sizeof(OCI_PARAM_INFO));

		OCIParam* p=0;
		OCIDefine* dfn=0;

		ub4 offset=0;

		for(ub4 i=0;i<def.pn && OCIParamGet(stmt,OCI_HTYPE_STMT,ora_err,(void**)&p,i+1)==OCI_SUCCESS;++i)
		{
		    ub4 len=0;
		    ub2 type=0;

		    OCIAttrGet(p,OCI_DTYPE_PARAM,&len,0,OCI_ATTR_DATA_SIZE,ora_err);
		    OCIAttrGet(p,OCI_DTYPE_PARAM,&type,0,OCI_ATTR_DATA_TYPE,ora_err);

		    type=oci2c_type(type);

		    switch(type)
		    {
		    case SQLT_NUM: def.pinfo[i].len=(len<22?22:len)+1; def.pinfo[i].type=SQLT_STR; break;
		    case SQLT_DAT: def.pinfo[i].len=(len<7?7:len); def.pinfo[i].type=SQLT_DAT; break;
		    default: def.pinfo[i].len=len+1; def.pinfo[i].type=SQLT_STR; break;
		    }

		    def.pinfo[i].offset=offset;
		    offset+=def.pinfo[i].len;
		}

		def.row=(char*)malloc(offset);
		def.row_len=offset;

		for(ub4 i=0;i<def.pn;++i)
		{
		    if(OCIDefineByPos(stmt,&dfn,ora_err,i+1,(dvoid*)(def.row+def.pinfo[i].offset),(sword)def.pinfo[i].len,def.pinfo[i].type,0,0,0,OCI_DEFAULT)!=OCI_SUCCESS)
                def.pinfo[i].clear();
		}
    }
    return 0;
}


int libora::lua_ora_query(lua_State* L)
{
    OCI_SVC* ora=(OCI_SVC*)luaL_checkudata(L,1,LUA_OCI_SVC);

    size_t len=0;

    const char* s=luaL_checklstring(L,2,&len);

    if(!ora->svc)
        return 0;

    OCIStmt* stmt=0;

    // prepare

    if(OCIHandleAlloc(ora_env,(void**)&stmt,OCI_HTYPE_STMT,0,0)!=OCI_SUCCESS)
        return luaL_error(L,"\"lua_ora_query\" OCIHandleAlloc fault");

    if(OCIStmtPrepare(stmt,ora_err,(text*)s,len,OCI_NTV_SYNTAX,OCI_DEFAULT)!=OCI_SUCCESS)
    {
        OCIHandleFree(stmt,OCI_HTYPE_STMT);
        return luaL_oci_error(L);
    }

    ub2 type=0;

    if(OCIAttrGet(stmt,OCI_HTYPE_STMT,&type,0,OCI_ATTR_STMT_TYPE,ora_err)!=OCI_SUCCESS)
    {
        OCIHandleFree(stmt,OCI_HTYPE_STMT);
        return luaL_oci_error(L);
    }

    if(!type || type==OCI_STMT_BEGIN || type==OCI_STMT_DECLARE)
    {
        OCIHandleFree(stmt,OCI_HTYPE_STMT);
        return luaL_error(L,"\"lua_ora_query\" this is OCI_STMT_BEGIN or OCI_STMT_DECLARE statement");
    }

    // execute

	if(oci_is_error(OCIStmtExecute(ora->svc,stmt,ora_err,type==OCI_STMT_SELECT?0:1,0,0,0,type==OCI_STMT_SELECT?OCI_STMT_SCROLLABLE_READONLY:OCI_COMMIT_ON_SUCCESS)))
	{
	    OCIHandleFree(stmt,OCI_HTYPE_STMT);
	    return luaL_oci_error(L);
	}

    // exit if INSERT, UPDATE or DELETE

	if(type!=OCI_STMT_SELECT)
	{
	    OCIHandleFree(stmt,OCI_HTYPE_STMT);
	    return 0;
	}


    // define row for SELECT

    OCI_ROW_DEFINE def;
    oci_define_row(stmt,def);

    // return stmt

    OCI_STMT* oci_stmt=(OCI_STMT*)lua_newuserdata(L,sizeof(OCI_STMT));

    oci_stmt->init();

    luaL_getmetatable(L,LUA_OCI_STMT);
    lua_setmetatable(L,-2);

    oci_stmt->stmt=stmt;
    oci_stmt->svc=ora->svc;
    oci_stmt->type=type;
    oci_stmt->row.copy(def);
    oci_stmt->executed=1;

    return 1;
}

int libora::lua_ora_stmt_open(lua_State* L)
{
    OCI_SVC* ora=(OCI_SVC*)luaL_checkudata(L,1,LUA_OCI_SVC);

    size_t len=0;

    const char* s=luaL_checklstring(L,2,&len);

    if(!ora->svc)
        return 0;

    OCIStmt* stmt=0;

    // prepare

    if(OCIHandleAlloc(ora_env,(void**)&stmt,OCI_HTYPE_STMT,0,0)!=OCI_SUCCESS)
        return luaL_error(L,"\"lua_ora_query\" OCIHandleAlloc fault");

    if(OCIStmtPrepare(stmt,ora_err,(text*)s,len,OCI_NTV_SYNTAX,OCI_DEFAULT)!=OCI_SUCCESS)
    {
        OCIHandleFree(stmt,OCI_HTYPE_STMT);
        return luaL_oci_error(L);
    }

    ub2 type=0;

    if(OCIAttrGet(stmt,OCI_HTYPE_STMT,&type,0,OCI_ATTR_STMT_TYPE,ora_err)!=OCI_SUCCESS)
    {
        OCIHandleFree(stmt,OCI_HTYPE_STMT);
        return luaL_oci_error(L);
    }

    // return stmt

    OCI_STMT* oci_stmt=(OCI_STMT*)lua_newuserdata(L,sizeof(OCI_STMT));

    oci_stmt->init();

    luaL_getmetatable(L,LUA_OCI_STMT);
    lua_setmetatable(L,-2);

    oci_stmt->stmt=stmt;
    oci_stmt->svc=ora->svc;
    oci_stmt->type=type;
    oci_stmt->executed=0;

    return 1;
}



int libora::lua_ora_stmt_close(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    ora->clear();

    return 0;
}

int libora::lua_ora_stmt_gc(lua_State* L)
{
    return lua_ora_stmt_close(L);
}

int libora::lua_ora_stmt_type(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    const char* type="undef";

    if(ora->stmt)
	switch(ora->type)
	{
	case OCI_STMT_SELECT: type="SELECT"; break;
	case OCI_STMT_UPDATE: type="UPDATE"; break;
	case OCI_STMT_DELETE: type="DELETE"; break;
	case OCI_STMT_INSERT: type="INSERT"; break;
	case OCI_STMT_CREATE: type="CREATE"; break;
	case OCI_STMT_DROP: type="DROP"; break;
	case OCI_STMT_ALTER: type="ALTER"; break;
	case OCI_STMT_BEGIN: type="BEGIN"; break;
	case OCI_STMT_DECLARE: type="DECLARE"; break;
	}

    lua_pushstring(L,type);

    return 1;
}

int libora::lua_ora_stmt_tostring(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    lua_pushstring(L,ora->stmt?"opened":"closed");

    return 1;
}

int libora::lua_ora_stmt_field_count(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    lua_pushinteger(L,ora->row.pn);

    return 1;
}

int libora::lua_ora_stmt_execute(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    if(!ora->stmt)
        return 0;

    if(ora->executed)
    {
        if(ora->type==OCI_STMT_SELECT)
            ora->row.clear();
    }

    if(oci_is_error(OCIStmtExecute(ora->svc,ora->stmt,ora_err,ora->type==OCI_STMT_SELECT?0:1,0,0,0,ora->type==OCI_STMT_SELECT?OCI_STMT_SCROLLABLE_READONLY:0)))   // no auto commit
        return luaL_oci_error(L);

    // define row for SELECT

    if(ora->type==OCI_STMT_SELECT)
    {
        OCI_ROW_DEFINE def;
        oci_define_row(ora->stmt,def);

        ora->row.copy(def);
    }

    ora->executed=1;

    return 0;
}

int libora::lua_ora_stmt_rows_next(lua_State* L)
{

    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,lua_upvalueindex(1),LUA_OCI_STMT);

    int orient=luaL_checkint(L,lua_upvalueindex(2));

    if(orient==OCI_FETCH_FIRST)
    {
        lua_pushinteger(L,OCI_FETCH_NEXT);
        lua_replace(L,lua_upvalueindex(2));
    }

    if(!ora->stmt || !ora->row.row)
        return 0;


    if(OCIStmtFetch(ora->stmt,ora_err,1,orient,OCI_DEFAULT)!=OCI_SUCCESS && oci_get_error_code()!=1405)
	    return 0;

    lua_newtable(L);

    for(ub4 i=0;i<ora->row.pn;i++)
    {
        ora->row.luaL_push(L,i);

        lua_rawseti(L,-2,i+1);
    }

    return 1;
}

void libora::luaL_push_oci_value(lua_State* L,ub2 type,const char* p,ub4 len)
{
    if(type==SQLT_STR)
    {
//        lua_pushlstring(L,p,len);
        lua_pushstring(L,p);
    }else if(type==SQLT_DAT)
    {
        const unsigned char* d=(const unsigned char*)p;

        char tmp[32];

        int n=sprintf(tmp,"%.4i-%.2i-%.2i %.2i:%.2i:%.2i",(d[0]<100?d[0]:d[0]-100)*100+(d[1]<100?d[1]:d[1]-100),
            d[2],d[3],d[4]-1,d[5]-1,d[6]-1);

        lua_pushlstring(L,tmp,n);
    }else
        lua_pushstring(L,"");
}

int libora::lua_ora_stmt_rows(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    if(!ora->stmt || !ora->row.row || !ora->executed)
        return 0;

    lua_pushvalue(L,-1);
    lua_pushinteger(L,OCI_FETCH_FIRST);
    lua_pushcclosure(L,lua_ora_stmt_rows_next,2);

    return 1;
}


int libora::lua_ora_stmt_table(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    if(!ora->stmt || !ora->row.row || !ora->executed)
        return 0;

    lua_newtable(L);
    int j=1;

    int orient=OCI_FETCH_FIRST;

    while(1)
    {
        if(OCIStmtFetch(ora->stmt,ora_err,1,orient,OCI_DEFAULT)!=OCI_SUCCESS && oci_get_error_code()!=1405)
            break;

        orient=OCI_FETCH_NEXT;

        lua_newtable(L);

        for(ub4 i=0;i<ora->row.pn;i++)
        {
            ora->row.luaL_push(L,i);

            lua_rawseti(L,-2,i+1);
        }

        lua_rawseti(L,-2,j++);
    }
    return 1;
}

int libora::lua_ora_stmt_bind(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    int pos=-1;
    size_t name_len=0;
    const char* name=0;
    ub2 type=SQLT_STR;
    size_t length=0;

    if(lua_type(L,2)==LUA_TNUMBER)
    {
        pos=luaL_checkint(L,2);
        if(pos<1)
            return luaL_error(L,"\"lua_ora_stmt_bind\" invalid index");
    }else
        name=luaL_checklstring(L,2,&name_len);

    if(lua_gettop(L)>3)
        length=luaL_checkint(L,4);

    if(lua_gettop(L)>2)
    {
        const char* type_name=luaL_checkstring(L,3);
        if(!strcasecmp(type_name,"date")) { type=SQLT_DAT; length=length<7?7:length; }
        else if(!strcasecmp(type_name,"number")) { type=SQLT_STR; length=(length<22?22:length)+1;}
        else if(!strcasecmp(type_name,"string")) { type=SQLT_STR; length=(length<128?128:length)+1;}
        else
            return luaL_error(L,"\"lua_ora_stmt_bind\" unknown data type, allow: date,number,string");
    }

    if(length<1)
        length=128;

    if(!ora->stmt)
        return 0;

    OCI_BIND_INFO* b=ora->binds.add(type,length);

    if(!b)
        return luaL_error(L,"\"lua_ora_stmt_bind\" max bind count reached");

    OCIBind* bnd=0;

    if(!name)
    {
        if(OCIBindByPos(ora->stmt,&bnd,ora_err,pos,(dvoid*)b->ptr,(sword)b->len,b->type,0,0,0,0,0,OCI_DEFAULT)!=OCI_SUCCESS)
            return luaL_oci_error(L);
    }else
    {
        if(OCIBindByName(ora->stmt,&bnd,ora_err,(text*)name,name_len,(dvoid*)b->ptr,(sword)b->len,b->type,0,0,0,0,0,OCI_DEFAULT)!=OCI_SUCCESS)
            return luaL_oci_error(L);
    }

    lua_pushinteger(L,ora->binds.pn);

    return 1;
}
int libora::lua_ora_stmt_bind_count(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    lua_pushinteger(L,ora->binds.pn);

    return 1;
}
int libora::lua_ora_stmt_bind_query(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    int n=luaL_checkint(L,2);

    if(n<1 || n>ora->binds.pn)
        return 0;

    n--;
    luaL_push_oci_value(L,ora->binds.pinfo[n].type,ora->binds.pinfo[n].ptr,ora->binds.pinfo[n].len);

    return 1;
}
int libora::lua_ora_stmt_bind_assign(lua_State* L)
{
    OCI_STMT* ora=(OCI_STMT*)luaL_checkudata(L,1,LUA_OCI_STMT);

    int n=luaL_checkint(L,2);

    const char* s=luaL_checkstring(L,3);

    if(n<1 || n>ora->binds.pn)
        return 0;

    n--;

    switch(ora->binds.pinfo[n].type)
    {
    case SQLT_NUM:
    case SQLT_STR:
        {
            char* p=ora->binds.pinfo[n].ptr;
            ub4 l=ora->binds.pinfo[n].len;

            int n=snprintf(p,l,"%s",s);
            if(n<0 || n>=l)
                p[l-1]=0;
        }
        break;
    case SQLT_DAT:
        if(ora->binds.pinfo[n].len>6)
        {
            tm t;
            sscanf(s,"%u-%u-%u %u:%u:%u",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec);

            unsigned char* p=(unsigned char*)ora->binds.pinfo[n].ptr;
            p[0]=t.tm_year/100+100;
            p[1]=t.tm_year%100+100;
            p[2]=t.tm_mon;
            p[3]=t.tm_mday;
            p[4]=t.tm_hour+1;
            p[5]=t.tm_min+1;
            p[6]=t.tm_sec+1;
        }
        break;
    }



    return 0;
}

int libora::lua_ora_stmt_at(lua_State* L)
{
    return lua_gettop(L)>2?lua_ora_stmt_bind_assign(L):lua_ora_stmt_bind_query(L);
}
