#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

//mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>

//icu
#include <unicode/uclean.h>

TEST_CASE("mssql datasource") {
    //This will be executed before each section
    
    SECTION("register plugin") 
	{
		std::string mssql_plugin = "./mssql.input";
		REQUIRE(mapnik::datasource_cache::instance().register_datasource(mssql_plugin));
    }
}

char * _connection_string = NULL;

int main (int argc, char* const argv[])
{
	//get connection string
	_connection_string = std::getenv("MSSQL_CONNECTION_STRING");
	if (_connection_string == NULL)
	{
		_connection_string = "Driver={SQL Server};Server=.;Database=master;Trusted_Connection=Yes;";
	}

	//run tests
    int result = Catch::Session().run( argc, argv );

	//clean up
    // http://icu-project.org/apiref/icu4c/uclean_8h.html#a93f27d0ddc7c196a1da864763f2d8920
    u_cleanup();
    return result;
}
