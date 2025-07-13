#include "parser_config.h"
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

Config loadConfig(const std::string& filename)
{
    Config config;
    try
    {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(filename, pt);

        config.search_port = pt.get<int>("searcher.search_port");
        config.db_name = pt.get<std::string>("database.dbname");
        config.db_user = pt.get<std::string>("database.user");
        config.db_password = pt.get<std::string>("database.password");
    }
    catch (const boost::property_tree::ini_parser::ini_parser_error& e)
    {
        std::cerr << "Error parsing INI file: " << e.what() << std::endl;
        throw;
    }
    catch (const std::exception& e)
    {
        std::cerr << "General error: " << e.what() << std::endl;
        throw;
    }

    return config;
}
