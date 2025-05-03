#include "ards_compiler.hpp"

#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>

namespace ards
{

/*
tilemap:
    u8  dtype;   // tile data type -- 1: u8, 2: u16
    u16 nrow;    // (little endian) tile row count
    u16 ncol;    // (little endian) tile column count
    [tile data]  // (big endian if u16 format)
*/

enum tilemap_format_t
{
    TMAPF_U8,
    TMAPF_U16,
};

std::string compiler_t::encode_tilemap_literal(
    std::vector<uint8_t>& data, ast_node_t const& n)
{
    assert(n.children.size() > 2);
    assert(n.children[0].type == AST::INT_CONST);
    assert(n.children[1].type == AST::INT_CONST);
    assert(n.children[2].type == AST::LIST);
    int64_t ncol = n.children[0].value;
    int64_t nrow = n.children[1].value;
    tilemap_format_t format = TMAPF_U8;

    if(nrow < 0 || nrow > 65535)
        return "Tilemap height must be in the range [0, 65535]";
    if(ncol < 0 || ncol > 65535)
        return "Tilemap width must be in the range [0, 65535]";

    auto const& tiles = n.children[2].children;
    // TODO: numbers in error message
    if(size_t(nrow * ncol) != tiles.size())
        return "Incorrect number of tiles for tilemap dimension";

    for(auto const& t : tiles)
    {
        if(t.type != AST::INT_CONST)
            return "Tile value \"" + std::string(t.data) + "\" is not an integral constant";
        if(t.value < 0 || t.value > 65535)
            return "Tile value \"" + std::string(t.data) + "\" is not in the range [0, 65535]";
    }

    // write data
    data.push_back((uint8_t)(format + 1));
    data.push_back(uint8_t(nrow >> 0));
    data.push_back(uint8_t(nrow >> 8));
    data.push_back(uint8_t(ncol >> 0));
    data.push_back(uint8_t(ncol >> 8));
    for(auto const& t : tiles)
    {
        if(format >= TMAPF_U16)
            data.push_back(uint8_t(t.value >> 8));
        data.push_back(uint8_t(t.value >> 0));
    }

    return "";
}

std::string compiler_t::encode_tilemap_tmx(
    std::vector<uint8_t>& data, std::string const& filename)
{
    tmx::Map map;

    std::vector<char> d;
    std::string path = current_path + "/" + filename;
    if(!file_loader || !file_loader(path, d))
        return "Unable to open \"" + filename + "\"";

    if(!map.loadFromString(std::string(d.begin(), d.end()), current_path))
        return "Unable to load tilemap: \"" + filename + "\"";

    tilemap_format_t format = TMAPF_U8;
    unsigned int nrow, ncol;
    for(auto const& layer : map.getLayers())
    {
        if(layer->getType() != tmx::Layer::Type::Tile)
            continue;
        auto const& tile_layer = layer->getLayerAs<tmx::TileLayer>();
        if(!tile_layer.getChunks().empty())
            return "Infinite tilemaps not supported: \"" + filename + "\"";
        auto const& tiles = tile_layer.getTiles();
        auto layer_size = tile_layer.getSize();
        nrow = layer_size.y;
        ncol = layer_size.x;
        if(nrow > UINT16_MAX)
            return "Too many rows in tilemap: \"" + filename + "\"";
        if(ncol > UINT16_MAX)
            return "Too many columns in tilemap: \"" + filename + "\"";
        for(auto const& t : tiles)
        {
            if(t.ID >= 256) format = TMAPF_U16;
            if(t.ID >= 65536)
                return "Too many tile IDs in tilemap: \"" + filename + "\"";
        }

        // write data
        data.push_back((uint8_t)(format + 1));
        data.push_back(uint8_t(nrow >> 0));
        data.push_back(uint8_t(nrow >> 8));
        data.push_back(uint8_t(ncol >> 0));
        data.push_back(uint8_t(ncol >> 8));
        for(auto const& t : tiles)
        {
            if(format >= TMAPF_U16)
                data.push_back(uint8_t(t.ID >> 8));
            data.push_back(uint8_t(t.ID >> 0));
        }

        return "";
    }
    
    return "Tilemap has no tile layers: \"" + filename + "\"";
}

}
