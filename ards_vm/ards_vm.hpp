#pragma once

#include <ArduboyFX.h>

namespace ards
{

extern __attribute__((section(".beforedata"))) struct vm_t
{
    uint8_t stack[256];    // 0x100
    uint8_t globals[1024]; // 0x200
    uint24_t calls[32];    // 0x600
    uint8_t sp;            // 0x660
    uint24_t pc;           // 0x661
} vm;

void vm_run();

}
