#pragma once

#ifndef ARDS_TONES_BUFFER_SIZE
#define ARDS_TONES_BUFFER_SIZE 2
#endif

#ifndef EEPROM_h
#define EEPROM_h
#endif

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

namespace detail
{
void fx_read_data_bytes(uint24_t addr, void* dst, size_t num);

struct note_t
{
    uint8_t period;
    uint8_t ticks;
};

struct channel_t
{
    note_t buffer[ARDS_TONES_BUFFER_SIZE];
    uint8_t size;
    uint24_t addr;
    bool active;
};

extern volatile channel_t c0; // timer3 channel
extern volatile channel_t c1; // timer4 channel
extern volatile channel_t c2; // timer4 channel #2 for sfx
extern volatile bool c2_active;
extern volatile bool reload_needed;
}

}

#ifdef ARDS_TONES_IMPLEMENTATION

namespace ards
{

namespace detail
{

static uint16_t const TIMER3_PERIODS[129] PROGMEM =
{
    0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xfd19, 0xeee4, 0xe17c, 0xd4d4, 0xc8e2,
    0xbd9c, 0xb2f7, 0xa8ec, 0x9f71, 0x967e, 0x8e0c, 0x8613, 0x7e8c,
    0x7772, 0x70be, 0x6a6a, 0x6471, 0x5ece, 0x597c, 0x5476, 0x4fb8,
    0x4b3f, 0x4706, 0x4309, 0x3f46, 0x3bb9, 0x385f, 0x3535, 0x3238,
    0x2f67, 0x2cbe, 0x2a3b, 0x27dc, 0x259f, 0x2383, 0x2185, 0x1fa3,
    0x1ddd, 0x1c2f, 0x1a9a, 0x191c, 0x17b3, 0x165f, 0x151d, 0x13ee,
    0x12d0, 0x11c1, 0x10c2, 0x0fd2, 0x0eee, 0x0e18, 0x0d4d, 0x0c8e,
    0x0bda, 0x0b2f, 0x0a8f, 0x09f7, 0x0968, 0x08e1, 0x0861, 0x07e9,
    0x0777, 0x070c, 0x06a7, 0x0647, 0x05ed, 0x0598, 0x0547, 0x04fc,
    0x04b4, 0x0470, 0x0431, 0x03f4, 0x03bc, 0x0386, 0x0353, 0x0324,
    0x02f6, 0x02cc, 0x02a4, 0x027e, 0x025a, 0x0238, 0x0218, 0x01fa,
    0x01de, 0x01c3, 0x01aa, 0x0192, 0x017b, 0x0166, 0x0152, 0x013f,
    0x012d, 0x011c, 0x010c, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
    0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
    0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
    0x0100,
};

volatile channel_t c0; // timer3 channel
volatile channel_t c1; // timer4 channel
volatile channel_t c2; // timer4 channel #2 for sfx
volatile bool c2_active;
volatile bool reload_needed;

__attribute__((naked, noinline))
void fx_read_data_bytes(uint24_t addr, void* dst, size_t num)
{
    // addr: r22,r23,r24
    // dst:  r20,r21
    // num:  r18,r19
    asm volatile(R"ASM(
            cbi   %[fxport], %[fxbit]
            ldi   r25, %[sfc_read]
            out   %[spdr], r25
            in    r25, %[sreg]          ;  1
            lds   r0, %[page]+0         ;  2
            add   r23, r0               ;  1
            lds   r0, %[page]+1         ;  2
            adc   r24, r0               ;  1
            rcall L%=_delay_10          ; 10
            out   %[spdr], r24
            rcall L%=_delay_17          ; 17
            out   %[spdr], r23
            rcall L%=_delay_17          ; 17
            out   %[spdr], r22
            rcall L%=_delay_16          ; 16
            out   %[spdr], r1

            ; skip straight to final read if num == 1
            movw  r26, r20              ;  1
            subi  r18, 1                ;  1
            sbci  r19, 0                ;  1
            rjmp .+0                    ;  2
            breq  2f                    ;  1 (2)
            rjmp .+0                    ;  2

            ; intermediate reads
        1:  rcall L%=_delay_7           ;  7
            cli                         ;  1
            out   %[spdr], r1           ;  1
            in    r0, %[spdr]           ;  1
            out   %[sreg], r25          ;  1
            st    X+, r0                ;  2
            subi  r18, 1                ;  1
            sbci  r19, 0                ;  1
            brne  1b                    ;  2 (1)

            ; final read
        2:  rcall L%=_delay_9           ;  9
            in    r0, %[spdr]
            st    X, r0
            sbi   %[fxport], %[fxbit]
            in    r0, %[spsr]
            ret

        L%=_delay_17:
            nop
        L%=_delay_16:
            rjmp .+0
        L%=_delay_14:
            lpm
        L%=_delay_11:
            nop
        L%=_delay_10:
            nop
        L%=_delay_9:
            rjmp .+0
        L%=_delay_7:
            ret       ; rcall is 3, ret is 4 cycles
        )ASM"
        :
        : [page]     ""  (&FX::programDataPage)
        , [sfc_read] "I" (SFC_READ)
        , [spdr]     "I" (_SFR_IO_ADDR(SPDR))
        , [spsr]     "I" (_SFR_IO_ADDR(SPSR))
        , [sreg]     "I" (_SFR_IO_ADDR(SREG))
        , [fxport]   "I" (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]    "I" (FX_BIT)
        );
}

static void disable()
{
    TCCR3B = 0x18;
    PORTC = 0;
    TIMSK1 = 0x00;
}

static void enable()
{
    TCCR3B = 0x1a; // Fast PWM 2 MHz
    TIMSK1 = 0x02;
}

static bool enabled()
{
    return TIMSK1 != 0;
}

static void update_timer3(uint8_t index)
{
    if(index == 255)
    {
        TCCR3B = 0x18; // silence
        c0.active = false;
    }
    else if(index == 0)
    {
        TCCR3B = 0x18; // silence
        PORTC = 0;
    }
    else
    {
        uint16_t ocr = pgm_read_word(&TIMER3_PERIODS[index]);
        OCR3A = ocr;
        TCCR3B = 0x1a;
    }
}

static void update_timer4(uint8_t index)
{
    // TODO
}

} // namespace detail

void Tones::setup()
{
    // timer1: CTC 250 Hz (prescaler /256, top 249)
    TCCR1A = 0x00;
    TCCR1B = 0x0c;
    OCR1A = 249;
    
    // timer3: fast PWM 2 MHz, toggle OC3A on compare match
    TCCR3A = 0x43;

    stop();
}

void Tones::stop()
{
    detail::disable();
    // prevent update from doing anything
    detail::c0.size = sizeof(detail::c0.buffer);
}

namespace detail
{

static void update_channel(volatile channel_t& c)
{
    if(!c.active) return;
    uint8_t size = c.size;
    uint8_t bytes = sizeof(c.buffer) - size;
    if(bytes > 0)
    {
        uint24_t addr = detail::c0.addr;
        detail::fx_read_data_bytes(
            addr,
            (uint8_t*)&c.buffer[0] + size,
            bytes);
        addr += bytes;
        c.addr = addr;
        c.size = sizeof(c.buffer);
    }
}

}

void Tones::play(uint24_t song)
{
    detail::disable();

    uint8_t sreg = SREG;
    cli();

    {
        auto& c = detail::c0.active ? detail::c1 : detail::c0;
        c.size = 0;
        c.addr = song;
        c.active = true;
        detail::update_channel(c);
    }

    SREG = sreg;

    detail::update_timer3(detail::c0.buffer[0].period);
    detail::enable();
}

bool Tones::playing()
{
    return detail::enabled();
}

void Tones::update()
{
    if(!detail::enabled())
        detail::reload_needed = false;
    if(!detail::reload_needed)
        return;
    uint8_t sreg = SREG;
    cli();

    detail::update_channel(detail::c0);

    detail::reload_needed = false;
    SREG = sreg;
}

namespace detail
{
struct advance_info_t
{
    bool update;
    uint8_t period;
};

static advance_info_t advance_channel(volatile channel_t& c)
{
    uint8_t ticks = c.buffer[0].ticks;
    if(--ticks == 0)
    {
        uint8_t size = c.size;
        size -= sizeof(ards::detail::note_t);
        if(size == 0)
        {
            // we have no more data available: leave ticks unmodified,
            // and the current note will continue playing until more data
            // is loaded into the buffer
            return;
        }
        // should be memmove, but works for backward moves on avr
        memcpy(
            &c.buffer[0],
            &c.buffer[1],
            sizeof(ards::detail::note_t) * (ARDS_TONES_BUFFER_SIZE - 1));
        ards::detail::reload_needed = true;
        c.size = size;
        return { true, c.buffer[0].period };
    }
    c.buffer[0].ticks = ticks;
    return { false, 0 };
}
}

} // namespace ards

ISR(TIMER1_COMPA_vect)
{
    ards::detail::advance_info_t a;
    
    a = ards::detail::advance_channel(ards::detail::c0);
    if(a.update) ards::detail::update_timer3(a.period);
    
    a = ards::detail::advance_channel(ards::detail::c1);
    if(ards::detail::c2.active)
        a = ards::detail::advance_channel(ards::detail::c2);
    if(a.update) ards::detail::update_timer4(a.period);
}

#endif
