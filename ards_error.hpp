#pragma once

#include <string>

namespace ards
{

struct error_t
{
    std::string msg;
    size_t line;   // 0 if none
    size_t column; // 0 if none
};
    
}
