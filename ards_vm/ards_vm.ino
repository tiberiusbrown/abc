#include "ards_vm.hpp"

#ifndef EEPROM_h
#define EEPROM_h
#endif
#include <Arduboy2.h>
#include <Arduboy2Audio.h>

#include "ards_tone.hpp"

Arduboy2Base a;

ARDUBOY_NO_USB

void setup()
{
    a.boot();
    
    FX::begin(0);
    
    // read data page if we are in dev mode
    if(FX::programDataPage == 0)
    {
        FX::readDataBytes(
            uint24_t(16) * 1024 * 1024 - 2,
            (uint8_t*)&FX::programDataPage,
            2);
    }
    
    // figure out save page
    {
        uint16_t save_page;
        FX::readDataBytes(
            uint24_t(16) * 1024 * 1024 - 4,
            (uint8_t*)&save_page,
            2);
        if(save_page != 0)
            FX::programSavePage = (FX::programDataPage + save_page) & 0xfff0;
    }
    
    Arduboy2Audio::begin();
    ards::Tones::setup();
}

void loop()
{
    ards::vm_run();
}
