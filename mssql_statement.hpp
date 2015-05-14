#ifndef MSSQL_STATEMENT_HPP
#define MSSQL_STATEMENT_HPP

#include "mssql_connection.hpp"
#include "odbc_exception.hpp"

#ifdef _WINDOWS
#include <windows.h>
#endif // _WINDOWS
#include <sql.h>
#include <sqlext.h>

#include <string>
#include <vector>

class mssql_statement
{
    struct column_info
    {
        column_info()
          : name(),
            native_type(),
            sql_type(0),
            sql_size(0),
            sql_scale(0),
            sql_nullable(0),
            c_type(0),
            c_len(0),
            len_or_ind(0),
            buffer(0),
            lob(0)
        {}
        ~column_info()
        {}

        std::string name;
        std::string native_type;
        SQLSMALLINT sql_type;
        SQLULEN sql_size;
        SQLSMALLINT sql_scale;
        SQLSMALLINT sql_nullable;
        SQLSMALLINT c_type;
        size_t c_len;
        SQLLEN len_or_ind;
        std::vector<char> buffer;
        bool lob;
    };
    
public:
    mssql_statement(mssql_connection& con, const std::string& sql)
      : stmt_(0),
        sql_(sql),
        cols_(0)
    {
        SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, con._get_connection_handle(), &stmt_);
        if (!SQL_SUCCEEDED(rc))
            throw odbc_exception(rc, SQL_HANDLE_DBC, con._get_connection_handle());
    }
    
    void execute()
    {
        SQLRETURN rc;
        rc = SQLExecDirect(stmt_, (SQLCHAR*)sql_.c_str(), SQL_NTS);
        if (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA)
            throw odbc_exception(rc, SQL_HANDLE_STMT, stmt_);
        
        rc = SQLNumResultCols(stmt_, &cols_);
        if (!SQL_SUCCEEDED(rc))
            throw odbc_exception(rc, SQL_HANDLE_STMT, stmt_);
        
        column_info_.resize(cols_);
        for (int i = 0; i < cols_; i++)
        {
            column_info& col = column_info_[i];
            SQLCHAR c_name[256];
            SQLSMALLINT len;
            rc = SQLDescribeCol(
                stmt_,
                i + 1,
                c_name,
                sizeof(c_name),
                &len,
                &col.sql_type,
                &col.sql_size,
                &col.sql_scale,
                &col.sql_nullable
            );
            if (!SQL_SUCCEEDED(rc))
                throw odbc_exception(rc, SQL_HANDLE_STMT, stmt_);
            
            col.name.assign(reinterpret_cast<char*>(c_name));
            
            rc = SQLColAttribute(
                stmt_,
                i + 1,
                SQL_DESC_TYPE_NAME,
                c_name,
                sizeof(c_name),
                &len,
                NULL
            );
            if (!SQL_SUCCEEDED(rc))
                throw odbc_exception(rc, SQL_HANDLE_STMT, stmt_);
            
            col.native_type.assign(reinterpret_cast<char*>(c_name));
            
            switch (col.sql_type)
            {
                case SQL_WCHAR:
                case SQL_WVARCHAR:
                    col.c_type = SQL_C_WCHAR;
                    col.c_len = sizeof(SQLWCHAR) * (col.sql_size + 1);
                    break;
                case SQL_WLONGVARCHAR:
                    col.c_type = SQL_C_WCHAR;
                    col.lob = true;
                    col.c_len = 1024;
                    break;
                case SQL_CHAR:
                case SQL_VARCHAR:
                    col.c_type = SQL_C_CHAR;
                    col.c_len = sizeof(SQLCHAR) * (col.sql_size + 1);
                    break;
                case SQL_LONGVARCHAR:
                    col.c_type = SQL_C_CHAR;
                    col.lob = true;
                    col.c_len = 1024;
                    break;
                case SQL_BINARY:
                case SQL_VARBINARY:
                    col.c_type = SQL_C_BINARY;
                    col.c_len = col.sql_size;
                    break;
                case SQL_LONGVARBINARY:
                    col.c_type = SQL_C_BINARY;
                    col.lob = true;
                    col.c_len = 1024;
                    break;
                case SQL_BIT:
                case SQL_TINYINT:
                case SQL_SMALLINT:
                case SQL_INTEGER:
                    col.c_type = SQL_C_LONG;
                    col.c_len = sizeof(SQLINTEGER);
                    break;
                case SQL_BIGINT:
                    col.c_type = SQL_C_SBIGINT;
                    col.c_len = sizeof(SQLBIGINT);
                    break;
                case SQL_DOUBLE:
                case SQL_FLOAT:
                case SQL_DECIMAL:
                case SQL_REAL:
                case SQL_NUMERIC:
                    col.c_type = SQL_C_DOUBLE;
                    col.c_len = sizeof(SQLDOUBLE);
                    break;
                default:
                    col.c_type = SQL_C_CHAR;
                    col.c_len = 128;
                    break;
            }
        }
    }
    
    bool fetch()
    {
        if (!(cols_ > 0))
            return false;
    
        SQLRETURN rc;
        rc = SQLFetch(stmt_);
        if (!SQL_SUCCEEDED(rc))
        {
            if (rc == SQL_NO_DATA)
                return false;
            throw odbc_exception(rc, SQL_HANDLE_STMT, stmt_);
        }

        for (int i = 0; i < cols_; i++)
        {
            column_info& col = column_info_[i];
            col.buffer.resize(col.c_len);
            rc = SQLGetData(
                stmt_,
                i + 1,
                col.c_type,
                col.buffer.data(),
                col.buffer.size(),
                &col.len_or_ind
            );
            if (!SQL_SUCCEEDED(rc))
                throw odbc_exception(rc, SQL_HANDLE_STMT, stmt_);
            
            if (rc == SQL_SUCCESS_WITH_INFO && col.lob)
            {
                size_t char_size(0);
                if (col.c_type == SQL_C_WCHAR)
                    char_size = sizeof(SQLWCHAR);
                if (col.c_type == SQL_C_CHAR)
                    char_size =  sizeof(SQLCHAR);

                if (col.len_or_ind != SQL_NO_TOTAL)
                {
                    size_t current_index = col.buffer.size() - char_size;
                    col.buffer.resize(col.len_or_ind + char_size);
                    SQLLEN remaining_bytes(col.buffer.size() - current_index);
                    char * buf_ptr = &col.buffer[current_index];
                    rc = SQLGetData(
                        stmt_,
                        i + 1,
                        col.c_type,
                        buf_ptr,
                        remaining_bytes,
                        &col.len_or_ind
                    );
                    if (!SQL_SUCCEEDED(rc))
                        throw odbc_exception(rc, SQL_HANDLE_STMT, stmt_);

                    //reset length
                    col.len_or_ind = col.buffer.size() - char_size;
                }
                else
                {
                    //TO DO: read in chunks if SQL_NO_TOTAL
                }
            }
        }
        return true;

    }

    short columns()
    {
        return cols_;
    }

    bool is_null(int i)
    {
        column_info& col = column_info_[i];
        return (col.len_or_ind == SQL_NULL_DATA);
    }

    std::string get_name(int i)
    {
        return column_info_[i].name;
    }

    short get_type(int i)
    {
        return column_info_[i].sql_type;
    }

    size_t get_length(int i)
    {
        size_t value = 0;
        column_info& col = column_info_[i];
        if (col.len_or_ind != SQL_NULL_DATA)
        {
            switch (col.c_type)
            {
            case SQL_C_CHAR:
                value = sizeof(SQLCHAR) * (col.len_or_ind + 1);
                break;
            case SQL_C_WCHAR:
                value = sizeof(SQLWCHAR) * (col.len_or_ind + 1);
            default:
                value = col.len_or_ind;
                break;
            }
        }
        return value;
    }
    
    long get_int32(int i)
    {
        char* buf = column_info_[i].buffer.data();
        SQLINTEGER* value = reinterpret_cast<SQLINTEGER*>(buf);
        return *value;
    }

    long long get_int64(int i)
    {
        char* buf = column_info_[i].buffer.data();
        SQLBIGINT* value = reinterpret_cast<SQLBIGINT*>(buf);
        return *value;
    }
    
    double get_double(int i)
    {
        char* buf = column_info_[i].buffer.data();
        SQLDOUBLE* d = reinterpret_cast<SQLDOUBLE*>(buf);
        return *d;
    }
    
    const char* get_char(int i)
    {
        column_info& col = column_info_[i];
        return col.buffer.data();
    }
    
    ~mssql_statement()
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt_);
    }
private:
    std::vector<column_info> column_info_;
    SQLSMALLINT cols_;
    std::string sql_;
    SQLHSTMT stmt_;
};

#endif // MSSQL_STATEMENT_HPP