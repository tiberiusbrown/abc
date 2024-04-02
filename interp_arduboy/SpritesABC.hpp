/*
Options

    SPRITESABC_IMPLEMENTATION
*/

#pragma once

#include <ArduboyFX.h>

struct SpritesABC
{

    static constexpr uint8_t MODE_OVERWRITE   = 0;
    static constexpr uint8_t MODE_PLUSMASK    = 1;
    static constexpr uint8_t MODE_SELFMASK    = 4;
    static constexpr uint8_t MODE_OVERWRITEFX = 2;
    static constexpr uint8_t MODE_PLUSMASKFX  = 3;
    static constexpr uint8_t MODE_SELFMASKFX  = 6;

    static void draw(
        int16_t x, int16_t y, uint8_t w, uint8_t h,
        uint24_t image, uint16_t frame, uint8_t mode);
};

#ifdef SPRITESU_IMPLEMENTATION

constexpr uint8_t FLAG_TOP         = 1 << 7;
constexpr uint8_t FLAG_BOT         = 1 << 6;
constexpr uint8_t FLAG_RESEEK      = 1 << 5;
constexpr uint8_t FLAG_NEED_RESEEK = 1 << 4;
    
#define USE_ASM 1

#if USE_ASM

__attribute__((naked, noinline))
void SpritesABC::draw(
    int16_t x, int16_t y, uint8_t w, uint8_t h,
    uint24_t image, uint16_t frame, uint8_t mode)
{    
    asm volatile(R"(
    
        ; Register Mapping
        ; ===========================
        ; x               r24 r25
        ; y               r22 r23
        ; w               r20
        ; h               r18
        ; image           r14 r15 r16
        ; frame           r12 r13
        ; mode            r10
        ; rx / buf_data   r21
        ; ry / buf_adv    r19
        ; rw              r17
        ; rh / count      r11
        ; start_page      r9
        ; num_pages       r8
        ; buf             r26 r27
        ; bufn            r28 r29
        ; image_adv       r6 r7
        ; image_data      r30 r31
        ; shift_mask      r4 r5
        ; shift_coef      r3
        
        ; Registers to Save
        ; =======================
        ; r3-r17, r28, r29
        
            push r3
            push r4
            push r5
            push r6
            push r7
            push r8
            push r9
            push r10
            push r11
            push r12
            push r13
            push r14
            push r15
            push r16
            push r17
            push r28
            push r29
        
        ;
        ; PREPARATION
        ;
        
            cpi  r24, 128
            cpc  r25, __zero_reg__
            brlt 1f
            rjmp L%=_finish
        1:
        
            cpi  r22, 64
            cpc  r23, __zero_reg__
            brlt 1f
            rjmp L%=_finish
        1:
        
            bst  r23, 7
            bld  r10, 7
        
            movw r30, r24
            add  r30, r20
            adc  r31, __zero_reg__
            cpi  r30, 1
            cpc  r31, __zero_reg__
            brge 1f
            rjmp L%=_finish
        1:  mov  r21, r24
            sbrc r25, 7
            clr  r21
            cpi  r30, 129
            cpc  r31, __zero_reg__
            brlt 1f
            ldi  r30, 128
        1:  mov  r17, r30
            sub  r17, r21
        
            set
            cpse r17, r20
            bld  r10, 4
        
            mov  r6, r20
            clr  r7
            sbrs r10, 0
            rjmp 1f
            lsl  r6
            rol  r7
        1:  sub  r14, r6
            sbc  r15, r7
            sbc  r16, __zero_reg__
        
            movw r30, r22
            add  r30, r18
            adc  r31, __zero_reg__
            cpi  r30, 1
            cpc  r31, __zero_reg__
            brge 1f
            rjmp L%=_finish
        1:  mov  r19, r22;
            sbrc r23, 7
            clr  r19

            cpi  r30, 65
            cpc  r31, __zero_reg__
            brlt 1f
            ldi  r30, 64
        1:  cpi  r30, 64
            brne 1f
            bld  r10, 6
        1:  mov  r11, r30
            sub  r11, r19
    
            ldi  r30, 1
            sbrc r22, 1
            ldi  r30, 4
            sbrc r22, 0
            lsl  r30
            sbrc r22, 2
            swap r30
            mov  r3, r30
    
            mov  r9, r19
            lsr  r9
            lsr  r9
            lsr  r9
    
            ldi  r30, 7
            mov  r8, r11
            add  r8, r30
            lsr  r8
            lsr  r8
            lsr  r8
            and  r11, r30
            brne 1f
            clt
            bld  r10, 7
        1:
            
            sbrc r10, 6
            dec  r8
            sbrc r10, 7
            dec  r8
    
            ldi  r26, lo8(%[sBuffer])
            ldi  r27, hi8(%[sBuffer])
            add  r26, r21
            adc  r27, __zero_reg__
            ldi  r30, 128
            mul  r30, r9
            add  r26, r0
            adc  r27, r1

            sub  r21, r24
            sub  r19, r22
            lsr  r19
            lsr  r19
            lsr  r19
            mul  r19, r20
            movw r30, r0
            clr  __zero_reg__
            add  r30, r21
            adc  r31, __zero_reg__
            sbrc r10, 0
            lsl  r30
            sbrc r10, 0
            rol  r31
            add  r14, r30
            adc  r15, r31
            adc  r16, __zero_reg__
    
            cp   r12, __zero_reg__
            cpc  r13, __zero_reg__
            breq 1f
            
            lsr  r18
            lsr  r18
            lsr  r18
            sbrc r10, 0
            lsl  r18
            mul  r18, r20
            movw r30, r0
            
            mul  r30, r13
            add  r15, r0
            adc  r16, r1
            mul  r31, r12
            add  r15, r0
            adc  r16, r1
            mul  r31, r13
            add  r16, r0
            mul  r30, r12
            add  r14, r0
            adc  r15, r1
            clr  __zero_reg__
            adc  r16, __zero_reg__
        1:
            
            ; buf_adv in ry
            ldi  r30, 128
            sub  r30, r17
            mov  r19, r30
        
            ldi  r31, 0xff
        
            clr  r4
            com  r4
            mov  r5, r4
            sbrc r10, 2
            rjmp 1f
            mul  r4, r3
            movw r4, r0
            com  r4
            com  r5
        1:
        
        ;
        ; RENDERING
        ;
        
        ; used:
        ;     count        1  (same as rh)
        ;     rw           1
        ;     buf_data     1  (same as rx)
        ;     image_data   2  (same as t)
        ;     shift_mask   2
        ;     shift_coef   1
        ;     buf          2
        ;     bufn         2  (kept in Y)
        ;     buf_adv      1  (same as ry)
        ;     image_adv    2
        ;     mode         1
        ;     num_pages    1
        ;     ==============
        ;     total       15
        
            lds  r0, %[page]+0
            add  r15, r0
            lds  r0, %[page]+1
            add  r16, r0
            rjmp L%=_top
        
        L%=_seek:
            in   r0, %[spsr]
            sbi  %[fxport], %[fxbit]
            cbi  %[fxport], %[fxbit]
            ldi  r30, 3
            out  %[spdr], r30
            rjmp 1f
        L%=_delay_17:
            nop
        L%=_delay_16:
            lpm
        L%=_delay_13:
            nop
        L%=_delay_12:
            nop
        L%=_delay_11:
            rjmp .+0
        L%=_delay_9:
            rjmp .+0
            ret
        1:  clr  __zero_reg__
            add  r14, r6
            adc  r15, r7
            adc  r16, __zero_reg__
            rcall L%=_delay_11
            out  %[spdr], r16
            rcall L%=_delay_17
            out  %[spdr], r15
            rcall L%=_delay_17
            out  %[spdr], r14
            rcall L%=_delay_17
            out  %[spdr], __zero_reg__
            ret
        
        ;
        ; TOP
        ;
            
        L%=_top:
        
            rcall L%=_seek
            sbrs r10, 7
            rjmp L%=_mid
            movw r24, r26
            mov  r11, r17
            sbrc r10, 0
            rjmp L%=_top_masked
            
        L%=_top_unmasked:
            
            in   r30, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r30, r3
            ld   r21, X
            and  r21, r5
            or   r21, r1
            st   X+, r21
            lpm
            rjmp .+0
            dec  r11
            brne L%=_top_unmasked
            rjmp L%=_top_done
            
        L%=_top_masked:
            
            in   r30, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r30, r3
            movw r30, r0
            rcall L%=_delay_13
            in   r4, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r4, r3
            ld   r21, X
            com  r1
            and  r21, r1
            or   r21, r31
            st   X+, r21
            rjmp .+0
            rjmp .+0
            dec  r11
            brne L%=_top_masked
            
        L%=_top_done:
            
            movw r26, r24
            set
            sbrc r10, 4
            bld  r10, 5
        
        ;
        ; MIDDLE
        ;
            
        L%=_mid:
        
            tst  r8
            brne 1f
            rjmp L%=_bot
        1:  push r28
            push r29
            clr  __zero_reg__
            ldi  r30, 128
            movw r28, r26
            add  r28, r30
            adc  r29, __zero_reg__
            sbrc r10, 0
            rjmp L%=_mid_masked
            
        L%=_mid_unmasked:
        
            sbrc r10, 5
            rcall L%=_seek
            mov  r11, r17
            rcall L%=_delay_11
        1:  in   r30, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r30, r3
            ld   r24, X
            ld   r25, Y
            and  r24, r4
            or   r24, r0
            and  r25, r5
            or   r25, r1
            st   X+, r24
            st   Y+, r25
            dec  r11
            brne 1b
            clr  __zero_reg__
            add  r26, r19
            adc  r27, __zero_reg__
            add  r28, r19
            adc  r29, __zero_reg__
            set
            sbrc r10, 4
            bld  r10, 5
            dec  r8
            brne L%=_mid_unmasked
            rjmp L%=_mid_done
        
        L%=_mid_masked:
        
            sbrc r10, 5
            rcall L%=_seek
            rcall L%=_delay_11
            mov  r11, r17
        1:  in   r30, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r30, r3
            movw r30, r0
            ld   r24, X
            ld   r25, Y
            rcall L%=_delay_9
            in   r4, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r4, r3
            com  r0
            com  r1
            and  r24, r0
            or   r24, r30
            and  r25, r1
            or   r25, r31
            st   X+, r24
            st   Y+, r25
            nop
            dec  r11
            brne 1b
            clr  __zero_reg__
            add  r26, r19
            adc  r27, __zero_reg__
            add  r28, r19
            adc  r29, __zero_reg__
            set
            sbrc r10, 4
            bld  r10, 5
            dec  r8
            brne L%=_mid_masked
            
        L%=_mid_done:
            
            pop  r29
            pop  r28
        
        ;
        ; BOTTOM
        ;
            
        L%=_bot:
        
            sbrs r10, 6
            rjmp L%=_finish
            sbrc r10, 5
            rcall L%=_seek
            rcall L%=_delay_9
            mov  r11, r17
            sbrc r10, 0
            rjmp L%=_bot_masked
            
        L%=_bot_unmasked:
            
            in   r30, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r30, r3
            ld   r21, X
            and  r21, r4
            or   r21, r0
            st   X+, r21
            lpm
            rjmp .+0
            dec  r11
            brne L%=_bot_unmasked
            rjmp L%=_finish
            
        L%=_bot_masked:
            
            in   r30, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r30, r3
            movw r30, r0
            rcall L%=_delay_13
            in   r4, %[spdr]
            out  %[spdr], __zero_reg__
            mul  r4, r3
            ld   r21, X
            com  r0
            and  r21, r0
            or   r21, r30
            st   X+, r21
            rjmp .+0
            rjmp .+0
            dec  r11
            brne L%=_bot_masked
        
        L%=_finish:
                
            rjmp .+0
            in   r0, %[spsr]
            sbi  %[fxport], %[fxbit]
            clr  __zero_reg__
            
            pop  r29
            pop  r28
            pop  r17
            pop  r16
            pop  r15
            pop  r14
            pop  r13
            pop  r12
            pop  r11
            pop  r10
            pop  r9
            pop  r8
            pop  r7
            pop  r6
            pop  r5
            pop  r4
            pop  r3

            ret
            
        )"
                
        :
        : [fxport]     "I"   (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]      "I"   (FX_BIT)
        , [spdr]       "I"   (_SFR_IO_ADDR(SPDR))
        , [spsr]       "I"   (_SFR_IO_ADDR(SPSR))
        , [spif]       "I"   (SPIF)
        , [page]       "i"   (&FX::programDataPage)
        , [sBuffer]    "i"   (&Arduboy2Base::sBuffer[0])
        
        );
}

#else

void SpritesABC::draw(
    int16_t x, int16_t y, uint8_t w, uint8_t h,
    uint24_t image, uint16_t frame, uint8_t mode)
{
    uint8_t rw;
    
    uint8_t start_page;
    uint8_t num_pages;
    
    uint8_t* buf;
    uint8_t buf_adv;
    
    uint16_t image_adv;
    
    uint8_t shift_coef;
    uint16_t shift_mask;
    
    // TEMPORARIES
    uint8_t rx, ry, rh;
    uint16_t t;
    
    uint8_t count;
    uint8_t buf_data;
    uint16_t image_data;
    uint8_t* bufn;
            
    if(x >= 128) return;
    if(y >=  64) return;
    
    // need top section for negative y
    if(y < 0)
        mode |= FLAG_TOP;
            
    {
        int16_t t = x + w;
        if(t <= 0) return;
        rx = x < 0 ? 0 : (uint8_t)x;
        if(t > 128) t = 128;
        rw = (uint8_t)t - rx;
    }
    
    // reseek flag
    if(rw != w)
        mode |= FLAG_NEED_RESEEK;
    
    // buffer advance
    buf_adv = 128 - rw;
    
    // sprite data advance
    image_adv = w;
    if(mode & MODE_PLUSMASK)
        image_adv <<= 1;
    image -= image_adv;

    t = y + h;
    if((int16_t)t <= 0) return;
    ry = y < 0 ? 0 : (uint8_t)y;
    if((int16_t)t > 64) t = 64;
    if((uint8_t)t == 64)
        mode |= FLAG_BOT;
    rh = (uint8_t)t - ry;
    
    shift_coef = 1 << ((uint8_t)y & 7);
    start_page = ry >> 3;
    
    num_pages = (rh + 7) >> 3;
    rh &= 7;
    if(!rh)
        mode &= ~(FLAG_TOP);
    if(mode & FLAG_TOP)
        --num_pages;
    if(mode & FLAG_BOT)
        --num_pages;
   
    // buffer start
    buf = Arduboy2Base::sBuffer + start_page * 128 + rx;
    
    // adjust image for sprite start
    {
        rx -= (uint8_t)x;
        ry -= (uint8_t)y;
        ry >>= 3;
        uint16_t t = ry * w;
        t += rx;
        if(mode & MODE_PLUSMASK)
            t <<= 1;
        image += t;
    }
    
    // adjust image to sprite frame
    if(frame != 0)
    {
        uint8_t p = h >> 3;
        if(mode & MODE_PLUSMASK)
            p <<= 1;
        uint16_t t = w * p;
        image += uint24_t(t) * frame;
    }
    
    // shift mask
    shift_mask = 0xffff;
    if(!(mode & MODE_SELFMASK))
        shift_mask = ~(0xff * shift_coef);
    
    //
    // RENDERING
    //
    
    image += image_adv;
    FX::seekData(image);
    
    if(mode & FLAG_TOP)
    {
        bufn = buf;
        count = rw;
        if(!(mode & MODE_PLUSMASK))
        {
            do
            {
                image_data = FX::readPendingUInt8();
                image_data = (uint8_t)image_data * shift_coef;
                buf_data = *buf;
                buf_data &= uint8_t(shift_mask >> 8);
                buf_data |= uint8_t(image_data >> 8);
                *buf++ = buf_data;
            } while(--count != 0);
        }
        else
        {
            do
            {
                image_data = FX::readPendingUInt8();
                image_data = (uint8_t)image_data * shift_coef;
                shift_mask = FX::readPendingUInt8();
                shift_mask = (uint8_t)shift_mask * shift_coef;
                buf_data = *buf;
                buf_data &= ~uint8_t(shift_mask >> 8);
                buf_data |= uint8_t(image_data >> 8);
                *buf++ = buf_data;
            } while(--count != 0);
        }
        buf = bufn;
        if(mode & FLAG_NEED_RESEEK)
            mode |= FLAG_RESEEK;
    }
    
    if(num_pages != 0)
    {
        bufn = buf + 128;
        if(!(mode & MODE_PLUSMASK))
        {
            do
            {
                if(mode & FLAG_RESEEK)
                {
                    (void)FX::readEnd();
                    image += image_adv;
                    FX::seekData(image);
                }
                
                count = rw;
                do
                {
                    image_data = FX::readPendingUInt8();
                    image_data = (uint8_t)image_data * shift_coef;
                    buf_data = *buf;
                    buf_data &= uint8_t(shift_mask >> 0);
                    buf_data |= uint8_t(image_data >> 0);
                    *buf++ = buf_data;
                    buf_data = *bufn;
                    buf_data &= uint8_t(shift_mask >> 8);
                    buf_data |= uint8_t(image_data >> 8);
                    *bufn++ = buf_data;
                } while(--count != 0);
                buf += buf_adv;
                bufn += buf_adv;
                if(mode & FLAG_NEED_RESEEK)
                    mode |= FLAG_RESEEK;
            } while(--num_pages != 0);
        }
        else
        {
            do
            {
                if(mode & FLAG_RESEEK)
                {
                    (void)FX::readEnd();
                    image += image_adv;
                    FX::seekData(image);
                }
                
                count = rw;
                do
                {
                    image_data = FX::readPendingUInt8();
                    image_data = (uint8_t)image_data * shift_coef;
                    shift_mask = FX::readPendingUInt8();
                    shift_mask = (uint8_t)shift_mask * shift_coef;
                    buf_data = *buf;
                    buf_data &= ~uint8_t(shift_mask >> 0);
                    buf_data |= uint8_t(image_data >> 0);
                    *buf++ = buf_data;
                    buf_data = *bufn;
                    buf_data &= ~uint8_t(shift_mask >> 8);
                    buf_data |= uint8_t(image_data >> 8);
                    *bufn++ = buf_data;
                } while(--count != 0);
                buf += buf_adv;
                bufn += buf_adv;
                if(mode & FLAG_NEED_RESEEK)
                    mode |= FLAG_RESEEK;
            } while(--num_pages != 0);
        }
    }
    
    if(mode & FLAG_BOT)
    {
        if(mode & FLAG_RESEEK)
        {
            (void)FX::readEnd();
            image += image_adv;
            FX::seekData(image);
        }
        
        count = rw;
        if(!(mode & MODE_PLUSMASK))
        {
            do
            {
                image_data = FX::readPendingUInt8();
                image_data = (uint8_t)image_data * shift_coef;
                buf_data = *buf;
                buf_data &= uint8_t(shift_mask >> 0);
                buf_data |= uint8_t(image_data >> 0);
                *buf++ = buf_data;
            } while(--count != 0);
        }
        else
        {
            do
            {
                image_data = FX::readPendingUInt8();
                image_data = (uint8_t)image_data * shift_coef;
                shift_mask = FX::readPendingUInt8();
                shift_mask = (uint8_t)shift_mask * shift_coef;
                buf_data = *buf;
                buf_data &= ~uint8_t(shift_mask >> 0);
                buf_data |= uint8_t(image_data >> 0);
                *buf++ = buf_data;
            } while(--count != 0);
        }
    }
    
    (void)FX::readEnd();
}
#endif

#endif
