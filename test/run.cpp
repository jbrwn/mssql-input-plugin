#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

//mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>

static std::string MSSQL_CONNECTION_STRING = (std::getenv("MSSQL_CONNECTION_STRING") == nullptr) ?
    "Driver={SQL Server};Server=.;Database=mapnik_tmp_mssql_db;Trusted_Connection=Yes;" :
    std::getenv("MSSQL_CONNECTION_STRING");

TEST_CASE("mssql") {

    //register plugin
    std::string mssql_plugin = "./mssql.input";
    mapnik::datasource_cache::instance().register_datasource(mssql_plugin);

    SECTION("is registered")
    {
        std::vector<std::string> plugins = mapnik::datasource_cache::instance().plugin_names();
        std::string plugin_name = "mssql";
        auto itr = std::find(plugins.begin(), plugins.end(), plugin_name);
        REQUIRE(itr != plugins.end());
    }

    SECTION("connect")
    {
        mapnik::parameters p;
        p["type"] = "mssql";
        p["connection_string"] = MSSQL_CONNECTION_STRING;
        p["table"] = "table1";
        p["geometry_field"] = "geom";
        p["srid"] = 4326;
        p["extent"] = "-10,-10,10,10";
        std::shared_ptr<mapnik::datasource> ds = mapnik::datasource_cache::instance().create(p);
    }
}

int main (int argc, char* const argv[])
{
    int result = Catch::Session().run( argc, argv );
    return result;
}
