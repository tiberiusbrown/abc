#pragma once

#include <array>
#include <cstdint>

template<class HOST>
struct abc_interpreter
{
    enum class result { normal, idle, error };
    
    result run();
    
    uint16_t tone_playing() const { return tone; }
    bool audio_enabled() const { return audio_en; }
    bool save_exists() const { return has_save; }
    std::array<uint8_t, 1024> const& save_data() const { return saved; }
    
protected:

    uint8_t prog(uint32_t addr) { return static_cast<HOST*>(this)->prog(addr); }
    uint32_t millis() { return static_cast<HOST*>(this)->millis(); }
    uint8_t buttons() { return static_cast<HOST*>(this)->buttons(); }
    
private:

    std::array<uint8_t, 1024> display;
    std::array<uint8_t, 1024> display_buffer;
    std::array<uint8_t, 1024> saved;
    std::array<uint8_t, 1024> globals;
    std::array<uint8_t,  256> stack;
    std::array<uint32_t,  24> call_stack;
    
    uint32_t pc;
    uint32_t tone_addr;
    uint16_t tone;
    uint8_t  csp;
    uint8_t  sp;
    bool     has_save;
    bool     audio_en;
    uint8_t  buttons_prev;
    uint8_t  buttons_curr;
};

#include "abc_interpreter.inl"
