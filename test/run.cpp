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
    std::string mssql_plugin = "src/mssql.input";
    mapnik::datasource_cache::instance().register_datasource(mssql_plugin);

    SECTION("is registered")
    {
        std::vector<std::string> plugins = mapnik::datasource_cache::instance().plugin_names();
        std::string plugin_name = "mssql";
        auto itr = std::find(plugins.begin(), plugins.end(), plugin_name);
        REQUIRE(itr != plugins.end());
    }

    SECTION("create datasource")
    {
        mapnik::parameters p;
        p["type"] = "mssql";
        p["connection_string"] = MSSQL_CONNECTION_STRING;
        p["table"] = "table1";
        p["geometry_field"] = "geom";
        p["srid"] = 4326;
        p["extent"] = "-10,-10,10,10";
        std::shared_ptr<mapnik::datasource> ds = mapnik::datasource_cache::instance().create(p);
        auto expected_type = mapnik::datasource::datasource_t::Vector;
        auto expected_extent = mapnik::box2d<double>(-10, -10, 10, 10);
        REQUIRE(expected_type == ds->type());
        REQUIRE(expected_extent == ds->envelope());
        mapnik::layer_descriptor ld = ds->get_descriptor();
        std::vector<mapnik::attribute_descriptor> attributes = ld.get_descriptors();
        CHECK(25 == attributes.size());

        SECTION("query features")
        {
            mapnik::query q(ds->envelope());
            for (auto descriptor : attributes)
            {
                q.add_property_name(descriptor.get_name());
            }
            mapnik::featureset_ptr fs = ds->features(q);
            mapnik::feature_ptr f1 = fs->next();
            
            //test attributes
            
            //integers
            std::vector<std::string> int32s {"_bit","_int","_smallint","_tinyint"};
            for (auto name : int32s)
            {
                mapnik::value attr = f1->get(name);
                CHECK(attr.is<mapnik::value_integer>());
                CHECK(1 == attr.to_int());
            }

            //64 bit integer
            mapnik::value _bigint = f1->get("_bigint");
            CHECK(_bigint.is<mapnik::value_integer>());
            CHECK(2147483648 == _bigint.to_int());

            //doubles
            std::vector<std::string> doubles { "_decimal","_money","_numeric","_smallmoney","_float","_real" };
            for (auto name : doubles)
            {
                mapnik::value attr = f1->get(name);
                CHECK(attr.is<double>());
                CHECK(1.25 == attr.to_double());
            }

            //dates and times 
            CHECK(f1->get("_date").is<mapnik::value_unicode_string>());
            CHECK("2000-01-01" == f1->get("_date").to_string());
            CHECK(f1->get("_datetime2").is<mapnik::value_unicode_string>());
            CHECK("2000-01-01 00:00:00.0000000" == f1->get("_datetime2").to_string());
            CHECK(f1->get("_datetime").is<mapnik::value_unicode_string>());
            CHECK("2000-01-01 00:00:00.000" == f1->get("_datetime").to_string());
            CHECK(f1->get("_datetimeoffset").is<mapnik::value_unicode_string>());
            CHECK("2000-01-01 12:00:00.0000000 +01:00" == f1->get("_datetimeoffset").to_string());
            CHECK(f1->get("_time").is<mapnik::value_unicode_string>());
            CHECK("12:00:00.0000000" == f1->get("_time").to_string());

            //text
            std::vector<std::string> text { "_text","_varchar","_ntext","_nvarchar" };
            for (auto name : text)
            {
                mapnik::value attr = f1->get(name);
                CHECK(attr.is<mapnik::value_unicode_string>());
                CHECK("text" == attr.to_string());
            }

            //binary
            std::vector<std::string> binary { "_varbinary","_binary","_image" };
            for (auto name : binary)
            {
                //binary not supported
                CHECK(!f1->has_key(name));
            }

            //null
            CHECK(f1->has_key("_null"));

            //guid
            mapnik::value _uniqueidentifier = f1->get("_uniqueidentifier");
            CHECK(_uniqueidentifier.is<mapnik::value_unicode_string>());
            CHECK("BE62A766-2614-42F9-A26E-74DDB9065977" == _uniqueidentifier.to_string());

            //test geom
            mapnik::geometry::geometry<double> geom = f1->get_geometry();
            REQUIRE(geom.is<mapnik::geometry::polygon<double>>());
            auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double>>(geom);
            REQUIRE(5 == poly.exterior_ring.size());
            REQUIRE(0 == poly.exterior_ring[0].x);
            REQUIRE(0 == poly.exterior_ring[0].y);

            //next feature should be empty
            mapnik::feature_ptr f2 = fs->next();
            REQUIRE(mapnik::feature_ptr() == f2);
        }
    }
}

int main (int argc, char* const argv[])
{
    int result = Catch::Session().run( argc, argv );
    return result;
}
