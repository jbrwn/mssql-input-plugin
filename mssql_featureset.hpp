#ifndef MSSQL_FEATURESET_HPP
#define MSSQL_FEATURESET_HPP

#include <mssql_statement.hpp>

// mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/unicode.hpp>

class mssql_featureset : public mapnik::Featureset
{
public:
    mssql_featureset(std::shared_ptr<mssql_statement> const& stmt,
                     mapnik::context_ptr const& ctx,
                     std::string const& encoding);
    virtual ~mssql_featureset();
    mapnik::feature_ptr next();
    
private:
    std::shared_ptr<mssql_statement> stmt_;
    mapnik::context_ptr ctx_;
    mapnik::value_integer feature_id_;
    const std::unique_ptr<mapnik::transcoder> char_tr_;
    const std::unique_ptr<mapnik::transcoder> wchar_tr_;
};

#endif // MSSQL_FEATURESET_HPP