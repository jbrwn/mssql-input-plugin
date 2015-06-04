// file plugin
#include "mssql_datasource.hpp"
#include "mssql_featureset.hpp"
#include "mssql_connection.hpp"
#include "mssql_statement.hpp"

// boost

#include <sstream>
#include <memory>

using mapnik::datasource;
using mapnik::parameters;

DATASOURCE_PLUGIN(mssql_datasource)

mssql_datasource::mssql_datasource(parameters const& params)
    : datasource(params),
      connection_string_(*params.get<std::string>("connection_string", "")),
      table_(*params.get<std::string>("table","")),
      geometry_field_(*params.get<std::string>("geometry_field","")),
      srid_(*params.get<mapnik::value_integer>("srid", 0)),
      sort_field_(*params.get<std::string>("sort_field","")),
      sort_order_(*params.get<std::string>("sort_order","ASC")),
      encoding_(*params.get<std::string>("encoding","")),
      extent_(),
      desc_(mssql_datasource::name(),*params.get<std::string>("encoding","utf-8"))
{
    //connection_string must not be empty
    if (connection_string_.empty())
        throw mapnik::datasource_exception("missing <connection_string> parameter");

    //table must not be empty
    if (table_.empty())
        throw mapnik::datasource_exception("missing <table> parameter");
    
    //geometry_field must not be empty
    // TO DO: automatically detect geometry field if empty
    if (geometry_field_.empty())
        throw mapnik::datasource_exception("missing <geometry_field> parameter");

    //srid must not be empty
    // TO DO: automatically detect srid if empty
    if (srid_ == 0)
        throw mapnik::datasource_exception("missing <srid> parameter");

    //extent must not be empty
    // TO DO: automatically detect extent if empty
    boost::optional<std::string> ext = params.get<std::string>("extent");
    if (!ext)
        throw mapnik::datasource_exception("missing <extent> parameter");
    extent_.from_string(*ext);
    
    //SQL_C_CHAR encoding is driver specific and not available in the ODBC api
    //best we can do is choose sensible defaults
    if (encoding_.empty())
    {
#ifdef _WINDOWS
		encoding_ = "Windows-1252";
#endif // _WINDOWS
        encoding_ = "utf-8";
    }
    
    //create connection
    connection_ = std::make_unique<mssql_connection>(connection_string_);

    //populate desc_
    mssql_statement stmt(*connection_, "SELECT TOP 0 * FROM " + table_);
    stmt.execute();
    for (int i = 0; i < stmt.columns(); i++)
    {
        std::string name = stmt.get_name(i);
        short sql_type = stmt.get_type(i);
        switch (sql_type)
        {
        case -7: //SQL_BIT
        case -6: //TINYINT
        case 5:  //SQL_SMALLINT
        case 4:  //SQL_INTEGER
        case -5: //SQL_BIGINT
            desc_.add_descriptor(mapnik::attribute_descriptor(name, mapnik::Integer));
            break;
        case 8:  //SQL_DOUBLE
        case 6:  //SQL_FLOAT
        case 3:  //SQL_DECIMAL
        case 7:  //SQL_REAL
        case 2:  //SQL_NUMERIC
            desc_.add_descriptor(mapnik::attribute_descriptor(name, mapnik::Double));
        case -2: //SQL_BINARY
        case -3: //SQL_VARBINARY
        case -4: //SQL_LONGVARBINARY
            //not supported
            break;
        default:
            //everything else maps to string
            desc_.add_descriptor(mapnik::attribute_descriptor(name, mapnik::String));
            break;
        }
    }
    

}

mssql_datasource::~mssql_datasource() { }

const char * mssql_datasource::name()
{
    return "mssql";
}

mapnik::datasource::datasource_t mssql_datasource::type() const
{
    return datasource::Vector;
}

mapnik::box2d<double> mssql_datasource::envelope() const
{
    return extent_;
}

boost::optional<mapnik::datasource_geometry_t> mssql_datasource::get_geometry_type() const
{
    boost::optional<mapnik::datasource_geometry_t> result;
    return result;
}

mapnik::layer_descriptor mssql_datasource::get_descriptor() const
{
    return desc_;
}

mapnik::featureset_ptr mssql_datasource::features(mapnik::query const& q) const
{
    mapnik::box2d<double> const& box = q.get_bbox();
    std::set<std::string> const& properties = q.property_names();

    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    std::string sql = "SELECT " + geometry_field_ + ".STAsBinary() ";
    for (auto& prop : properties)
    {
        sql += ", [" + prop + "]";
        ctx->push(prop);
    }
    sql += " FROM " + table_;
    sql += " WHERE " + geometry_field_ + ".STIntersects(" + box_to_sqlgeom(box, srid_) + ") = 1";
    if (!sort_field_.empty())
    {
        sql += " ORDER BY " + sort_field_ + " " + sort_order_;
    }
    
    std::shared_ptr<mssql_statement> stmt = std::make_shared<mssql_statement>(*connection_, sql);
    stmt->execute();


    return std::make_shared<mssql_featureset>(stmt,ctx,encoding_);
}

mapnik::featureset_ptr mssql_datasource::features_at_point(mapnik::coord2d const& pt, double tol) const
{
    // features_at_point is rarely used - only by custom applications,
    // so for this sample plugin let's do nothing...
    return mapnik::featureset_ptr();
}

std::string mssql_datasource::box_to_sqlgeom(mapnik::box2d<double> const& box, int srid) const
{
    std::stringstream ss;
    ss << "geometry::STPolyFromText('POLYGON(("
        << box.minx() << " " << box.miny() << ", "
        << box.maxx() << " " << box.miny() << ", "
        << box.maxx() << " " << box.maxy() << ", "
        << box.minx() << " " << box.maxy() << ", "
        << box.minx() << " " << box.miny()
        << "))', " << srid << ")";
    return ss.str();
}