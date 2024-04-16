#include "ards_compiler.hpp"

#include <stdio.h>

int main(int argc, char** argv)
{
    if(argc < 5)
    {
        fprintf(stderr, "Not enough arguments\n");
        return 1;
    }

    ards::compiler_t c{};
    std::vector<uint8_t> data;
    std::vector<char> ttf;
    ards::ast_node_t n{};

    {
        int size = atoi(argv[2]);
        if(size < 4 || size > 64)
        {
            fprintf(stderr, "Bad font size: %d\n", size);
            return 1;
        }

        n.children.resize(1);
        n.children[0].value = size;
    }

    {
        std::ifstream f(argv[3], std::ios::in | std::ios::binary);
        if(f.fail())
        {
            fprintf(stderr, "Could not open TTF file\n");
            return 1;
        }
        ttf = std::vector<char>(
            (std::istreambuf_iterator<char>(f)),
            (std::istreambuf_iterator<char>()));
    }

    c.encode_font_ttf(data, n, (uint8_t const*)ttf.data(), ttf.size());

    if(!c.errors().empty())
    {
        fprintf(stderr, "Error: %s\n", c.errors()[0].msg.c_str());
        return 1;
    }

    {
        FILE* f = fopen(argv[4], "w");
        if(!f)
        {
            fprintf(stderr, "Could not open output file\n");
            return 1;
        }
        fprintf(f, "#pragma once\n\n");
        fprintf(f, "#include <stdint.h>\n\n");
        fprintf(f, "static uint8_t const %s[] =\n{\n", argv[1]);
        for(size_t i = 0; i < data.size(); ++i)
        {
            if(i % 16 == 0)
                fprintf(f, "    ");
            fprintf(f, "%d,%c", data[i], i % 16 == 15 ? '\n' : ' ');
        }
        if(data.size() % 16 != 0)
            fprintf(f, "\n");
        fprintf(f, "};\n");
        fclose(f);
    }

    return 0;
}
