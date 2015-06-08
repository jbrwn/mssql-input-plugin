#include "mssql_featureset.hpp"
#include "mssql_statement.hpp"

// mapnik
#include <mapnik/feature_factory.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/wkb.hpp>
#include <mapnik/unicode.hpp>

#include <memory>

mssql_featureset::mssql_featureset(std::shared_ptr<mssql_statement> const& stmt,
                 mapnik::context_ptr const& ctx,
                 std::string const& encoding)
  : stmt_(stmt),
    ctx_(ctx),
    feature_id_(1),
    char_tr_(std::make_unique<mapnik::transcoder>(encoding)),
    wchar_tr_(std::make_unique<mapnik::transcoder>("utf-16le"))
{

}

mssql_featureset::~mssql_featureset() { }

mapnik::feature_ptr mssql_featureset::next()
{
    while (stmt_->fetch())
    {
        mapnik::feature_ptr feature = mapnik::feature_factory::create(ctx_, feature_id_);
        feature_id_++;
        
        //get geom
        size_t geom_size = stmt_->get_length(0);
        const char * geom_data = stmt_->get_char(0);
        mapnik::geometry::geometry<double> geom = mapnik::geometry_utils::from_wkb(geom_data, geom_size);
        feature->set_geometry(std::move(geom));
        
        short cols = stmt_->columns();
        for (int i = 1; i < cols; i++)
        {
            if (!stmt_->is_null(i))
            {
                std::string name = stmt_->get_name(i);
                short sql_type = stmt_->get_type(i);
                size_t size;
                const char * data;
                switch (sql_type)
                {
                    case -7: //SQL_BIT
                    case -6: //TINYINT
                    case 5:  //SQL_SMALLINT
                    case 4:  //SQL_INTEGER
                        feature->put(name, stmt_->get_int32(i));
                        break;
                    case -5: //SQL_BIGINT
                        feature->put(name, stmt_->get_int64(i));
                        break;
                    case 8:  //SQL_DOUBLE
                    case 6:  //SQL_FLOAT
                    case 3:  //SQL_DECIMAL
                    case 7:  //SQL_REAL
                    case 2:  //SQL_NUMERIC
                        feature->put(name, stmt_->get_double(i));
                        break;
                    case -2: //SQL_BINARY
                    case -3: //SQL_VARBINARY
                    case -4: //SQL_LONGVARBINARY
                        //not supported
                        break;
                    case -8:  //SQL_WCHAR
                    case -9:  //SQL_WVARCHAR
                    case -10: //SQL_LONGVARCHAR
                        size = stmt_->get_length(i);
                        data = stmt_->get_char(i);
                        feature->put(name, wchar_tr_->transcode(data,size));
                        break;
                    default:
                        //everything else maps to string
                        size = stmt_->get_length(i);
                        data = stmt_->get_char(i);
                        feature->put(name, char_tr_->transcode(data,size));
                        break;
                }
            }
        }
        return feature;
    }
    return mapnik::feature_ptr();
}