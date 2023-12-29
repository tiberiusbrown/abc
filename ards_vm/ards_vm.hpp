#pragma once

#ifndef EEPROM_h
#define EEPROM_h
#endif
#include <ArduboyFX.h>

namespace ards
{

enum error_t : uint8_t
{
    ERR_NONE,
    ERR_SIG, // signature error
    ERR_IDX, // array index out of bounds
    ERR_DIV, // division by zero
    ERR_ASS, // assertion failed
    ERR_DST, // data stack overflow
    ERR_CST, // call stack overflow
    ERR_FRM, // sprite frame outside of set
    ERR_STR, // source and dest string overlap
    NUM_ERR,
};

constexpr uint8_t MAX_CALLS = 16;

/*

Building this requires the following linker flags:

-Wl,--section-start=.beforedata=0x800100
-Wl,--section-start=.data=0x800639
    
*/
extern __attribute__((section(".beforedata"))) struct vm_t
{
    uint8_t stack[256];        // 0x100
    uint8_t globals[1024];     // 0x200
    uint24_t calls[MAX_CALLS]; // 0x600
    uint8_t sp;                // 0x630
    uint24_t pc;               // 0x631
    uint8_t csp;               // 0x634
    uint8_t error;             // 0x635
    uint8_t frame_dur;         // 0x636
    uint8_t frame_start;       // 0x637
    bool just_rendered;        // 0x638
} vm;

void vm_run();

}
