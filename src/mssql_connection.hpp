#ifndef MSSQL_CONNECTION_HPP
#define MSSQL_CONNECTION_HPP

#include "odbc_exception.hpp"

#ifdef _WINDOWS
#include <windows.h>
#endif // _WINDOWS
#include <sql.h>
#include <sqlext.h>

#include <string>

class mssql_connection
{
public:
    mssql_connection(const std::string& connection_string)
     : env_(0),
       con_(0)
    {
        SQLRETURN rc;
        rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
        if (!SQL_SUCCEEDED(rc))
            throw odbc_exception(rc, SQL_HANDLE_ENV, env_);
        
        try
        {
            rc = SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_UINTEGER);
            if (!SQL_SUCCEEDED(rc))
                throw odbc_exception(rc, SQL_HANDLE_ENV, env_);
            
            rc = SQLAllocHandle(SQL_HANDLE_DBC, env_, &con_);
            if (!SQL_SUCCEEDED(rc))
                throw odbc_exception(rc, SQL_HANDLE_ENV, env_);
            
        }
        catch (...)
        {
            SQLFreeHandle(SQL_HANDLE_ENV, env_);
            throw;
        }
        
        try
        {
            SQLCHAR conOut[1024];
            SQLSMALLINT conOutSize;
            rc = SQLDriverConnect(con_, NULL, (SQLCHAR*)connection_string.c_str(), SQL_NTS, conOut, sizeof(conOut), &conOutSize, SQL_DRIVER_NOPROMPT);
            if (!SQL_SUCCEEDED(rc))
                throw odbc_exception(rc, SQL_HANDLE_DBC, con_);
        }
        catch (...)
        {
            SQLFreeHandle(SQL_HANDLE_DBC, con_);
            SQLFreeHandle(SQL_HANDLE_ENV, env_);
            throw;
        }
        
    }
    ~mssql_connection()
    {
        SQLDisconnect(con_);
        SQLFreeHandle(SQL_HANDLE_DBC, con_);
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
    }
    
    const SQLHDBC& _get_connection_handle()
    {
        return con_;
    }
    
    
private:
    SQLHENV env_;
    SQLHDBC con_;
    
    
};


#endif // MSSQL_CONNECTION_HPP