#ifndef MSSQL_DATASOURCE_HPP
#define MSSQL_DATASOURCE_HPP

#include "mssql_connection.hpp"

#include <mapnik/datasource.hpp>
#include <mapnik/params.hpp>
#include <mapnik/query.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/coord.hpp>
#include <mapnik/feature_layer_desc.hpp>

#include <boost/optional.hpp>
#include <memory>
#include <string>

class mssql_datasource : public mapnik::datasource
{
public:
    mssql_datasource(mapnik::parameters const& params);
    virtual ~mssql_datasource();
    
    // mandatory
    mapnik::datasource::datasource_t type() const;
    static const char * name();
    mapnik::featureset_ptr features(mapnik::query const& q) const;
    mapnik::featureset_ptr features_at_point(mapnik::coord2d const& pt, double tol = 0) const;
    mapnik::box2d<double> envelope() const;
    boost::optional<mapnik::datasource_geometry_t> get_geometry_type() const;
    mapnik::layer_descriptor get_descriptor() const;
    
private:
    std::string connection_string_;
    std::string table_;
    std::string geometry_field_;
    int srid_;
    std::string sort_field_;
    std::string sort_order_;
    mapnik::box2d<double> extent_;
    std::string encoding_;
    mapnik::layer_descriptor desc_;
    std::unique_ptr<mssql_connection> connection_;
    std::string box_to_sqlgeom(mapnik::box2d<double> const& box, int srid) const;
};


#endif // MSSQL_DATASOURCE_HPP