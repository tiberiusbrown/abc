#include "ards_shades.hpp"

#include "SpritesABC.hpp"

#if ABC_SHADES != 2

#define ABG_PRECHARGE_CYCLES 1
#define ABG_DISCHARGE_CYCLES 2

static uint8_t* cmd_ptr;
static uint8_t current_plane;

inline uint8_t const* cmd0_end()
{
    return &ards::vm.gs.buf0[sizeof(ards::vm.gs.buf0)];
}

inline uint8_t const* cmd1_end()
{
    return &ards::vm.gs.buf1[sizeof(ards::vm.gs.buf1)];
}

inline bool cmd0_room(uint8_t n)
{
    return cmd_ptr + n < cmd0_end();
}

template<class T> inline uint8_t ld_inc(T const*& p)
{
    uint8_t r;
    asm volatile("ld %[r], %a[p]+\n" : [p] "+&e" (p), [r] "=&r" (r) :: "memory");
    return r;
}

template<class T> inline void st_inc(T*& p, uint8_t x)
{
    asm volatile("st %a[p]+, %[x]\n" : [p] "+&e" (p) : [x] "r" (x) : "memory");
}

static inline uint8_t planeColor(uint8_t c)
{
#if ABC_SHADES == 2
    return c;
#else
    return c > current_plane ? 1 : 0;
#endif
}

void shades_init()
{
    static uint8_t const CMDS[] PROGMEM =
    {
        0xC0, 0xA0, // reset to normal orientation
        0xD9, ((ABG_PRECHARGE_CYCLES) | ((ABG_DISCHARGE_CYCLES) << 4)),
#if defined(OLED_SH1106)
        0xD5, 0xF0, // clock divider (not set in homemade package)
#endif
        0x81, 255,  // max brightness
        0xA8, 0     // park at row 0
    };

    FX::enableOLED();
    Arduboy2Base::LCDCommandMode();
    for (uint8_t i = 0; i < sizeof(CMDS); i++)
        Arduboy2Base::SPItransfer(pgm_read_byte(CMDS + i));
    Arduboy2Base::LCDDataMode();
    FX::disableOLED();

    cmd_ptr = &ards::vm.gs.buf0[0];
}

// clear: low byte is whether to clear, high byte is clear color
// pages: low byte is page count, high byte is starting page (which is unused for SSD1306)
__attribute__((naked, noinline))
static void paint(uint8_t* image, uint16_t clear, uint16_t pages, uint8_t mask)
{
    // image: r24:r25
    // clear: r22:r23
    // pages: r20:r21
    // mask : r18
#if defined(OLED_SH1106) || defined(LCD_ST7565)
    asm volatile(
    
        R"ASM(
                
            ; set buffer pointer to end of buffer pages
            movw r26, r24
            ldi  r19, 128
            mul  r20, r19
            movw r24, r0
            clr  __zero_reg__
            add  r26, r24
            adc  r27, r25
            
            ; add OLED_SET_PAGE_ADDRESS
            subi r21, -(%[page_cmd])
        
            ; outer loop
        1:  ldi  r19, %[DORD2]
            out  %[spcr], r19
            cbi  %[dc_port], %[dc_bit]
            out  %[spdr], r21 ; set page
            rcall 3f
            rcall 3f
            rjmp .+0
            ldi  r24, %[col_cmd]
            out  %[spdr], r24 ; set column hi
            rcall 3f
            rcall 3f
            rcall 3f
            sbi  %[dc_port], %[dc_bit]
            ldi  r19, %[DORD1]
            out  %[spcr], r19
            ldi  r24, 128
        
            ; main loop: send buffer in reverse direction, masking bytes
        2:  ld   __tmp_reg__, -X
            mov  r19, __tmp_reg__
            cpse r22, __zero_reg__
            mov  r19, r23
            st   X, r19
            lpm  r19, Z
            lpm  r19, Z
            and  __tmp_reg__, r18
            out  %[spdr], __tmp_reg__
            dec  r24
            brne 2b
            
            rcall 3f
            rcall 3f
            inc  r21
            dec  r20
            brne 1b
                            
            ; delay for final byte, then reset SPCR DORD and clear SPIF
            rcall 3f
            rcall 3f
            ldi  r19, %[DORD2]
            in   __tmp_reg__, %[spsr]
            out  %[spcr], r19
        3:  ret
        
        )ASM"
        
        :
        : [spdr]     "I"   (_SFR_IO_ADDR(SPDR)),
            [spsr]     "I"   (_SFR_IO_ADDR(SPSR)),
            [spcr]     "I"   (_SFR_IO_ADDR(SPCR)),
            [page_cmd] "M"   (OLED_SET_PAGE_ADDRESS),
            [col_cmd]  "M"   (OLED_SET_COLUMN_ADDRESS_HI),
            [DORD1]    "i"   (_BV(SPE) | _BV(MSTR) | _BV(DORD)),
            [DORD2]    "i"   (_BV(SPE) | _BV(MSTR)),
            [dc_port]  "I"   (_SFR_IO_ADDR(DC_PORT)),
            [dc_bit]   "I"   (DC_BIT)
        );
#else
    asm volatile(
    
        R"ASM(
    
            ; set SPCR DORD to MSB-to-LSB order
            ldi  r19, %[DORD1]
            out  %[spcr], r19
            
            ; init counter and set buffer pointer to end of buffer pages
            movw r26, r24
            ldi  r19, 128
            mul  r20, r19
            movw r24, r0
            clr  __zero_reg__
            add  r26, r24
            adc  r27, r25
    
            ; main loop: send buffer in reverse direction, masking bytes
        1:  ld   r21, -X
            mov  r19, r21
            cpse r22, __zero_reg__
            mov  r19, r23
            st   X, r19
            lpm
            rjmp .+0
            and  r21, r18
            out  %[spdr], r21
            sbiw r24, 1
            brne 1b
            
            ; delay for final byte, then reset SPCR DORD and clear SPIF
            rcall 2f
            rcall 2f
            ldi  r19, %[DORD2]
            in   __tmp_reg__, %[spsr]
            out  %[spcr], r19
        2:  ret
    
        )ASM"
        
        :
        : [spdr]    "I"   (_SFR_IO_ADDR(SPDR))
        , [spsr]    "I"   (_SFR_IO_ADDR(SPSR))
        , [spcr]    "I"   (_SFR_IO_ADDR(SPCR))
        , [DORD1]   "i"   (_BV(SPE) | _BV(MSTR) | _BV(DORD))
        , [DORD2]   "i"   (_BV(SPE) | _BV(MSTR))
        );
#endif
}

void shades_swap()
{
    memcpy(ards::vm.gs.buf1, ards::vm.gs.buf0, sizeof(ards::vm.gs.buf1));
    ards::vm.gs.buf0[0] = SHADES_CMD_END;
    cmd_ptr = &ards::vm.gs.buf0[0];
}

void shades_display()
{
    // Push display buffer to display

    while(*(uint8_t volatile*)&ards::vm.needs_render != 2)
        ;
    ards::vm.needs_render = 0;
#if ABC_SHADES == 3
    current_plane = !current_plane;
#elif ABC_SHADES == 4
    {
        uint8_t t = current_plane;
        if(++t > 2) t = 0;
        current_plane = t;
    }
#endif
    auto* b = Arduboy2Base::sBuffer;

    FX::enableOLED();

    paint(&b[128 * 7], 0x0001, 0x0001, 0x7f);

    Arduboy2Base::LCDCommandMode();
    Arduboy2Base::SPItransfer(0xa8);
    Arduboy2Base::SPItransfer(63);
    Arduboy2Base::LCDDataMode();

    paint(&b[128 * 0], 0x0001, 0x0107, 0xff);

    Arduboy2Base::LCDCommandMode();
    Arduboy2Base::SPItransfer(0xa8);
    Arduboy2Base::SPItransfer(0);
    Arduboy2Base::LCDDataMode();

    FX::disableOLED();

    // Render command buffer 1 to display buffer
    uint8_t const* p = ards::vm.gs.buf1;
    while(p < cmd1_end())
    {
        uint8_t cmd = ld_inc(p);
        if(cmd == SHADES_CMD_END) break;
        switch(cmd)
        {
        case SHADES_CMD_RECT:
        case SHADES_CMD_FILLED_RECT:
        {
            int8_t x = (int8_t)ld_inc(p);
            int8_t y = (int8_t)ld_inc(p);
            uint8_t w = ld_inc(p);
            uint8_t h = ld_inc(p);
            uint8_t c = planeColor(ld_inc(p));
            if(cmd == SHADES_CMD_FILLED_RECT)
            {
                SpritesABC::fillRect(x, y, w, h, c);
            }
            else
            {
                SpritesABC::fillRect(x, y, w, 1, c);
                SpritesABC::fillRect(x, y, 1, h, c);
                SpritesABC::fillRect(x, y + h - 1, w, 1, c);
                SpritesABC::fillRect(x + w - 1, y, 1, h, c);
            }
        }
        default:
            break;
        }
    }
}

void shades_draw_rect(
    int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c, bool filled)
{
    if(!cmd0_room(6))
        return;

    if(x >= WIDTH) return;
    if(y >= HEIGHT) return;
    if(x + w <= 0) return;
    if(y + h <= 0) return;
    
    if(x < 0)
    {
        w += x;
        x = 0;
    }

    if(y < 0)
    {
        h += y;
        y = 0;
    }

    uint8_t* p = cmd_ptr;
    st_inc(p, filled ? SHADES_CMD_FILLED_RECT : SHADES_CMD_RECT);
    st_inc(p, (uint8_t)x);
    st_inc(p, (uint8_t)y);
    st_inc(p, w);
    st_inc(p, h);
    st_inc(p, c);
    cmd_ptr = p;
}

#endif
