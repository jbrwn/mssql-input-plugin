
//mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/map.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/feature_type_style.hpp>

#include <cmath>
#include <ctime>

void render(mapnik::Map& map, mapnik::box2d<double>& extent, mapnik::value_integer z)
{
    int n = std::pow(2, z);
    double w = extent.width();
    double h = extent.height();
    double wsize = w / n;
    double hsize = h / n;

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            double minx = extent.minx() + (i * wsize);
            double maxx = extent.minx() + ((i + 1) * wsize);
            double miny = extent.miny() + (j * hsize);
            double maxy = extent.miny() + ((j + 1) * hsize);
            
            mapnik::box2d<double> zoom(minx, miny, maxx, maxy);
            map.zoom_to_box(zoom);
            mapnik::image_rgba8 im(map.width(), map.height());
            mapnik::agg_renderer<mapnik::image_rgba8> ren(map, im);
            ren.apply();
        }
    }
}

void parse_args(int argc, char** argv, mapnik::parameters & params)
{
    if (argc > 0) {
        for (int i = 1; i < argc; ++i)
        {
            std::string opt(argv[i]);
            // parse --foo bar
            if (!opt.empty() && (opt.find("--") != 0))
            {
                std::string key = std::string(argv[i - 1]);
                if (!key.empty() && (key.find("--") == 0)) {
                    key = key.substr(key.find_first_not_of("-"));
                    params[key] = opt;
                }
            }
        }
    }
}


int main (int argc, char** argv)
{
    mapnik::parameters p;
    parse_args(argc, argv, p);
    boost::optional<std::string> plugin = p.get<std::string>("plugin");
    boost::optional<std::string> xml = p.get<std::string>("xml");
    boost::optional<std::string> extent = p.get<std::string>("extent");
    boost::optional<mapnik::value_integer> z = p.get<mapnik::value_integer>("z");
    if (!plugin || !xml || !z || !extent)
    {
        std::clog << "incorrect benchmark params" << "\n";
        return -1;
    }

    try
    {
        mapnik::datasource_cache::instance().register_datasource(*plugin);
        mapnik::box2d<double> ext;
        ext.from_string(*extent);
        mapnik::Map m(256, 256);
        mapnik::load_map(m, *xml);

        std::clock_t start = clock();
        render(m, ext, *z);
        clock_t end = clock();
        double elapsed = double(end - start) / CLOCKS_PER_SEC;
        std::cout << elapsed << "\n";
    }
    catch (std::exception const& ex)
    {
        std::clog << ex.what() << "\n";
        return -1;
    }

    return 0;
}


