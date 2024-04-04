/*
Options

    SPRITESABC_IMPLEMENTATION
*/

#pragma once

#include <Arduboy2.h>
#include <ArduboyFX.h>

struct SpritesABC
{

    // color: zero for BLACK, 1 for WHITE
    static void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
    static void fillRect_i8(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t color);

    static constexpr uint8_t MODE_OVERWRITE   = 0;
    static constexpr uint8_t MODE_PLUSMASK    = 1;
    static constexpr uint8_t MODE_SELFMASK    = 4;
    static constexpr uint8_t MODE_OVERWRITEFX = 2;
    static constexpr uint8_t MODE_PLUSMASKFX  = 3;
    static constexpr uint8_t MODE_SELFMASKFX  = 6;

    static void drawBasic(
        int16_t x, int16_t y, uint8_t w, uint8_t h,
        uint24_t image, uint16_t frame, uint8_t mode);
};

#ifdef SPRITESABC_IMPLEMENTATION

void SpritesABC::drawBasic(
    int16_t x, int16_t y, uint8_t w, uint8_t h,
    uint24_t image, uint16_t frame, uint8_t mode)
{
    register bool     bottom     asm("r2");
    register uint8_t  col_start  asm("r3");
    register uint8_t  buf_adv    asm("r4");
    register uint8_t  shift_coef asm("r5");
    register uint16_t shift_mask asm("r6");
    register uint8_t  cols       asm("r8");
    register uint8_t  buf_data   asm("r9");
    register uint8_t  a_mode     asm("r10") = mode;
    register uint8_t  reseek     asm("r11");
    register uint16_t a_frame    asm("r12") = frame;
    register uint24_t a_image    asm("r14") = image;
    register int8_t   page_start asm("r17");
    register uint8_t  a_h        asm("r18") = h;
    register uint8_t  pages      asm("r19");
    register uint8_t  a_w        asm("r20") = w;
    register uint8_t  count      asm("r21");
    register int16_t  a_y        asm("r22") = y;
    register int16_t  a_x        asm("r24") = x;
    register uint8_t* buf        asm("r26");
    register uint8_t* bufn       asm("r30");

    asm volatile(R"ASM(
    
            cpi  %A[x], 128
            cpc  %B[x], __zero_reg__
            brlt 1f
            rjmp L%=_end
        1:
            cpi  %A[y], 64
            cpc  %B[y], __zero_reg__
            brlt 1f
            rjmp L%=_end
        1:
            movw %A[buf], %A[x]
            add  %A[buf], %[w]
            adc  %B[buf], __zero_reg__
            cp   __zero_reg__, %A[buf]
            cpc  __zero_reg__, %B[buf]
            brlt 1f
            rjmp L%=_end
        1:
            movw %A[buf], %A[y]
            add  %A[buf], %[h]
            adc  %B[buf], __zero_reg__
            cp   __zero_reg__, %A[buf]
            cpc  __zero_reg__, %B[buf]
            brlt 1f
            rjmp L%=_end
        1:
    
            ; begin initial seek
            cbi  %[fxport], %[fxbit]
            ldi  %[count], 3
            out  %[spdr], %[count]
    
            cp   %A[frame], __zero_reg__
            cpc  %B[frame], __zero_reg__
            breq 1f
            
            ; add frame offset to image
            mov  %A[shift_mask], %[h]
            lsr  %A[shift_mask]
            lsr  %A[shift_mask]
            lsr  %A[shift_mask]
            sbrc %[mode], 0
            lsl  %A[shift_mask]
            mul  %A[shift_mask], %[w]
            movw %A[shift_mask], r0
            
            mul  %A[shift_mask], %B[frame]
            add  %B[image], r0
            adc  %C[image], r1
            mul  %B[shift_mask], %A[frame]
            add  %B[image], r0
            adc  %C[image], r1
            mul  %B[shift_mask], %B[frame]
            add  %C[image], r0
            mul  %A[shift_mask], %A[frame]
            add  %A[image], r0
            adc  %B[image], r1
            clr  __zero_reg__
            adc  %C[image], __zero_reg__
        1:
    
            mov  %[col_start], %A[x]
            clr  %[bottom]
            mov  %[cols], %[w]
    
            mov  %[pages], %[h]
            lsr  %[pages]
            lsr  %[pages]
            lsr  %[pages]
            
            movw %A[bufn], %A[y]
            asr  %B[bufn]
            ror  %A[bufn]
            asr  %B[bufn]
            ror  %A[bufn]
            asr  %B[bufn]
            ror  %A[bufn]
            
            ; clip against top edge
            mov  %[page_start], %A[bufn]
            cpi  %[page_start], 0xff
            brge 1f
            com  %[page_start]
            sub  %[pages], %[page_start]
            sbrc %[mode], 0
            lsl  %[page_start]
            mul  %[page_start], %[w]
            add  %A[image], r0
            adc  %B[image], r1
            adc  %C[image], %[bottom]
            ldi  %[page_start], 0xff
        1:
        
            lds r0, %[page]+0
            add %B[image], r0
            lds r0, %[page]+1
            adc %C[image], r0
        
            ; clip against left edge
            sbrs %B[x], 7
            rjmp 2f
            add %[cols], %A[x]
            sbrs %[mode], 0
            rjmp 1f
            lsl  %A[x]
            rol  %B[x]
        1:  sub  %A[image], %A[x]
            sbc  %B[image], %B[x]
            sbc  %C[image], %[bottom]
            sbrc %B[x], 7
            inc  %C[image]
            clr  %[col_start]
        2:
        
            ; continue initial seek
            out  %[spdr], %C[image]
        
            ; compute buffer start address
            ldi  %A[buf], lo8(%[sBuffer])
            ldi  %B[buf], hi8(%[sBuffer])
            ldi  %[count], 128
            mulsu %[page_start], %[count]
            add  r0, %[col_start]
            add  %A[buf], r0
            adc  %B[buf], r1
            
            ; clip against right edge
            sub  %[count], %[col_start]
            cp   %[cols], %A[count]
            brlo 1f
            mov  %[cols], %A[count]
        1:
        
            ; clip against bottom edge
            ldi  %A[bufn], 7
            sub  %A[bufn], %[page_start]
            
            cp   %A[bufn], %[pages]
            brge 1f
            mov  %[pages], %A[bufn]
            inc  %[bottom]
        1:
            
            ; continue initial seek
            out  %[spdr], %B[image]
        
            ldi  %A[bufn], 128
            sub  %A[bufn], %[cols]
            mov  %[buf_adv], %A[bufn]
            
            ; precompute vertical shift coef and mask
            ldi  %[count], 1
            sbrc %A[y], 1
            ldi  %[count], 4
            sbrc %A[y], 0
            lsl  %[count]
            sbrc %A[y], 2
            swap %[count]
            mov  %[shift_coef], %[count]
            clr  %A[shift_mask]
            com  %A[shift_mask]
            mov  %B[shift_mask], %A[shift_mask]
            
            sbrc %[mode], 2
            rjmp 1f
            ldi  %A[bufn], 0xff
            mul  %A[bufn], %[shift_coef]
            movw %A[shift_mask], r0
            com  %A[shift_mask]
            com  %B[shift_mask]
        1:
            
            ; continue initial seek
            out  %[spdr], %A[image]
        
            ; continue initial seek
            clr  __zero_reg__
            rcall L%=_delay_16
            out  %[spdr], __zero_reg__
            rcall L%=_delay_7
            rjmp L%=_begin

        ;
        ;   RENDERING
        ;

        L%=_seek:

            ; seek subroutine
            cbi %[fxport], %[fxbit]
            ldi %A[bufn], 3
            out %[spdr], %A[bufn]
            clr __zero_reg__
            add %A[image], %[w]
            adc %B[image], __zero_reg__
            adc %C[image], __zero_reg__
            sbrc %[mode], 0
            add %A[image], %[w]
            sbrc %[mode], 0
            adc %B[image], __zero_reg__
            sbrc %[mode], 0
            adc %C[image], __zero_reg__
        L%=_seek_after_adv:
            clr %[reseek]
            cp  %[w], %[cols]
            breq .+2
            inc %[reseek]
            lpm
            out %[spdr], %C[image]
            rcall L%=_delay_17
            out %[spdr], %B[image]
            rcall L%=_delay_17
            out %[spdr], %A[image]
            rcall L%=_delay_17
            out %[spdr], __zero_reg__
            rcall L%=_delay_10
            ret
            
        L%=_delay_17:
            nop
        L%=_delay_16:
            rjmp .+0
        L%=_delay_14:
            nop
        L%=_delay_13:
            lpm
        L%=_delay_10:
            lpm
        L%=_delay_7:
            ret

        L%=_begin:

            ; initial seek
            ; clr  __zero_reg__
            ; cbi  %[fxport], %[fxbit]
            ; ldi  %A[bufn], 3
            ; out  %[spdr], %A[bufn]
            ; rcall L%=_delay_7
            ; rcall L%=_seek_after_adv
            cp   %[page_start], __zero_reg__
            brlt L%=_top
            tst  %[pages]
            brne L%=_middle_skip_reseek
            rjmp L%=_bottom_dispatch

        L%=_top:

            ; init buf
            subi %A[buf], lo8(-128)
            sbci %B[buf], hi8(-128)
            mov  %[count], %[cols]

            ; loop dispatch
            sbrc %[mode], 0
            rjmp L%=_top_loop_masked

        L%=_top_loop:

            in   %A[x], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[x], %[shift_coef]
            ld   %[buf_data], %a[buf]
            and  %[buf_data], %B[shift_mask]
            or   %[buf_data], r1
            st   %a[buf]+, %[buf_data]
            lpm
            rjmp .+0
            dec  %[count]
            brne L%=_top_loop
            rjmp L%=_top_loop_done

        L%=_top_loop_masked:

            in   %A[x], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[x], %[shift_coef]
            movw %A[x], r0
            rcall L%=_delay_13
            in   %A[shift_mask], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[shift_mask], %[shift_coef]
            movw %A[shift_mask], r0
            ld   %[buf_data], %a[buf]
            com  %B[shift_mask]
            and  %[buf_data], %B[shift_mask]
            or   %[buf_data], %B[x]
            st   %a[buf]+, %[buf_data]
            lpm
            dec  %[count]
            brne L%=_top_loop_masked

        L%=_top_loop_done:

            ; decrement pages, reset buf back
            clr __zero_reg__
            sub  %A[buf], %[cols]
            sbc  %B[buf], __zero_reg__
            dec  %[pages]
            brne L%=_middle
            rjmp L%=_finish

        L%=_middle:

            ; only seek again if necessary
            tst  %[reseek]
            breq L%=_middle_skip_reseek
            in   r0, %[spsr]
            sbi  %[fxport], %[fxbit]
            rcall L%=_seek

        L%=_middle_skip_reseek:

            movw %[bufn], %[buf]
            subi %A[bufn], lo8(-128)
            sbci %B[bufn], hi8(-128)

        L%=_middle_loop_outer:

            mov  %[count], %[cols]

            ; loop dispatch
            sbrc %[mode], 0
            rjmp L%=_middle_loop_inner_masked

        L%=_middle_loop_inner:

            ; write one page from image to buf/buf+128
            in   %A[x], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[x], %[shift_coef]
            ld   %[buf_data], %a[buf]
            and  %[buf_data], %A[shift_mask]
            or   %[buf_data], r0
            st   %a[buf]+, %[buf_data]
            ld   %[buf_data], %a[bufn]
            and  %[buf_data], %B[shift_mask]
            or   %[buf_data], r1
            st   %a[bufn]+, %[buf_data]
            dec  %[count]
            brne L%=_middle_loop_inner
            rjmp L%=_middle_loop_outer_next

        L%=_middle_loop_inner_masked:

            ; write one page from image to buf/buf+128
            in   %A[x], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[x], %[shift_coef]
            movw %A[x], r0
            ld   %[buf_data], %a[buf]
            ld   %B[shift_mask], %a[bufn]
            rcall L%=_delay_7
            rjmp .+0
            in   %A[shift_mask], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[shift_mask], %[shift_coef]
            com  r0
            and  %[buf_data], r0
            or   %[buf_data], %A[x]
            st   %a[buf]+, %[buf_data]
            com  r1
            and  %B[shift_mask], r1
            or   %B[shift_mask], %B[x]
            st   %a[bufn]+, %B[shift_mask]
            nop
            dec  %[count]
            brne L%=_middle_loop_inner_masked

        L%=_middle_loop_outer_next:

            ; advance buf to the next page
            clr  __zero_reg__
            add  %A[buf], %[buf_adv]
            adc  %B[buf], __zero_reg__
            dec  %[pages]
            brne L%=_middle

        L%=_bottom:

            tst  %[bottom]
            breq L%=_finish

            ; seek if needed
            tst  %[reseek]
            breq L%=_bottom_dispatch
            in   r0, %[spsr]
            sbi  %[fxport], %[fxbit]
            rcall L%=_seek
            lpm

        L%=_bottom_dispatch:

            ; loop dispatch
            sbrc %[mode], 0
            rjmp L%=_bottom_loop_masked

        L%=_bottom_loop:

            ; write one page from image to buf
            in   %A[x], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[x], %[shift_coef]
            ld   %[buf_data], %a[buf]
            and  %[buf_data], %A[shift_mask]
            or   %[buf_data], r0
            st   %a[buf]+, %[buf_data]
            lpm
            rjmp .+0
            dec  %[cols]
            brne L%=_bottom_loop
            rjmp L%=_finish

        L%=_bottom_loop_masked:

            ; write one page from image to buf
            in   %A[x], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %A[x], %[shift_coef]
            movw %A[x], r0
            rcall L%=_delay_13
            in   %[pages], %[spdr]
            out  %[spdr], __zero_reg__
            mul  %[pages], %[shift_coef]
            mov  %[pages], r0
            ld   %[buf_data], %a[buf]
            com  %[pages]
            and  %[buf_data], %[pages]
            or   %[buf_data], %A[x]
            st   %a[buf]+, %[buf_data]
            lpm
            dec  %[cols]
            brne L%=_bottom_loop_masked
            lpm

        L%=_finish:

            clr  __zero_reg__
            sbi  %[fxport], %[fxbit]
            in   r0, %[spsr]
            
        L%=_end:

        )ASM"
        
        : [pages]      "=&r" (pages)
        , [shift_coef] "=&r" (shift_coef)
        , [shift_mask] "=&r" (shift_mask)
        , [page_start] "=&a" (page_start)
        , [cols]       "=&r" (cols)
        , [col_start]  "=&r" (col_start)
        , [bottom]     "=&r" (bottom)
        , [buf]        "=&x" (buf)
        , [bufn]       "=&z" (bufn)
        , [buf_adv]    "=&r" (buf_adv)
        , [x]          "+&r" (a_x)
        , [y]          "+&r" (a_y)
        , [image]      "+&r" (a_image)
        , [reseek]     "=&r" (reseek)
        , [buf_data]   "=&r" (buf_data)
        , [count]      "=&a" (count)
        
        : [frame]      "r"   (a_frame)
        , [mode]       "r"   (a_mode)
        , [w]          "r"   (a_w)
        , [h]          "r"   (a_h)
        , [sBuffer]    "i"   (Arduboy2Base::sBuffer)
        , [fxport]     "I"   (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]      "I"   (FX_BIT)
        , [spdr]       "I"   (_SFR_IO_ADDR(SPDR))
        , [spsr]       "I"   (_SFR_IO_ADDR(SPSR))
        , [page]       "i"   (&FX::programDataPage)
        
        : "memory"
                   
        );
}

void SpritesABC::fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
    if(x >= 128) return;
    if(x + w <= 0) return;
    if(y + h <= 0) return;
    fillRect_i8((int8_t)x, (int8_t)y, w, h, color);
}

// from Mr. Blinky's ArduboyFX library
static __attribute__((always_inline)) uint8_t SpritesABC_bitShiftLeftMaskUInt8(uint8_t bit)
{
    uint8_t result;
    asm volatile(
        "ldi    %[result], 1    \n" // 0 = 000 => 1111 1111 = -1
        "sbrc   %[bit], 1       \n" // 1 = 001 => 1111 1110 = -2
        "ldi    %[result], 4    \n" // 2 = 010 => 1111 1100 = -4
        "sbrc   %[bit], 0       \n" // 3 = 011 => 1111 1000 = -8
        "lsl    %[result]       \n"
        "sbrc   %[bit], 2       \n" // 4 = 100 => 1111 0000 = -16
        "swap   %[result]       \n" // 5 = 101 => 1110 0000 = -32
        "neg    %[result]       \n" // 6 = 110 => 1100 0000 = -64
        :[result] "=&d" (result)    // 7 = 111 => 1000 0000 = -128
        :[bit]    "r"   (bit)
        :
    );
    return result;
}

void SpritesABC::fillRect_i8(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    if(w == 0 || h == 0) return;
    if(y >= 64)  return;
    if(x + w <= 0) return;
    if(y + h <= 0) return;

    if(color & 1) color = 0xff;

    // clip coords
    uint8_t xc = x;
    uint8_t yc = y;

    // TODO: extreme clipping behavior

    // clip
    if(y < 0)
        h += y, yc = 0;
    if(x < 0)
        w += x, xc = 0;
    if(h >= uint8_t(64 - yc))
        h = 64 - yc;
    if(w >= uint8_t(128 - xc))
        w = 128 - xc;
    uint8_t y1 = yc + h;

    uint8_t c0 = SpritesABC_bitShiftLeftMaskUInt8(yc); // 11100000
    uint8_t m1 = SpritesABC_bitShiftLeftMaskUInt8(y1); // 11000000
    uint8_t m0 = ~c0; // 00011111
    uint8_t c1 = ~m1; // 00111111

    uint8_t r0 = yc;
    uint8_t r1 = y1 - 1;
    asm volatile(
        "lsr %[r0]\n"
        "lsr %[r0]\n"
        "lsr %[r0]\n"
        "lsr %[r1]\n"
        "lsr %[r1]\n"
        "lsr %[r1]\n"
        : [r0] "+&r" (r0),
          [r1] "+&r" (r1));

    uint8_t* buf = Arduboy2Base::sBuffer;
    asm volatile(
        "mul %[r0], %[c128]\n"
        "add %A[buf], r0\n"
        "adc %B[buf], r1\n"
        "clr __zero_reg__\n"
        "add %A[buf], %[x]\n"
        "adc %B[buf], __zero_reg__\n"
        :
        [buf]  "+&e" (buf)
        :
        [r0]   "r"   (r0),
        [x]    "r"   (xc),
        [c128] "r"   ((uint8_t)128)
        );

    uint8_t rows = r1 - r0; // middle rows + 1
    uint8_t f = 0;
    uint8_t bot = c1;
    if(m0  == 0) ++rows; // no top fragment
    if(bot == 0) ++rows; // no bottom fragment
    c0 &= color;
    c1 &= color;

    uint8_t col;
    uint8_t buf_adv = 128 - w;

#ifdef ARDUINO_ARCH_AVR
    asm volatile(R"ASM(
            tst  %[rows]
            brne L%=_top
            or   %[m1], %[m0]
            and  %[c1], %[c0]
            rjmp L%=_bottom_loop

        L%=_top:
            tst  %[m0]
            breq L%=_middle
            mov  %[col], %[w]

        L%=_top_loop:
            ld   __tmp_reg__, %a[buf]
            and  __tmp_reg__, %[m0]
            or   __tmp_reg__, %[c0]
            st   %a[buf]+, __tmp_reg__
            dec  %[col]
            brne L%=_top_loop
            add  %A[buf], %[buf_adv]
            adc  %B[buf], __zero_reg__

        L%=_middle:
            dec  %[rows]
            breq L%=_bottom

        L%=_middle_outer_loop:
            mov  %[col], %[w]
            sbrs %[col], 0
            rjmp L%=_middle_inner_loop
            inc  %[col]
            rjmp L%=_middle_inner_loop_odd
            
        L%=_middle_inner_loop:
            st   %a[buf]+, %[color]
        L%=_middle_inner_loop_odd:
            st   %a[buf]+, %[color]
            subi %[col], 2
            brne L%=_middle_inner_loop
            add  %A[buf], %[buf_adv]
            adc  %B[buf], __zero_reg__
            dec  %[rows]
            brne L%=_middle_outer_loop

        L%=_bottom:
            tst  %[bot]
            breq L%=_finish

        L%=_bottom_loop:
            ld   __tmp_reg__, %a[buf]
            and  __tmp_reg__, %[m1]
            or   __tmp_reg__, %[c1]
            st   %a[buf]+, __tmp_reg__
            dec  %[w]
            brne L%=_bottom_loop

        L%=_finish:
        )ASM"
        :
        [buf]     "+&e" (buf),
        [w]       "+&r" (w),
        [rows]    "+&r" (rows),
        [col]     "=&r" (col)
        :
        [buf_adv] "r"   (buf_adv),
        [color]   "r"   (color),
        [m0]      "r"   (m0),
        [m1]      "r"   (m1),
        [c0]      "r"   (c0),
        [c1]      "r"   (c1),
        [bot]     "r"   (bot)
        );
#else
    if(rows == 0)
    {
        m1 |= m0;
        c1 &= c0;
    }
    else
    {
        if(m0 != 0)
        {
            col = w;
            do
            {
                uint8_t t = *buf;
                t &= m0;
                t |= c0;
                *buf++ = t;
            } while(--col != 0);
            buf += buf_adv;
        }
        
        if(--rows != 0)
        {
            do
            {
                col = w;
                do
                {
                    *buf++ = color;
                } while(--col != 0);
                buf += buf_adv;
            } while(--rows != 0);
        }
    }
    
    if(bot)
    {
        do
        {
            uint8_t t = *buf;
            t &= m1;
            t |= c1;
            *buf++ = t;
        } while(--w != 0);
    }
    
#endif
}

#endif
