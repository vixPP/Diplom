#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

#include <string>

struct Config
{
    int search_port = 0;
    std::string db_name;
    std::string db_user;
    std::string db_password;
};

Config loadConfig(const std::string& filename);

#endif
