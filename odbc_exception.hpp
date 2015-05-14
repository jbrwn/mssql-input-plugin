#ifndef ODBC_EXCEPTION_HPP
#define ODBC_EXCEPTION_HPP

#ifdef _WINDOWS
#include <windows.h>
#endif // _WINDOWS

#include <sql.h>
#include <sqlext.h>

#include <string>
#include <sstream>

class odbc_exception : public std::exception
{
public:
    odbc_exception(SQLRETURN rc, SQLSMALLINT handle_type, SQLHANDLE handle)
      : message_("")
    {
        std::ostringstream ss;
        
        if (rc == SQL_INVALID_HANDLE)
        {
            ss << "Invalid SQL handle";
        }
        else
        {
            SQLSMALLINT i = 1;
            SQLCHAR sql_state[6];
            SQLINTEGER native_error;
            SQLCHAR message_text[SQL_MAX_MESSAGE_LENGTH+1];
            SQLSMALLINT len;
            while (SQL_SUCCESS == SQLGetDiagRec(handle_type, handle, i, sql_state, &native_error, message_text, sizeof(message_text), &len))
            {
                if (i > 1)
                    ss << " ";
                ss << "SQLState: " << sql_state << " Native Error: " << native_error << " Message: " << message_text;
                i++;
            }
        }
        this->message_ = ss.str();
    }
    ~odbc_exception() throw()
    {
    }
    virtual const char* what() const throw()
    {
        return message_.c_str();
    }
private:
    std::string message_;
};


#endif // ODBC_EXCEPTION_HPP