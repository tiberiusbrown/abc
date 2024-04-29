#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include "ards_compiler.hpp"

#include <algorithm>
#include <cmath>
#include <strstream>
#include <tuple>

#include <MidiFile.h>

namespace ards
{

// Quantization is 4 ms

/*
Tones Encoding
================================
period   (2 bytes)
duration (1 byte)  each duration tick is 4 ms
*/

// midi note periods (0 is silence)
static std::vector<uint16_t> periods;

static std::unordered_map<std::string, int> notes;

struct midi_tone_t
{
    int start_tick;
    int dur_ticks;
    int note;
};

bool operator<(midi_tone_t const& a, midi_tone_t const& b)
{
    return
        std::tie(a.start_tick, b.note, a.dur_ticks) <
        std::tie(b.start_tick, a.note, b.dur_ticks);
}

std::string compiler_t::encode_tones_midi(
    std::vector<uint8_t>& data, std::string const& filename, bool music)
{
    std::vector<char> d;
    std::string path = current_path + "/" + filename;
    if(!file_loader || !file_loader(path, d))
        return "Unable to open \"" + filename + "\"";

    smf::MidiFile f;
    {
        std::istrstream ss(d.data(), (int)d.size());
        if(!f.readSmf(ss))
            return "An error occurred while reading MIDI file \"" + filename + "\"";
    }

    f.doTimeAnalysis();
    f.linkNotePairs();

    // time/duration/key
    std::vector<midi_tone_t> tones;

    for(int i = 0; i < f.getTrackCount(); ++i)
    {
        auto const& track = f[i];
        for(int j = 0; j < track.size(); ++j)
        {
            auto const& ev = track[j];
            if(!ev.isNoteOn())
                continue;
            double start = ev.seconds;
            double dur = ev.getDurationInSeconds();
            int start_tick = (int)std::round(start * 250.0);
            int dur_ticks = (int)std::round((start + dur) * 250.0) - start_tick + 1;
            int k = std::clamp(ev.getKeyNumber(), 1, 127);
            tones.push_back({ start_tick, dur_ticks, k });
        }
    }

    // sort in ascending time order
    std::sort(tones.begin(), tones.end());

    std::vector<uint8_t> cdata[2];
    int ctick[2] = { 0, 0 };
    ctick[1] = music ? 0 : INT_MAX;

    for(auto tone : tones)
    {
        // skip note if entirely overlapping with prior note
        int end_tick = tone.start_tick + tone.dur_ticks;
        if(end_tick <= ctick[0] && end_tick <= ctick[1])
            continue;

        // get first available channel
        int channel = ctick[0] < ctick[1] ? 0 : 1;
        auto& cd = cdata[channel];
        int tick = ctick[channel];

        // adjust tone if it partially overlaps with prior note
        if(tone.start_tick < tick)
        {
            tone.dur_ticks -= (tick - tone.start_tick);
            tone.start_tick = tick;
        }

        while(tone.start_tick > tick)
        {
            // insert silence
            uint8_t per = 0;
            uint8_t dur = uint8_t(std::min<int>(tone.start_tick - tick, 255));
            cd.push_back(uint8_t(per >> 0));
            cd.push_back(uint8_t(dur >> 0));
            tick += dur;
        }

        int ticks_left = tone.dur_ticks;
        while(tick < tone.start_tick + tone.dur_ticks)
        {
            // insert note
            uint8_t per = (uint8_t)std::clamp(tone.note, 1, 127);
            uint8_t dur = uint8_t(std::min<int>(ticks_left, 255));
            cd.push_back(uint8_t(per >> 0));
            cd.push_back(uint8_t(dur >> 0));
            tick += dur;
            ticks_left -= dur;
        }

        ctick[channel] = tick;
    }

    for(auto& cd : cdata)
    {
        cd.push_back(255);
        cd.push_back(0);
    }

    if(music)
    {
        size_t offset = cdata[0].size();
        data.push_back(uint8_t(offset >> 0));
        data.push_back(uint8_t(offset >> 8));
        data.push_back(uint8_t(offset >> 16));
        data.insert(data.end(), cdata[0].begin(), cdata[0].end());
        data.insert(data.end(), cdata[1].begin(), cdata[1].end());
    }
    else
    {
        data = std::move(cdata[0]);
    }

    return "";
}

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
        auto e = encode_tones_midi(data, std::string(n.children[0].data), false);
        if(!e.empty())
            errs.push_back({ e, n.line_info });
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
        //uint16_t per = periods[note];
        uint8_t per = (uint8_t)note;
        int64_t ms = n.children[i + 1].value + ms_rem;
        ms_rem = ms % 4;
        int64_t ticks = ms / 4;

        while(ticks > 0)
        {
            uint8_t dur = (uint8_t)std::min<int64_t>(ticks, 255);
            ticks -= dur;
            data.push_back((uint8_t)(per >> 0));
            //data.push_back((uint8_t)(per >> 8));
            data.push_back((uint8_t)(dur >> 0));
            //data.push_back((uint8_t)(dur >> 8));
        }
    }

    data.push_back(255);
    data.push_back(0);
    //data.push_back(0);
    //data.push_back(0);
}

}
