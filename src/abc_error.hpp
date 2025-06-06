#pragma once

#include <string>

namespace abc
{

struct error_t
{
    std::string msg;
    std::pair<size_t, size_t> line_info;
};
    
}
