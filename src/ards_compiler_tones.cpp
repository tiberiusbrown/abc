#include "ards_compiler.hpp"

#include <algorithm>
#include <cmath>

namespace ards
{

/*
Tones Encoding
================================
period - 
*/

static std::vector<uint16_t> periods;
static std::unordered_map<std::string, int> notes;

void compiler_t::encode_tones(std::vector<uint8_t>& data, ast_node_t const& n)
{
    if(periods.empty())
    {
        periods.resize(129);
        for(int i = 0; i <= 128; ++i)
        {
            double f = 440.0 * std::pow(2.0, 1.0 / 12 * (i - 69));
            double p = 1e6 / f;
            uint16_t period = (uint16_t)std::clamp<int>((int)std::round(p), 256, 65535);
            periods[i] = period;
        }
        periods[0] = 1;
    }

    static int const OCTAVE[7] = { 0, 2, 4, 5, 7, 9, 11 };
    if(notes.empty())
    {
        notes["-"] = 0;
        for(int i = 0; i < 7; ++i)
        {
            char c = "CDEFGAB"[i];
            std::string t;
            t += c;
            int const note = 60 + OCTAVE[i];
            notes[t] = note;
            for(int j = 0; j < 9; ++j)
            {
                t += char('0' + j);
                int const note2 = note + (j - 4) * 12;
                notes[t] = note2;

                if(note2 < 128 && c != 'E' && c != 'B')
                {
                    t += '#';
                    notes[t] = note2 + 1;
                    t.pop_back();
                }
                if(note2 > 1 && c != 'F' && c != 'C')
                {
                    t += 'b';
                    notes[t] = note2 - 1;
                    t.pop_back();
                }

                t.pop_back();
            }
        }
        for(auto const& [k, v] : notes)
            assert(v >= 0 && v <= 128);
    }

    assert(n.children.size() >= 1);

    if(n.children[0].type == AST::STRING_LITERAL)
    {
        errs.push_back({
            "MIDI import not supported yet",
            n.line_info });
        return;
    }

    assert(n.children.size() % 2 == 0);
    for(size_t i = 0; i < n.children.size(); i += 2)
    {
        assert(n.children[i + 0].type == AST::TOKEN);
        assert(n.children[i + 1].type == AST::INT_CONST);
    }

    int ms_rem = 0;

    for(size_t i = 0; i < n.children.size(); i += 2)
    {
        auto it = notes.find(std::string(n.children[i + 0].data));
        if(it == notes.end())
        {
            errs.push_back({
                "Unknown note: \"" + std::string(n.children[i + 0].data) + "\"",
                n.children[0].line_info });
            return;
        }
        int note = it->second;
        assert(note >= 0 && note <= 128);
        uint16_t per = periods[note];
        int64_t ms = n.children[i + 1].value + ms_rem;
        ms_rem = ms % 4;
        uint16_t dur = (uint16_t)std::clamp<int64_t>(ms / 4, 1, 65535);

        data.push_back((uint8_t)(per >> 0));
        data.push_back((uint8_t)(per >> 8));
        data.push_back((uint8_t)(dur >> 0));
        data.push_back((uint8_t)(dur >> 8));
    }

    data.push_back(0);
    data.push_back(0);
    data.push_back(0);
    data.push_back(0);
}

}
