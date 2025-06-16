#pragma once

#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

#include <limits.h>

#include "abc_error.hpp"
#include <abc_instr.hpp>

namespace abc
{

constexpr size_t FILE_TABLE_STRING_LENGTH = 32;

// overloaded sysfuncs
extern std::unordered_map<std::string, std::vector<std::string>> const sys_overloads;

// map from sys function names to indices
extern std::unordered_map<std::string, sysfunc_t> const sys_names;

struct compiler_t;
struct compiler_global_t;
struct compiler_instr_t;

struct assembler_t
{
    assembler_t()
        : file_table{ { "", (uint8_t)0 } }
        , prev_pc_offset(0)
        , globals_bytes(0)
        , byte_count(256) // header page
        , saved_bytes(0)
        , code_bytes(0)
        , data_bytes(0)
        , debug_bytes(0)
        , error{}
    {}
    error_t assemble(std::istream& f);
    error_t assemble(compiler_t const& c);
    error_t add_custom_label(
        std::string const& name,
        std::vector<uint8_t> const& data);
    error_t link();
    bool has_save() const { return saved_bytes > 0; }

    size_t globals_size() const { return globals_bytes; }
    size_t save_size() const { return saved_bytes; }
    size_t code_size() const { return code_bytes; }
    size_t data_size() const { return data_bytes; }
    size_t debug_size() const { return debug_bytes; }

    int num_shades() const { return shades; }

    std::vector<uint8_t> const& data() { return linked_data; }

    bool enable_relaxing = true;

private:
    std::vector<uint8_t> linked_data;

    /*
    Line Table Command Encoding
    =========================================================
        0-127     Advance pc by N+1 bytes
        128-252   Advance line counter by N-127 lines
        253       Set file to next byte
        254       Set line counter to next two bytes
        255       Set pc to next three bytes
    */
    std::unordered_map<std::string, uint8_t> file_table;
    std::vector<uint8_t> line_table;
    uint32_t prev_pc_offset;

    void advance_pc_offset();

    std::unordered_map<std::string, std::vector<uint8_t>> custom_labels;
    
    // convert label name to node index
    std::unordered_map<std::string, size_t> labels;

    // convert label name to global data offset
    std::unordered_map<std::string, size_t> globals;

    std::string githash;
    int shades;
    size_t max_globals_bytes() const { return shades == 2 ? 1024 : 256; }

    void add_global(compiler_global_t const& global);
    void add_label(std::string const& label);
    void add_instr(compiler_instr_t const& instr, uint16_t& line,
        uint16_t& file, std::vector<std::string> const& filenames);
    void add_file(std::string const& filename);

    // current amount of global data bytes
    size_t globals_bytes;

    // whether the binary will need a save sector
    size_t saved_bytes;

    size_t code_bytes;
    size_t data_bytes;
    size_t debug_bytes;
    
    enum node_type_t : uint8_t
    {
        INSTR,       // instruction opcode
        LABEL,       // label reference
        GLOBAL,      // global label reference (raw address)
        IMM,         // immediate value
        FILE,        // file directive
        LINE,        // line directive (imm)
        CUSTOM_DATA, // add_custom_label 
    };
    
    struct node_t
    {
        size_t offset;
        node_type_t type;
        instr_t instr;
        uint32_t size; // size of object in bytes
        uint32_t imm;  // also used for label offset
        std::string label;
    };
    
    std::vector<node_t> nodes;
    size_t byte_count;
    error_t error;

    void relax_jumps();
    
    void push_instr(instr_t i);

    void push_imm(uint32_t i, uint16_t size)
    {
        nodes.push_back({ byte_count, IMM, I_NOP, size, i });
        byte_count += size;
    }

    void push_file(uint8_t file)
    {
        nodes.push_back({ byte_count, FILE, I_NOP, 0, file });
    }

    void push_line(uint16_t line)
    {
        nodes.push_back({ byte_count, LINE, I_NOP, 0, line });
    }

    void push_label(std::istream& f, bool has_offset = false, uint16_t size = 3);
    void push_label(std::string const& label, uint32_t offset = 0, uint16_t size = 3);

    void push_global(std::istream& f, bool has_offset = false, uint16_t size = 2);
    void push_global(std::string const& label, uint32_t offset = 0, uint16_t size = 2);

};
    
}
