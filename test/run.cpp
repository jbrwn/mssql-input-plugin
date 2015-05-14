#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

// icu
#include <unicode/uclean.h>

TEST_CASE("mssql datasource") {
    //This will be executed before each section
    
    SECTION("") {
    
    }
}

int main (int argc, char* const argv[])
{
    int result = Catch::Session().run( argc, argv );
    // http://icu-project.org/apiref/icu4c/uclean_8h.html#a93f27d0ddc7c196a1da864763f2d8920
    u_cleanup();
    return result;
}
