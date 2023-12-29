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

/*

Building this requires the following linker flags:

-Wl,--section-start=.beforedata=0x800100
-Wl,--section-start=.data=0x800669
    
*/
extern __attribute__((section(".beforedata"))) struct vm_t
{
    uint8_t stack[256];    // 0x100
    uint8_t globals[1024]; // 0x200
    uint24_t calls[32];    // 0x600
    uint8_t sp;            // 0x660
    uint24_t pc;           // 0x661
    uint8_t csp;           // 0x664
    uint8_t error;         // 0x665
    uint8_t frame_dur;     // 0x666
    uint8_t frame_start;   // 0x667
    bool just_rendered;    // 0x668
} vm;

void vm_run();

}
