#include "ards_vm.hpp"

#ifndef EEPROM_h
#define EEPROM_h
#endif
#include <Arduboy2.h>
#include <Arduboy2Audio.h>

#include "ards_tone.hpp"

Arduboy2Base a;

ARDUBOY_NO_USB

extern "C" void vm_error(ards::error_t e);

void setup()
{
    a.boot();
    
    FX::begin(0x0000, 0xfff0);
    
    // figure out data/save pages if we are in dev mode
    if(FX::programDataPage == 0x0000)
    {
        // look for dev end signature
        uint24_t addr = uint24_t(16) * 1024 * 1024 - 4;
        FX::seekData(addr);
        uint32_t sig = FX::readPendingLastUInt32();
        if(sig != 0xABCEEABC)
        {
            // we might have a save page: look back one 4K sector
            addr -= 4096;
            FX::seekData(addr);
            sig = FX::readPendingLastUInt32();
            if(sig != 0xABCEEABC)
                vm_error(ards::ERR_SIG);
        }
        // read the data page from end-of-data
        FX::readDataBytes(
            addr - 2,
            (uint8_t*)&FX::programDataPage,
            2);
    }

    // verify start signature
    FX::seekData(0);
    if(FX::readPendingLastUInt32() != 0xABC00ABC)
        vm_error(ards::ERR_SIG);
    
    Arduboy2Audio::begin();
    ards::Tones::setup();
}

void loop()
{
    ards::vm_run();
}
