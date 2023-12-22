#pragma once

#ifndef ARDS_TONES_BUFFER_SIZE
#define ARDS_TONES_BUFFER_SIZE 4
#endif

// tick rate is 500 Hz (2 ms)

#include <ArduboyFX.h>
#include <string.h>

#include <avr/io.h>

namespace ards
{

struct Tones
{
    static void setup();
    
    static void play(uint24_t song);
    static void stop();
    static bool playing();
    
    static void update();
};

}

#ifdef ARDS_TONES_IMPLEMENTATION

namespace ards
{

namespace detail
{

struct note_t
{
    uint16_t period;
    uint16_t ticks;
};

static volatile note_t buffer[ARDS_TONES_BUFFER_SIZE];
static volatile uint8_t buffer_size;

// current song address
static volatile uint24_t addr;

static __attribute__((naked, noinline))
void fx_read_data_bytes(uint24_t addr, void* dst, size_t num)
{
    // addr: r22,r23,r24
    // dst:  r20,r21
    // num:  r18,r19
    asm volatile(R"ASM(
            cbi   %[fxport], %[fxbit]
            ldi   r25, %[sfc_read]
            out   %[spdr], r25
            lds   r0, %[page]+0         ;  2
            add   r23, r0               ;  1
            lds   r0, %[page]+1         ;  2
            adc   r24, r0               ;  1
            rcall L%=_delay_11          ; 11
            out   %[spdr], r24
            rcall L%=_delay_17          ; 17
            out   %[spdr], r23
            rcall L%=_delay_17          ; 17
            out   %[spdr], r22
            rcall L%=_delay_17          ; 17
            out   %[spdr], r1

            ; skip straight to final read if num == 1
            movw  r26, r20              ;  1
            subi  r18, 1                ;  1
            sbci  r19, 0                ;  1
            breq  2f                    ;  1 (2)
            adiw  r30, 0                ;  2

            ; intermediate reads
        1:  rcall L%=_delay_10          ; 10
            in    r0, %[spdr]           ;  1
            out   %[spdr], r1
            st    X+, r0                ;  2
            subi  r18, 1                ;  1
            sbci  r19, 0                ;  1
            brne  1b                    ;  2 (1)

            ; final read
        2:  rcall L%=_delay_11          ; 11
            in    r0, %[spdr]
            st    X, r0
            sbi   %[fxport], %[fxbit]
            in    r0, %[spsr]
            ret

        L%=_delay_17:
            lpm
        L%=_delay_14:
            lpm
        L%=_delay_11:
            nop
        L%=_delay_10:
            lpm
        L%=_delay_7:
            ret       ; rcall is 3, ret is 4 cycles
        )ASM"
        :
        : [page]     ""  (&FX::programDataPage)
        , [sfc_read] "I" (SFC_READ)
        , [spdr]     "I" (_SFR_IO_ADDR(SPDR))
        , [spsr]     "I" (_SFR_IO_ADDR(SPSR))
        , [fxport]   "I" (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]    "I" (FX_BIT)
        );
}

static void disable()
{
    TIMSK3 = 0x00;
    TIMSK4 = 0x00;
}

static void enable()
{
    TIMSK3 = 0x02;
    TIMSK4 = 0x40;
}

static bool enabled()
{
    return TIMSK4 != 0;
}

} // namespace detail

void Tones::setup()
{
    DDRC  = 0xc0;
    PORTC = 0x80;
    
    //TCCR4A = 0x00; // disconnect sound pins
    TCCR4B = 0x09;   // CK/256
    //TCCR4D = 0x00; // normal waveform
    TC4H   = 0x00;
    OCR4A  = 125;    // 16 MHz / 256 / 125 / 2 = 250 Hz
    
    TCCR3A = 0x03; // Fast PWM 2 MHz
    TCCR3B = 0x1a; // Fast PWM 2 MHz

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
    uint8_t sreg = SREG;
    cli();
    detail::disable();
    detail::buffer_size = sizeof(detail::buffer);
    detail::fx_read_data_bytes(
        song,
        &detail::buffer[0],
        sizeof(detail::buffer));
    song += sizeof(detail::buffer);
    detail::addr = song;
    OCR3A = detail::buffer[0].period;
    detail::enable();
    SREG = sreg;
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

ISR(TIMER3_COMPA_vect, __attribute((naked)))
{
    //PINC = 0xc0;
    asm volatile(R"(
        sbi 0x06, 6
        sbi 0x06, 7
        reti
        )");
}

ISR(TIMER4_COMPA_vect)
{
    uint16_t ticks = ards::detail::buffer[0].ticks;
    if(--ticks == 0)
    {
        // should be memmove, but works for backward moves on avr
        memcpy(
            &ards::detail::buffer[0],
            &ards::detail::buffer[1],
            sizeof(ards::detail::note_t) * (ARDS_TONES_BUFFER_SIZE - 1));
        uint8_t size = ards::detail::buffer_size - sizeof(ards::detail::note_t);
        if(size == 0)
        {
            // leave ticks unmodified
            return;
        }
        ards::detail::buffer_size = size;
        uint16_t period = ards::detail::buffer[0].period;
        if(period < 256)
        {
            if((uint8_t)period == 0)
                ards::Tones::stop();
            else
                TIMSK3 = 0x00; // silence
        }
        else
        {
            OCR3A = period;
            TIMSK3 = 0x02;
        }
    }
    else
        ards::detail::buffer[0].ticks = ticks;
}

#endif
