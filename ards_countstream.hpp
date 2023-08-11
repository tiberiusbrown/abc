#pragma once

#include <streambuf>

#include "ards_error.hpp"

namespace ards
{
    
struct counting_stream_buffer : public std::streambuf
{
    counting_stream_buffer(std::streambuf* b, error_t& e)
        : buf(b)
        , error(e)
        , prev_line(1)
        , prev_column(SIZE_MAX)
        , file_pos(0)
    {
        error.line = 1;
        error.column = 0;
    }
    std::streambuf* buf;
    error_t& error;
    size_t prev_line;
    size_t prev_column;
    size_t file_pos;

    // extract next character from stream
    std::streambuf::int_type uflow()
    {
        int_type rc = buf->sbumpc();
        prev_line = error.line;
        if(traits_type::eq_int_type(rc, traits_type::to_int_type('\n')))
        {
            ++error.line;
            prev_column = error.column + 1;
            error.column = SIZE_MAX;
        }
        ++error.column;
        ++file_pos;
        return rc;
    }

    // put back last character
    std::streambuf::int_type pbackfail(std::streambuf::int_type c)
    {
        if(traits_type::eq_int_type(c, traits_type::to_int_type('\n')))
        {
            --error.line;
            prev_line = error.line;
            error.column = prev_column;
            prev_column = 0;
        }
        --error.column;
        --file_pos;
        if(c != traits_type::eof())
            return buf->sputbackc(traits_type::to_char_type(c));
        else
            return buf->sungetc();
    }

    // change position by offset, according to way and mode  
    virtual std::ios::pos_type seekoff(std::ios::off_type pos,
                                  std::ios_base::seekdir dir,
                                  std::ios_base::openmode mode)
    {
        if(dir == std::ios_base::beg
         && pos == static_cast<std::ios::off_type>(0))
        {
            prev_line = 1;
            error.line = 1;
            error.column = 0;
            prev_column = SIZE_MAX;
            file_pos = 0;
            return buf->pubseekoff(pos, dir, mode);
        }
        else
            return std::streambuf::seekoff(pos, dir, mode);
    }

    // change to specified position, according to mode
    virtual std::ios::pos_type seekpos(std::ios::pos_type pos,
                                  std::ios_base::openmode mode)
    {
        if(pos == static_cast<std::ios::pos_type>(0))
        {
            prev_line = 1;
            error.line = 1;
            error.column = 0;
            prev_column = static_cast<unsigned int>(-1);
            file_pos = 0;
            return buf->pubseekpos(pos, mode);
        }
        else
            return std::streambuf::seekpos(pos, mode);
    }

};

}
