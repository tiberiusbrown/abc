#include "ards_shades.hpp"

#if ABC_SHADES != 2

#include "SpritesABC.hpp"

#define ABG_PRECHARGE_CYCLES 1
#define ABG_DISCHARGE_CYCLES 2

void draw_char(int16_t& x, int16_t y, char c);
extern "C" uint8_t font_get_x_advance(char c);
extern "C" uint8_t font_get_line_height();
extern "C" void fx_seek_data(uint24_t addr);

void wait_for_frame_timing();

static uint8_t* cmd_ptr;
static uint8_t* batch_ptr; // for sprite/char batching (nullptr if no active batch)
static uint8_t current_plane;

// for sprite batching
constexpr int8_t SPRITE_BATCH_ADV = 127;
static int8_t batch_px; // previous coords
static int8_t batch_py;
static int8_t batch_dx; // difference in coords
static int8_t batch_dy;

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

template<class T> inline uint8_t ld_inc(T*& p)
{
    uint8_t r;
    asm volatile("ld %[r], %a[p]+\n" : [p] "+&e" (p), [r] "=&r" (r) :: "memory");
    return r;
}

template<class T> inline uint16_t ld_inc2(T*& p)
{
    uint16_t r;
    asm volatile(
        "ld %A[r], %a[p]+\n"
        "ld %B[r], %a[p]+\n"
        : [p] "+&e" (p), [r] "=&r" (r) :: "memory");
    return r;
}

template<class T> inline uint24_t ld_inc3(T*& p)
{
    uint24_t r;
    asm volatile(
        "ld %A[r], %a[p]+\n"
        "ld %B[r], %a[p]+\n"
        "ld %C[r], %a[p]+\n"
        : [p] "+&e" (p), [r] "=&r" (r) :: "memory");
    return r;
}

template<class T> inline void st_inc(T*& p, uint8_t x)
{
    asm volatile("st %a[p]+, %[x]\n" : [p] "+&e" (p) : [x] "r" (x) : "memory");
}

template<class T> inline void st_inc2(T*& p, uint16_t x)
{
    asm volatile(
        "st %a[p]+, %A[x]\n"
        "st %a[p]+, %B[x]\n"
        : [p] "+&e" (p) : [x] "r" (x) : "memory");
}

template<class T> inline void st_inc3(T*& p, uint24_t x)
{
    asm volatile(
        "st %a[p]+, %A[x]\n"
        "st %a[p]+, %B[x]\n"
        "st %a[p]+, %C[x]\n"
        : [p] "+&e" (p) : [x] "r" (x) : "memory");
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
        0xDB, 0x70,
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
    memset(ards::vm.gs.buf0, SHADES_CMD_END, sizeof(ards::vm.gs.buf0));
    cmd_ptr = &ards::vm.gs.buf0[0];
    batch_ptr = nullptr;
    wait_for_frame_timing();
}

[[gnu::noinline]]
void shades_display()
{
    // Push display buffer to display

    while(*(uint8_t volatile*)&ards::vm.needs_render != 2)
        Arduboy2Base::idle();
    auto* b = Arduboy2Base::sBuffer;

    FX::enableOLED();

    paint(&b[128 * 7], 0x0001, 0x0001, 0x7f);

    Arduboy2Base::LCDCommandMode();

    {
        uint8_t t = current_plane;
        uint8_t contrast = 255;
#if ABC_SHADES == 3
        t = !t;
        if(t & 1) contrast = 64;
#elif ABC_SHADES == 4
        if(++t > 2) t = 0;
        if(t & 1) contrast = 25;
        if(t & 2) contrast = 85;
#endif
        current_plane = t;
#if ABC_SHADES_ADJUST_CONTRAST
        Arduboy2Base::SPItransfer(0x81);
        Arduboy2Base::SPItransfer(contrast);
#endif
    }

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
            uint8_t x = ld_inc(p);
            uint8_t y = ld_inc(p);
            uint8_t w = ld_inc(p);
            uint8_t h = ld_inc(p);
            uint8_t c = planeColor(ld_inc(p));
            if(cmd == SHADES_CMD_FILLED_RECT)
            {
                SpritesABC::fillRect_clipped(x, y, w, h, c);
            }
            else
            {
                SpritesABC::fillRect_clipped(x, y, w, 1, c);
                SpritesABC::fillRect_clipped(x, y, 1, h, c);
                SpritesABC::fillRect_clipped(x, y + h - 1, w, 1, c);
                SpritesABC::fillRect_clipped(x + w - 1, y, 1, h, c);
            }
            break;
        }
        case SHADES_CMD_SPRITE:
        {
            int16_t x = (int16_t)ld_inc2(p);
            int16_t y = (int16_t)ld_inc2(p);
            uint24_t img = ld_inc3(p);
            uint16_t frame = ld_inc2(p);
            FX::seekData(img);
            uint8_t w = FX::readPendingUInt8();
            uint8_t h = FX::readPendingUInt8();
            uint8_t mode = FX::readPendingLastUInt8();

            h = (h + 7) & 0xf8;
#if 0
            uint8_t p = h >> 3;
#else
            uint8_t p = h;
            asm volatile(R"(
                lsr %[p]
                lsr %[p]
                lsr %[p]
                )"
                : [p] "+&r" (p)
            );
#endif
            if(mode) p <<= 1;
#if 0
            uint24_t t = w * p;
            frame *= (ABC_SHADES - 1);
            frame += current_plane;
            img += t * frame;
            img += 5;
#else
            uint16_t t;
            asm volatile(
#if ABC_SHADES == 4
                R"(
                    movw %A[t], %A[frame]
                    add  %A[frame], %A[frame]
                    adc  %B[frame], %B[frame]
                    add  %A[frame], %A[t]
                    adc  %B[frame], %B[t]
                )"
#elif ABC_SHADES == 3
                R"(
                    add  %A[frame], %A[frame]
                    adc  %B[frame], %B[frame]
                )"
#else
#error "shades"
#endif
                R"(
                    ldi  %A[t], 5
                    add  %A[img], %A[t]
                    adc  %B[img], __zero_reg__
                    adc  %C[img], __zero_reg__
                    
                    lds  %A[t], %[plane]
                    add  %A[frame], %A[t]
                    adc  %B[frame], __zero_reg__
                    
                    mul  %[w], %[p]
                    movw %A[t], r0
                    
                    mul  %B[t], %A[frame]
                    add  %B[img], r0
                    adc  %C[img], r1
                    mul  %A[t], %B[frame]
                    add  %B[img], r0
                    adc  %C[img], r1
                    mul  %B[t], %B[frame]
                    add  %C[img], r0
                    mul  %A[t], %A[frame]
                    add  %A[img], r0
                    adc  %B[img], r1
                    clr  __zero_reg__
                    adc  %C[img], __zero_reg__
                )"
                : [img]    "+&r" (img)
                , [frame]  "+&r" (frame)
                , [t]      "=&d" (t)
                : [plane]  ""    (&current_plane)
                , [w]      "r"   (w)
                , [p]      "r"   (p)
            );
#endif

            SpritesABC::drawBasicFX(x, y, w, h, img, mode);
            break;
        }
        case SHADES_CMD_SPRITE_BATCH:
        {
            uint24_t img = ld_inc3(p);
            uint8_t n = ld_inc(p);
            FX::seekData(img);
            uint8_t w = FX::readPendingUInt8();
            uint8_t h = FX::readPendingUInt8();
            uint8_t mode = FX::readPendingLastUInt8();
            uint16_t t;
            {
                h = (h + 7) & 0xf8;
#if 0
                uint8_t p = h >> 3;
#else
                uint8_t p = h;
                asm volatile(R"(
                    lsr %[p]
                    lsr %[p]
                    lsr %[p]
                    )"
                    : [p] "+&r" (p)
                );
#endif
                if(mode) p <<= 1;
                {
#if 0
                    uint8_t tt;
                    tt = p * current_plane;
                    img += w * tt + 5u;
                    t = w * p;
#else
                    uint16_t tt;
                    asm volatile(R"(
                            mul  %[p], %[plane]
                            mul  %[w], r0
                            movw %A[tt], r0
                            adiw %A[tt], 5
                            mul  %[w], %[p]
                            movw %A[t], r0
                            clr  __zero_reg__
                            add  %A[img], %A[tt]
                            adc  %B[img], %B[tt]
                            adc  %C[img], __zero_reg__
                        )"
                        : [tt]    "=&w" (tt)
                        , [img]   "+&r" (img)
                        , [t]     "=&r" (t)
                        : [w]     "r"   (w)
                        , [p]     "r"   (p)
                        , [plane] "r"   (current_plane)
                    );
#endif
                }
                t *= (ABC_SHADES - 1);
            }
            int8_t x, y, dx = 0, dy = 0;
            do
            {
                uint8_t f = ld_inc(p);
                int8_t ty = (int8_t)ld_inc(p);
                if(ty == SPRITE_BATCH_ADV)
                {
                    x += dx;
                    y += dy;
                }
                else
                {
                    dy = ty - y;
                    y = ty;
                    ty = (int8_t)ld_inc(p);
                    dx = ty - x;
                    x = ty;
                }
                uint24_t timg = img;

                //      A1 A0
                //         B0
                //   ========
                //      A0*B0
                //   A1*B0
                //   ========
                //   C2 C1 C0
                asm volatile(R"(
                        mul  %B[t], %[f]
                        add  %B[timg], r0
                        adc  %C[timg], r1
                        mul  %A[t], %[f]
                        add  %A[timg], r0
                        adc  %B[timg], r1
                        clr  __zero_reg__
                        adc  %C[timg], __zero_reg__
                    )"
                    : [timg] "+&r" (timg)
                    : [t]    "r"   (t)
                    , [f]    "r"   (f)
                    );
                
                SpritesABC::drawBasicFX(x, y, w, h, timg, mode);

            } while(--n != 0);
            break;
        }
        case SHADES_CMD_CHARS:
        {
            int16_t x = (int16_t)ld_inc2(p);
            int16_t y = (int16_t)ld_inc2(p);
            uint24_t font = ld_inc3(p);
            uint8_t mode = ld_inc(p);
            uint16_t n = ld_inc2(p);
            int16_t bx = x;
            {
                uint24_t t = ards::vm.text_font;
                ards::vm.text_font = font;
                font = t;
            }
#if 0
            {
                uint8_t t = SpritesABC::MODE_SELFMASK;
                if(planeColor(mode))
                    t |= (SpritesABC::MODE_SELFMASK ^ SpritesABC::MODE_SELFMASK_ERASE);
                mode = t;
            }
#else
            mode = planeColor(mode) ? SpritesABC::MODE_SELFMASK : SpritesABC::MODE_SELFMASK_ERASE;
#endif
            {
                uint8_t t = ards::vm.text_mode;
                ards::vm.text_mode = mode;
                mode = t;
            }
            while(n-- != 0)
            {
                uint8_t c = ld_inc(p);
                if(c == '\n')
                {
                    x = bx;
                    y += font_get_line_height();
                    continue;
                }
                draw_char(x, y, c);
            }
            ards::vm.text_font = font;
            ards::vm.text_mode = mode;
            break;
        }
        default:
            break;
        }
    }
    
    // set this here to allow frame skipping
    ards::vm.needs_render = 0;
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

    uint8_t xc = x;
    uint8_t yc = y;
    
    if(x < 0)
    {
        w += xc;
        xc = 0;
    }

    if(y < 0)
    {
        h += yc;
        yc = 0;
    }

    if(h >= uint8_t(64 - yc))
        h = 64 - yc;
    if(w >= uint8_t(128 - xc))
        w = 128 - xc;

    uint8_t* p = cmd_ptr;
    static_assert(SHADES_CMD_FILLED_RECT == SHADES_CMD_RECT + 1, "");
    st_inc(p, SHADES_CMD_RECT + filled);
    st_inc(p, (uint8_t)xc);
    st_inc(p, (uint8_t)yc);
    st_inc(p, w);
    st_inc(p, h);
    st_inc(p, c);
    cmd_ptr = p;
}

void shades_draw_sprite(
    int16_t x, int16_t y, uint24_t img, uint16_t frame)
{
    if(!cmd0_room(3))
        return;

    if(x >= WIDTH) return;
    if(y >= HEIGHT) return;

    FX::seekData(img);
    uint8_t w = FX::readPendingUInt8();
    uint8_t h = FX::readPendingLastUInt8();

    if(x + w <= 0) return;
    if(y + h <= 0) return;

    uint8_t* p = cmd_ptr;
    {
        uint8_t* b = batch_ptr;
        if(x < -128 || y < -128) goto unbatchable;
        if(frame >= 256) goto unbatchable;
        if(b == nullptr) goto start_new_batch;
        if(ld_inc(b) != SHADES_CMD_SPRITE_BATCH) goto start_new_batch;
        if(ld_inc3(b) != img) goto start_new_batch;
        uint8_t n = *b;
        n += 1;
        if(n == 0) goto start_new_batch;
        *b = n;
        goto batch_add;
    }

start_new_batch:
    if(!cmd0_room(8))
        return;

    batch_ptr = p;
    batch_px = x;
    batch_py = y;
    st_inc(p, SHADES_CMD_SPRITE_BATCH);
    st_inc3(p, img);
    st_inc(p, 1);
    goto batch_add_first;

batch_add:
    if( (int8_t)x == int8_t(batch_px + batch_dx) &&
        (int8_t)y == int8_t(batch_py + batch_dy))
    {
        st_inc(p, (uint8_t)frame);
        st_inc(p, SPRITE_BATCH_ADV);
    }
    else
    {
batch_add_first:
        st_inc(p, (uint8_t)frame);
        st_inc(p, (uint8_t)y);
        st_inc(p, (uint8_t)x);
        batch_dx = int8_t(x - batch_px);
        batch_dy = int8_t(y - batch_py);
    }
    batch_px = (int8_t)x;
    batch_py = (int8_t)y;
    cmd_ptr = p;
    return;

unbatchable:
    if(!cmd0_room(10))
        return;
    
    st_inc(p, SHADES_CMD_SPRITE);
    st_inc2(p, (uint16_t)x);
    st_inc2(p, (uint16_t)y);
    st_inc3(p, img);
    st_inc2(p, frame);
    cmd_ptr = p;
}

void shades_draw_chars_begin(int16_t x, int16_t y)
{
    if(!cmd0_room(12)) // at least one char
        return;
    
    uint8_t* p = cmd_ptr;
    st_inc(p, SHADES_CMD_CHARS);
    st_inc2(p, (uint16_t)x);
    st_inc2(p, (uint16_t)y);
    st_inc3(p, ards::vm.text_font);
    st_inc(p, ards::vm.text_mode);
    batch_ptr = p;
    st_inc2(p, 0);
    cmd_ptr = p;
}

void shades_draw_char(char c)
{
    uint8_t* b = batch_ptr;
    if(b == nullptr) return;
    if(!cmd0_room(1))
    {
        batch_ptr = nullptr;
        return;
    }
    uint16_t n = *(uint16_t*)b;
    n += 1;
    *(uint16_t*)b = n;
    st_inc(cmd_ptr, (uint8_t)c);
}

void shades_draw_chars_end()
{
    batch_ptr = nullptr;
}

#endif
