#pragma once

#ifndef ARDS_TONES_BUFFER_SIZE
#define ARDS_TONES_BUFFER_SIZE 2
#endif

#ifndef EEPROM_h
#define EEPROM_h
#endif

#include <Arduboy2Audio.h>
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

extern volatile note_t buffer[ARDS_TONES_BUFFER_SIZE];
extern volatile uint8_t buffer_size;
}

}

#ifdef ARDS_TONES_IMPLEMENTATION

namespace ards
{

namespace detail
{

static uint16_t const PERIODS[129] PROGMEM =
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

volatile note_t buffer[ARDS_TONES_BUFFER_SIZE];
volatile uint8_t buffer_size;

// current song address
static volatile uint24_t addr;

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
    TIMSK4 = 0x00;
}

static void enable()
{
    TCCR3B = 0x1a; // Fast PWM 2 MHz
    TIMSK4 = 0x40;
}

static bool enabled()
{
    return TIMSK4 != 0;
}

} // namespace detail

void Tones::setup()
{
    //TCCR4A = 0x00; // disconnect sound pins
    TCCR4B = 0x09;   // CK/256
    //TCCR4D = 0x00; // normal waveform
    TC4H   = 0x00;
    OCR4A  = 125;    // 16 MHz / 256 / 125 / 2 = 250 Hz
    
    TCCR3A = 0x43; // Fast PWM 2 MHz, toggle OC3A on compare match
    TCCR3B = 0x18; // Fast PWM 2 MHz

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
    if(!Arduboy2Audio::enabled())
        return;
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
    uint8_t index = ards::detail::buffer[0].period;
    OCR3A = pgm_read_word(&ards::detail::PERIODS[index]);;
    PORTC = 0x80;
    detail::enable();
    if(index == 255)
        stop();
end:
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

//ISR(TIMER3_COMPA_vect, __attribute((naked)))
//{
//    //PINC = 0xc0;
//    asm volatile(R"(
//        sbi 0x06, 6
//        sbi 0x06, 7
//        reti
//        )");
//}

ISR(TIMER4_COMPA_vect, __attribute((flatten)))
{
    uint8_t ticks = ards::detail::buffer[0].ticks;
    if(--ticks == 0)
    {
        // should be memmove, but works for backward moves on avr
        memcpy(
            &ards::detail::buffer[0],
            &ards::detail::buffer[1],
            sizeof(ards::detail::note_t) * (ARDS_TONES_BUFFER_SIZE - 1));
        uint8_t size = ards::detail::buffer_size;
        size -= sizeof(ards::detail::note_t);
        if(size == 0)
        {
            // leave ticks unmodified
            return;
        }
        ards::detail::buffer_size = size;
        uint8_t index = ards::detail::buffer[0].period;
        if(index == 255)
            ards::Tones::stop();
        else if(index == 0)
        {
            TCCR3B = 0x18; // silence
            PORTC = 0;
        }
        else
        {
            uint16_t ocr = pgm_read_word(&ards::detail::PERIODS[index]);
            OCR3A = ocr;
            TCCR3B = 0x1a;
        }
    }
    else
        ards::detail::buffer[0].ticks = ticks;
}

#endif
