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

#include "ards_vm.hpp"

namespace ards
{

constexpr uint16_t STANDARD_TPS = 250;

#if ABC_SHADES == 3
constexpr uint16_t GRAY_TPS = 155;
#elif ABC_SHADES == 4
constexpr uint16_t GRAY_TPS = 155;
#endif

#if ABC_SHADES == 2
constexpr uint16_t TPS = STANDARD_TPS;
#else
constexpr uint8_t TPS_FACTOR = 32;
constexpr uint16_t TPS = GRAY_TPS * TPS_FACTOR;
constexpr uint8_t TPS_TONE_COUNTER = (TPS + STANDARD_TPS / 2) / STANDARD_TPS;
#endif

struct Tones
{
    static void setup();
    
    static void update();

    // Tones play only on the tones and/or the primary channel.
    // Tones never play on the secondary channel -- it's reserved for
    // dual-channel music playing simultaneously with tones.

    // When both music (primary+secondary) and tones (tones) are playing,
    // The tones channel is given priority over the music's secondary channel,
    // but the music's secondary channel still advances.
    
    // Play a tone on the tones channel.
    static void tones_play(uint24_t t);

    // Play a tone on the primary channel.
    static void tones_play_primary(uint24_t t);

    // Play a tone on an unused channel, prioritizing the tones channel.
    static void tones_play_auto(uint24_t t);

    // Stop all playing tones.
    static void tones_stop();

    // Return true if any tones are playing.
    static bool tones_playing();

    // Play music on primary and secondary channels.
    static void music_play(uint24_t song);

    // Stop playing music.
    static void music_stop();

    // Return true if music is playing.
    static bool music_playing();

    // Stop all audio.
    static void stop();

    // Return true if any audio is playing.
    static bool playing();
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

extern volatile channel_t channels[3]; // primary, secondary, tones channels
extern volatile bool reload_needed;
extern volatile bool music_active;
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

static uint16_t const TIMER4_PERIODS[129] PROGMEM =
{
    0x0000, 0xc385, 0xc352, 0xc323, 0xc2f5, 0xc2cb, 0xc2a3, 0xc27d,
    0xc259, 0xc237, 0xc217, 0xb3f3, 0xb3bb, 0xb385, 0xb352, 0xb323,
    0xb2f5, 0xb2cb, 0xb2a3, 0xb27d, 0xb259, 0xb237, 0xb217, 0xa3f3,
    0xa3bb, 0xa385, 0xa352, 0xa323, 0xa2f5, 0xa2cb, 0xa2a3, 0xa27d,
    0xa259, 0xa237, 0xa217, 0x93f3, 0x93bb, 0x9385, 0x9352, 0x9323,
    0x92f5, 0x92cb, 0x92a3, 0x927d, 0x9259, 0x9237, 0x9217, 0x83f3,
    0x83bb, 0x8385, 0x8352, 0x8323, 0x82f5, 0x82cb, 0x82a3, 0x827d,
    0x8259, 0x8237, 0x8217, 0x73f3, 0x73bb, 0x7385, 0x7352, 0x7323,
    0x72f5, 0x72cb, 0x72a3, 0x727d, 0x7259, 0x7237, 0x7217, 0x63f3,
    0x63bb, 0x6385, 0x6352, 0x6323, 0x62f5, 0x62cb, 0x62a3, 0x627d,
    0x6259, 0x6237, 0x6217, 0x53f3, 0x53bb, 0x5385, 0x5352, 0x5323,
    0x52f5, 0x52cb, 0x52a3, 0x527d, 0x5259, 0x5237, 0x5217, 0x43f3,
    0x43bb, 0x4385, 0x4352, 0x4323, 0x42f5, 0x42cb, 0x42a3, 0x427d,
    0x4259, 0x4237, 0x4217, 0x33f3, 0x33bb, 0x3385, 0x3352, 0x3323,
    0x32f5, 0x32cb, 0x32a3, 0x327d, 0x3259, 0x3237, 0x3217, 0x23f3,
    0x23bb, 0x2385, 0x2352, 0x2323, 0x22f5, 0x22cb, 0x22a3, 0x227d,
    0x2259,
};

volatile channel_t channels[3]; // primary, secondary, tones channels
volatile bool reload_needed;
volatile bool music_active;

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
    PORTC = 0;
#if ABC_SHADES == 2
    TIMSK1 = 0x00;
#endif
}

static void enable()
{
#if ABC_SHADES == 2
    TIMSK1 = 0x02;
#endif
}

static bool enabled()
{
#if ABC_SHADES == 2
    return TIMSK1 != 0;
#else
    return true;
#endif
}

static void check_all_channels()
{
#if ABC_SHADES == 2
    if(channels[0].active || channels[1].active)
        return;
    music_active = false;
    if(channels[2].active)
        return;
    TIMSK1 = 0x00;
    PORTC = 0;
#endif
}

static void update_timer3(uint8_t index)
{
    if(index >= 129)
    {
        TCCR3B = 0x18;
    }
    else if(index == 0)
    {
        TCCR3B = 0x18;
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
    if(index >= 129)
    {
        TCCR4A = 0x00;
    }
    else if(index == 0)
    {
        TCCR4A = 0x00;
        PORTC = 0;
    }
    else
    {
        uint16_t top = pgm_read_word(&TIMER4_PERIODS[index]);
        uint8_t pre;

        asm volatile(R"(
                mov  %[pre], %B[top]
                swap %[pre]
                andi %[pre], 0x0f
                andi %B[top], 0x0f
            )"
            : [top] "+&d" (top)
            , [pre] "=&d" (pre)
            );

        TCCR4B = pre;
        TC4H  = uint8_t(top >> 8);
        OCR4C = uint8_t(top >> 0);
        asm volatile(R"(
            lsr  %B[top]
            ror  %A[top]
            )"
            : [top] "+&r" (top));
        TC4H  = uint8_t(top >> 8);
        OCR4A = uint8_t(top >> 0);
        
        TCCR4A = 0x82;
    }
}

} // namespace detail

void Tones::setup()
{
    // timer1: CTC 250 Hz (prescaler /8, top 249)
    TCCR1A = 0x00;
    TCCR1B = 0x0a; // prescaler /8
    OCR1A = (16000000ul / 8) / TPS - 1;
#if ABC_SHADES > 2
    TIMSK1 = 0x02;
#endif

    // timer3: fast PWM 2 MHz, toggle OC3A on compare match
    TCCR3A = 0x43;
    TCCR3B = 0x18;

    // clear previous config of timer4
    TCCR4A = 0x00;
    TCCR4B = 0x00;
    TCCR4D = 0x00;

    stop();
}

void Tones::stop()
{
    detail::disable();

    // prevent updates from doing anything
    detail::channels[0].active = false;
    detail::channels[1].active = false;
    detail::channels[2].active = false;
    detail::reload_needed = false;
    detail::music_active = false;

    detail::update_timer3(0);
    detail::update_timer4(0);
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
        uint24_t addr = c.addr;
        detail::fx_read_data_bytes(
            addr,
            (uint8_t*)&c.buffer[0] + size,
            bytes);
        addr += bytes;
        c.addr = addr;
        c.size = sizeof(c.buffer);
    }
}

static void init_channel(uint24_t t, volatile channel_t& c)
{
    c.size = 0;
    c.addr = t;
    c.active = true;
    update_channel(c);
}

}

void Tones::tones_play(uint24_t t)
{
    detail::disable();

    uint8_t sreg = SREG;
    cli();
    detail::init_channel(t, detail::channels[2]);
    detail::update_timer4(detail::channels[2].buffer[0].period);
    SREG = sreg;

    detail::enable();
}

void Tones::tones_play_primary(uint24_t t)
{
    music_stop();

    detail::disable();

    uint8_t sreg = SREG;
    cli();
    detail::init_channel(t, detail::channels[0]);
    detail::update_timer3(detail::channels[0].buffer[0].period);
    SREG = sreg;

    detail::enable();
}

void Tones::tones_play_auto(uint24_t t)
{
    if(!detail::channels[2].active || detail::channels[0].active)
        tones_play(t);
    else
        tones_play_primary(t);
}

void Tones::tones_stop()
{
    detail::channels[2].active = false;
    if(!music_playing())
    {
        detail::channels[0].active = false;
        detail::update_timer3(0);
        detail::update_timer4(0);
    }
    detail::check_all_channels();
}

bool Tones::tones_playing()
{
#if ABC_SHADES == 2
    return playing() && (!music_playing() || detail::channels[2].active);
#else
    if(music_playing())
        return detail::channels[2].active;
    return playing();
#endif
}

bool Tones::playing()
{
#if ABC_SHADES == 2
    return detail::enabled();
#else
    return detail::channels[0].active || detail::channels[1].active || detail::channels[2].active;
#endif
}

void Tones::music_play(uint24_t song)
{
    detail::disable();

    uint24_t offset;
    detail::fx_read_data_bytes(song, &offset, 3);
    song += 3;

    uint8_t sreg = SREG;
    cli();
    detail::init_channel(song, detail::channels[0]);
    song += offset;
    detail::init_channel(song, detail::channels[1]);
    detail::update_timer3(detail::channels[0].buffer[0].period);
    detail::update_timer4(detail::channels[1].buffer[0].period);
    detail::music_active = true;
    SREG = sreg;

    detail::enable();
}

void Tones::music_stop()
{
    if(!detail::music_active) return;
    detail::music_active = false;
    detail::channels[0].active = false;
    detail::channels[1].active = false;
    detail::check_all_channels();

    detail::update_timer3(0);
    detail::update_timer4(0);
}

bool Tones::music_playing()
{
    return detail::music_active;
}

void Tones::update()
{
    if(!detail::enabled())
        detail::reload_needed = false;
    if(!detail::reload_needed)
        return;
    uint8_t sreg = SREG;

    cli();
    for(volatile auto& c : detail::channels)
        detail::update_channel(c);
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
    if(!c.active) return;
    uint8_t ticks = c.buffer[0].ticks;
    if(--ticks == 0)
    {
        ards::detail::reload_needed = true;
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
        c.size = size;
        return { true, c.buffer[0].period };
    }
    c.buffer[0].ticks = ticks;
    return { false, 0 };
}
}

} // namespace ards

#if ABC_SHADES != 2
static volatile uint8_t tps_counter = 0;
static volatile uint8_t tps_tone_counter = 0;
#endif

ISR(TIMER1_COMPA_vect)
{
#if ABC_SHADES != 2
    {
        uint8_t t = tps_counter;
        if(++t >= ards::TPS_FACTOR) t = 0;
        if(t <= 1) ards::vm.needs_render = t + 1;
        tps_counter = t;
    }
    {
        uint8_t t = tps_tone_counter;
        if(++t >= ards::TPS_TONE_COUNTER) t = 0;
        tps_tone_counter = t;
        if(t != 0) return;
    }
#endif
    
    ards::detail::advance_info_t a;

    a = ards::detail::advance_channel(ards::detail::channels[0]);
    if(a.period >= 129) ards::detail::channels[0].active = false;
    if(a.update) ards::detail::update_timer3(a.period);
    
    a = ards::detail::advance_channel(ards::detail::channels[1]);
    if(a.period >= 129) ards::detail::channels[1].active = false;
    if(ards::detail::channels[2].active)
    {
        a = ards::detail::advance_channel(ards::detail::channels[2]);
        if(a.period >= 129) ards::detail::channels[2].active = false;
    }
    if(a.update) ards::detail::update_timer4(a.period);

    ards::detail::check_all_channels();
}

#endif
