#pragma once

#ifndef STREAMABC_BUFFER_BYTES
#define STREAMABC_BUFFER_BYTES 32
#endif

#include <ArduboyFX.h>
#include <string.h>

#include <avr/io.h>

struct StreamABC
{
    static void setup();
    
    static void play(uint24_t song);
    static void stop();
    static bool playing();
    
    static void update();
};

#ifdef STREAMABC_IMPLEMENTATION

namespace detail
{

struct sample_t
{
    uint8_t samp;
    uint8_t reps;
};

constexpr size_t STREAMABC_BUFFER_SIZE =
    STREAMABC_BUFFER_BYTES / sizeof(sample_t);
    
static_assert(
    STREAMABC_BUFFER_SIZE & (STREAMABC_BUFFER_SIZE - 1) == 0,
    "StreamABC: buffer size must be a power of 2");

static volatile sample_t buffer[STREAMABC_BUFFER_SIZE];
static volatile uint8_t buffer_wr;
static volatile uint8_t buffer_rd;

// current song address
static volatile uint24_t addr;

static volatile uint8_t master_volume;

static void disable()
{
    TIMSK3 = 0x00;
    TCCR4B = 0x00;
}

static void enable()
{
    TCNT3 = 0;
    TCCR4B = 0x01;
    TIMSK3 = 0x02;
}

static bool enabled()
{
    return TIMSK3 != 0;
}

} // namespace detail

void Tones::setup()
{
    PLLFRQ = 0x56; // 96 MHz
    TCCR4A = 0x42; // PWM mode
    TCCR4D = 0x01; // dual slope PWM
    TCCR4E = 0x40; // enhanced mode
    TC4H   = 0x00;
    OCR4C  = 0x7f; // 8-bit PWM precision
    
    TCCR3A = 0x00;
    TCCR3B = 0x09; // CTC 16 MHz
    OCR3A  = 2000; // 8 kHz

    stop();
}

void Tones::stop()
{
    detail::disable();
    // prevent update from doing anything
    detail::buffer_size = sizeof(detail::buffer);
}

void Tones::play(uint24_t song)
{
    
}

bool Tones::playing()
{
    return detail::enabled();
}

void Tones::update()
{
    if(!detail::enabled() ||
        detail::buffer_size >= sizeof(detail::buffer))
        return;
    uint8_t sreg = SREG;
    cli();
    uint8_t size = detail::buffer_size;
    uint8_t bytes = sizeof(detail::buffer) - size;
    uint24_t addr = detail::addr;
    detail::fx_read_data_bytes(
        addr,
        (uint8_t*)&detail::buffer[0] + size,
        bytes);
    addr += bytes;
    detail::addr = addr;
    detail::buffer_size = sizeof(detail::buffer);
    SREG = sreg;
}

} // namespace ards

ISR(TIMER3_COMPA_vect)
{
    TC4H  = 0;
    OCR4A = 128;
}

#endif
