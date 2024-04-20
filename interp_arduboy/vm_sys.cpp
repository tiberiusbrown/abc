#ifndef EEPROM_h
#define EEPROM_h
#endif

#include <Arduboy2.h>
#include <Arduboy2Audio.h>
#include <ArduboyFX.h>

#define SPRITESABC_IMPLEMENTATION
#include "SpritesABC.hpp"

#include "ards_tone.hpp"

#include "ards_vm.hpp"

#include <math.h>

constexpr uint8_t FONT_HEADER_PER_CHAR = 7;
constexpr uint8_t FONT_HEADER_OFFSET = 1;
constexpr uint16_t FONT_HEADER_BYTES =
    FONT_HEADER_PER_CHAR * 256 + FONT_HEADER_OFFSET;

using sys_func_t = void(*)();
extern sys_func_t const SYS_FUNCS[] PROGMEM;
extern "C" void vm_error(ards::error_t e);

template<class T>
__attribute__((always_inline)) inline uint8_t ld_inc(T*& p)
{
    uint8_t t;
    asm volatile("ld %[t], %a[p]+\n" : [t]"=&r"(t), [p]"+&e"(p));
    return t;
}

template<class T>
__attribute__((always_inline)) inline uint8_t ld_predec(T*& p)
{
    uint8_t t;
    asm volatile("ld %[t], -%a[p]\n" : [t]"=&r"(t), [p]"+&e"(p));
    return t;
}

template<class T>
__attribute__((always_inline)) inline void st_inc(T*& p, uint8_t t)
{
    asm volatile("st %a[p]+, %[t]\n" : [p]"+&e"(p) : [t]"r"(t));
}

__attribute__((always_inline)) inline uint8_t* vm_pop_begin()
{
    uint8_t* r;
    asm volatile(
        "lds  %A[r], %[vm_sp]\n"
        "ldi  %B[r], 1\n"
        : [r]     "=&d" (r)
        : [vm_sp] ""    (&ards::vm.sp));
    return r;
}

__attribute__((always_inline)) inline void vm_pop_end(uint8_t* ptr)
{
    ards::vm.sp = (uint8_t)(uintptr_t)ptr;
}

template<class T>
__attribute__((always_inline)) inline T vm_pop(uint8_t*& ptr)
{
    union
    {
        T r;
        uint8_t b[sizeof(T)];
    } u;
    for(size_t i = 0; i < sizeof(T); ++i)
    {
        asm volatile(
            "ld %[t], -%a[ptr]\n"
            : [t] "=&r" (u.b[sizeof(T) - i - 1])
            , [ptr] "+&e" (ptr)
            );
    }
    return u.r;
}

template<size_t N> struct byte_storage { uint8_t d[N]; };

template<size_t N>
__attribute__((always_inline))
inline void vm_push_n_unsafe(uint8_t*& ptr, byte_storage<N> x)
{
    static_assert(N <= 4, "");
    if(N >= 1) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[0]));
    if(N >= 2) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[1]));
    if(N >= 3) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[2]));
    if(N >= 4) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[3]));
}

template<size_t N>
__attribute__((noinline))
inline void vm_push_n(uint8_t* ptr, byte_storage<N> x)
{
    if((uint8_t)(uintptr_t)ptr + N >= 256)
        vm_error(ards::ERR_DST);
    vm_push_n_unsafe<N>(ptr, x);
    ards::vm.sp = (uint8_t)(uintptr_t)ptr;
}

template<class T>
__attribute__((always_inline))
void vm_push_unsafe(uint8_t*& ptr, T x)
{
    union
    {
        T x;
        byte_storage<sizeof(T)> b;
    } u = { x };
    vm_push_n_unsafe<sizeof(T)>(ptr, u.b);
}

template<class T>
//__attribute__((always_inline))
void vm_push(T x)
{
    uint8_t* ptr;
    asm volatile(
        "lds  %A[ptr], %[vm_sp]\n"
        "ldi  %B[ptr], 1\n"
        : [ptr]   "=&e" (ptr)
        : [vm_sp] ""    (&ards::vm.sp));
    union
    {
        T x;
        byte_storage<sizeof(T)> b;
    } u = { x };
    vm_push_n<sizeof(T)>(ptr, u.b);
}

extern "C" uint8_t font_get_x_advance(char c);
extern "C" uint8_t font_get_line_height();
extern "C" void fx_seek_data(uint24_t addr);

__attribute__((naked, noinline))
static void seek_to_pc()
{
    asm volatile(R"(
        
        1:  ldi  r24, 3
            cbi  %[fxport], %[fxbit]
            out  %[spdr], r24

            ; see if we need to call ards::Tones::update()
            lds  r16, %[tones_size]
            cpi  r16, %[tones_maxsize]
            brsh 2f
            clr  r1
            sbi  %[fxport], %[fxbit]
            call %x[tones_update]
            rjmp 1b

        2:  lds  r20, %[pc]+0
            lds  r21, %[pc]+1
            lds  r22, %[pc]+2
        L%=_seek:
            lds  r24, %[datapage]+0
            lds  r25, %[datapage]+1
            add  r21, r24
            adc  r22, r25
            out  %[spdr], r22
            rjmp 3f
        L%=_delay_17:
            nop
        L%=_delay_16:
            nop
        L%=_delay_15:
            rjmp .+0
        L%=_delay_13:
            nop
        L%=_delay_12:
            rjmp .+0
        L%=_delay_10:
            lpm
        L%=_delay_7:
            ret
        3:  rcall L%=_delay_15
            out  %[spdr], r21
            rcall L%=_delay_17
            out  %[spdr], r20
            rcall L%=_delay_16
            out  %[spdr], __zero_reg__
            rjmp  L%=_delay_13

        font_get_x_advance:
            ldi  r25, 3
            cbi  %[fxport], %[fxbit]
            out  %[spdr], r25
            lds  r20, %[font]+0
            lds  r21, %[font]+1
            lds  r22, %[font]+2
            ldi  r25, %[header_per_char]
            mul  r24, r25
            add  r20, r0
            adc  r21, r1
            clr  r1
            adc  r22, r1
            rcall L%=_seek
            rjmp .+0
            rjmp .+0
            in   r24, %[spdr]
            sbi  %[fxport], %[fxbit]
            ret

        font_get_line_height:
            ldi  r25, 3
            cbi  %[fxport], %[fxbit]
            out  %[spdr], r25
            lds  r20, %[font]+0
            lds  r21, %[font]+1
            lds  r22, %[font]+2
            ldi  r25, %[header_per_char]
            add  r21, r25
            adc  r22, r1
            rcall L%=_seek
            rjmp .+0
            rjmp .+0
            in   r24, %[spdr]
            sbi  %[fxport], %[fxbit]
            ret
        
        fx_seek_data:
            ldi  r25, 3
            cbi  %[fxport], %[fxbit]
            out  %[spdr], r25
            movw  r20, r22
            mov   r22, r24
            lpm
            rjmp .+0
            rcall L%=_seek
            nop
            ret
        )"
        :
        : [pc]              "i" (&ards::vm.pc)
        , [font]            "i" (&ards::vm.text_font)
        , [header_per_char] "i" (FONT_HEADER_PER_CHAR)
        , [fxport]          "i" (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]           "i" (FX_BIT)
        , [spdr]            "i" (_SFR_IO_ADDR(SPDR))
        , [datapage]        ""  (&FX::programDataPage)
        , [tones_update]    ""  (ards::Tones::update)
        , [tones_size]      ""  (&ards::detail::buffer_size)
        , [tones_maxsize]   ""  (sizeof(ards::detail::buffer))
        );
}

__attribute__((noinline, naked))
static uint8_t fx_read_byte_inc(uint24_t& fb)
{
    asm volatile(R"(
        cbi  %[fxport], %[fxbit]
        ldi  r26, 3
        out  %[spdr], r26
        movw r30, r24
        ld   r24, Z+
        ld   r25, Z+
        ld   r26, Z+
        lds  r20, %[datapage]+0
        lds  r21, %[datapage]+1
        add  r20, r25
        adc  r21, r26
        rjmp .+0
        rjmp .+0
        out  %[spdr], r21
        rcall L%=_delay_17
        out  %[spdr], r20
        rcall L%=_delay_17
        out  %[spdr], r24
        adiw r24, 1
        adc  r26, __zero_reg__
        st   -Z, r26
        st   -Z, r25
        st   -Z, r24
        rcall L%=_delay_7
        out  %[spdr], __zero_reg__
        rcall L%=_delay_16
        in   r24, %[spdr]
        sbi  %[fxport], %[fxbit]
        ret

    L%=_delay_17:
        nop
    L%=_delay_16:
        lpm
        lpm
        lpm
    L%=_delay_7:
        ret
        )"
        :
        : [fxport]   "i" (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]    "i" (FX_BIT)
        , [spdr]     "i" (_SFR_IO_ADDR(SPDR))
        , [spsr]     "i" (_SFR_IO_ADDR(SPSR))
        , [datapage] ""  (&FX::programDataPage)
    );
}

static void sys_display()
{
    (void)FX::readEnd();
    FX::display(true);
    seek_to_pc();
}

static void sys_display_noclear()
{
    (void)FX::readEnd();
    FX::display(false);
    seek_to_pc();
}

static void sys_draw_pixel()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::drawPixel(x, y, color);
}

static void sys_get_pixel()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t c = Arduboy2Base::getPixel(x, y);
    vm_push_unsafe<uint8_t>(ptr, c);
    vm_pop_end(ptr);
}

__attribute__((naked))
static void sys_draw_hline()
{
#if 1
    // no need to push r16
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            ld   r20, -Y
            ldi  r18, 1
            ld   r16, -Y
            sts  %[vmsp], r28
            jmp  %x[fillrect]
        )"
        :
        : [vmsp]     "i" (&ards::vm.sp)
        , [fillrect] ""  (SpritesABC::fillRect)
        );
#else
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, w, 1, color);
#endif
}

__attribute__((naked))
static void sys_draw_vline()
{
#if 1
    // no need to push r16
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            ldi  r20, 1
            ld   r18, -Y
            ld   r16, -Y
            sts  %[vmsp], r28
            jmp  %x[fillrect]
        )"
        :
        : [vmsp]     "i" (&ards::vm.sp)
        , [fillrect] ""  (SpritesABC::fillRect)
        );
#else
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, 1, h, color);
#endif
}

static void sys_draw_line()
{
    auto ptr = vm_pop_begin();
    int16_t x0 = vm_pop<int16_t>(ptr);
    int16_t y0 = vm_pop<int16_t>(ptr);
    int16_t x1 = vm_pop<int16_t>(ptr);
    int16_t y1 = vm_pop<int16_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::drawLine(x0, y0, x1, y1, color);
}

static void sys_draw_rect()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, w, 1, color);
    SpritesABC::fillRect(x, y, 1, h, color);
    SpritesABC::fillRect(x, y + h - 1, w, 1, color);
    SpritesABC::fillRect(x + w - 1, y, 1, h, color);
}

__attribute__((naked))
static void sys_draw_filled_rect()
{
#if 1
    // no need to push r16
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            ld   r20, -Y
            ld   r18, -Y
            ld   r16, -Y
            sts  %[vmsp], r28
            jmp  %x[fillrect]
        )"
        :
        : [vmsp]     "i" (&ards::vm.sp)
        , [fillrect] ""  (SpritesABC::fillRect)
        );
#else
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, w, h, color);
#endif
}

static void sys_draw_circle()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t r = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::drawCircle(x, y, r, color);
}

__attribute__((always_inline))
static void draw_fast_vline(int16_t x, int16_t y, uint16_t h, uint8_t color)
{
    SpritesABC::fillRect(x, y, 1, h, color);
}

// adapted from Arduboy2 library (BSD 3-clause)
static void fill_circle_helper(
    int16_t x0, int16_t y0, uint8_t r,
    uint8_t sides, int16_t delta, uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while(x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        if(sides & 0x1) // right side
        {
            draw_fast_vline(x0+x, y0-y, 2*y+1+delta, color);
            draw_fast_vline(x0+y, y0-x, 2*x+1+delta, color);
        }

        if(sides & 0x2) // left side
        {
            draw_fast_vline(x0-x, y0-y, 2*y+1+delta, color);
            draw_fast_vline(x0-y, y0-x, 2*x+1+delta, color);
        }
    }
}

static void sys_draw_filled_circle()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t r = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);

    draw_fast_vline(x, y-r, 2 * r + 1, color);
    fill_circle_helper(x, y, r, 3, 0, color);
    //Arduboy2Base::fillCircle(x, y, r, color);
}

static void sys_set_frame_rate()
{
    auto ptr = vm_pop_begin();
    uint8_t fr = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    ards::vm.frame_dur = 1000u / fr;
}

extern unsigned long volatile timer0_millis;
static void sys_next_frame()
{
    uint8_t now = (uint8_t)timer0_millis;
    uint8_t frame_duration = now - ards::vm.frame_start;
    
    if(ards::vm.just_rendered)
    {
        ards::vm.just_rendered = false;
        vm_push<uint8_t>(0);
        return;
    }
    else if(frame_duration < ards::vm.frame_dur)
    {
        if(++frame_duration < ards::vm.frame_dur)
            Arduboy2Base::idle();
        vm_push<uint8_t>(0);
        return;
    }
    
    ards::vm.just_rendered = true;
    ards::vm.frame_start = now;
    
    vm_push<uint8_t>(1);
}

static void sys_idle()
{
    Arduboy2Base::idle();
}

static void sys_debug_break()
{
    asm volatile("break\n");
}

static void sys_assert()
{
    auto ptr = vm_pop_begin();
    uint8_t b = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    if(!b) vm_error(ards::ERR_ASS);
}

static void sys_poll_buttons()
{
    Arduboy2Base::pollButtons();
}

static void sys_just_pressed()
{
    auto ptr = vm_pop_begin();
    uint8_t btn = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    bool b = Arduboy2Base::justPressed(btn);
    vm_push((uint8_t)b);
}

static void sys_just_released()
{
    auto ptr = vm_pop_begin();
    uint8_t btn = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    bool b = Arduboy2Base::justReleased(btn);
    vm_push((uint8_t)b);
}

static void sys_pressed()
{
    auto ptr = vm_pop_begin();
    uint8_t btns = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    bool b = Arduboy2Base::pressed(btns);
    vm_push((uint8_t)b);
}

static void sys_any_pressed()
{
    auto ptr = vm_pop_begin();
    uint8_t btns = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    bool b = Arduboy2Base::anyPressed(btns);
    vm_push((uint8_t)b);
}

static void sys_not_pressed()
{
    auto ptr = vm_pop_begin();
    uint8_t btns = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    bool b = Arduboy2Base::notPressed(btns);
    vm_push((uint8_t)b);
}

static void sys_millis()
{
    vm_push((uint32_t)millis());
}

static void reverse_str(char* b, char* p)
{
    while(b < p)
    {
        char bc = *b;
        char pc = (char)ld_predec(p);
        st_inc(b, pc);
        *p = bc;
    }
}

static void draw_sprite_helper(uint8_t selfmask_bit)
{
#define DRAW_SPRITE_HELPER_OPT 1
#if !DRAW_SPRITE_HELPER_OPT
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint24_t image = vm_pop<uint24_t>(ptr);
    (void)FX::readEnd();
#endif

#if DRAW_SPRITE_HELPER_OPT
    void* ptr;
    int16_t x;
    int16_t y;
    uint24_t image;
    uint8_t w;
    uint8_t h;
    uint16_t num;
    uint8_t mode;
    uint16_t frame;
    asm volatile(R"(
            sbi  %[fxport], %[fxbit]
            cbi  %[fxport], %[fxbit]
            ldi  %[w], 3
            out  %[spdr], %[w]
            lds  %A[ptr], %[vmsp]
            ldi  %B[ptr], 1
            sbiw %A[ptr], 4
            ld   %C[image], -%a[ptr]
            ld   %B[image], -%a[ptr]
            ld   %A[image], -%a[ptr]
            lds  %A[num], %[datapage]+0
            lds  %B[num], %[datapage]+1
            add  %A[num], %B[image]
            adc  %B[num], %C[image]
            out  %[spdr], %B[num]
            ldd  %A[x], %a[ptr]+5
            ldd  %B[x], %a[ptr]+6
            ldd  %A[y], %a[ptr]+3
            ldd  %B[y], %a[ptr]+4
            ld   %B[frame], -%a[ptr]
            ld   %A[frame], -%a[ptr]
            sts  %[vmsp], %A[ptr]

            rjmp 1f
        draw_sprite_helper_delay_17:
            nop
        draw_sprite_helper_delay_16:
            nop
        draw_sprite_helper_delay_15:
            nop
        draw_sprite_helper_delay_14:
            nop
        draw_sprite_helper_delay_13:
            rjmp .+0
        draw_sprite_helper_delay_11:
            rjmp .+0
            rjmp .+0
            ret
        1:  ldi  %[w], 5

            out  %[spdr], %A[num]
            rcall draw_sprite_helper_delay_16
            in   r0, %[sreg]
            out  %[spdr], %A[image]
            add  %A[image], %[w]
            adc  %B[image], __zero_reg__
            adc  %C[image], __zero_reg__
            rcall draw_sprite_helper_delay_13
            out  %[spdr], __zero_reg__
            rcall draw_sprite_helper_delay_15

            cli
            out  %[spdr], __zero_reg__
            in   %[w], %[spdr]
            out  %[sreg], r0
            rcall draw_sprite_helper_delay_13

            cli
            out  %[spdr], __zero_reg__
            in   %[h], %[spdr]
            out  %[sreg], r0
            rcall draw_sprite_helper_delay_13

            cli
            out  %[spdr], __zero_reg__
            in   %A[num], %[spdr]
            out  %[sreg], r0
            rcall draw_sprite_helper_delay_13

            cli
            out  %[spdr], __zero_reg__
            in   %B[num], %[spdr]
            out  %[sreg], r0
            rcall draw_sprite_helper_delay_14

            in   %[mode], %[spdr]
            in   r0, %[spsr]
            sbi  %[fxport], %[fxbit]

            or   %[mode], %[mask]
        )"
        : [w]        "=&d" (w)
        , [h]        "=&r" (h)
        , [num]      "=&r" (num)
        , [mode]     "=&r" (mode)
        , [ptr]      "=&z" (ptr)
        , [frame]    "=&r" (frame)
        , [image]    "=&r" (image)
        , [x]        "=&r" (x)
        , [y]        "=&r" (y)
        : [mask]     "r"   (selfmask_bit)
        , [vmsp]     ""    (&ards::vm.sp)
        , [fxport]   "i"   (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]    "i"   (FX_BIT)
        , [spdr]     "i"   (_SFR_IO_ADDR(SPDR))
        , [spsr]     "i"   (_SFR_IO_ADDR(SPSR))
        , [sreg]     "i"   (_SFR_IO_ADDR(SREG))
        , [datapage] ""    (&FX::programDataPage)
    );
    if(frame >= num)
        vm_error(ards::ERR_FRM);
    SpritesABC::drawBasic(x, y, w, h, image, frame, mode);
#endif

#if !DRAW_SPRITE_HELPER_OPT
    struct
    {
        uint8_t w;
        uint8_t h;
        uint16_t num;
        uint8_t mode;
    } sprite_data;
    ards::detail::fx_read_data_bytes(image, (uint8_t*)&sprite_data, sizeof(sprite_data));
    auto sd = sprite_data;
    sd.mode |= selfmask_bit;
    uint16_t frame = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);

    if(frame >= sd.num)
        vm_error(ards::ERR_FRM);
    SpritesABC::drawBasic(x, y, sd.w, sd.h, image + 5, frame, sd.mode);
#endif

    seek_to_pc();
}

static void sys_draw_sprite()
{
    draw_sprite_helper(0);
}

static void sys_draw_sprite_selfmask()
{
    draw_sprite_helper(4);
}

static void draw_char(
    int16_t& x, int16_t y, char c)
{
#if 0
    FX::seekData(font + (uint8_t)c * (uint8_t)FONT_HEADER_PER_CHAR);
    uint8_t xadv = (uint8_t)FX::readPendingUInt8();
    int8_t xoff = (int8_t)FX::readPendingUInt8();
    int8_t yoff = (int8_t)FX::readPendingUInt8();
    uint8_t offset_lo = FX::readPendingUInt8();
    uint8_t offset_hi = FX::readPendingUInt8();
    uint8_t w = FX::readPendingUInt8();
    uint8_t h = FX::readPendingLastUInt8();
    //SpritesABC::fillRect(x + xoff, y + yoff, w, h, WHITE);
    SpritesABC::drawBasic(
        x + xoff, y + yoff, w, h,
        font + FONT_HEADER_BYTES + offset_lo + offset_hi * 256,
        0, ards::vm.text_mode);
    x += xadv;
#else
    /*
        BEFORE
        =================
        &x    r24 r25
        y     r22 r23
        c     r20

        AFTER
        =================
        x     r24 r25
        y     r22 r21
        w     r20     (same as c)
        h     r18
        font  r14 r15 r16
    */

    register uint8_t  m    asm("r19");
    register uint16_t t    asm("r30");
    register int16_t  xv   asm("r24");
    register uint8_t  h    asm("r18");
    register uint24_t addr asm("r14");
    asm volatile(R"(
            ldi  %A[t], 3
            cbi  %[fxport], %[fxbit]
            out  %[spdr], %A[t]
            
            ; add datapage to addr
            lds  %A[addr], %[font]+0
            lds  %B[addr], %[font]+1
            lds  %C[addr], %[font]+2
            lds  %A[t], %[page]+0
            lds  %B[t], %[page]+1
            add  %B[addr], %A[t]
            adc  %C[addr], %B[t]

            ; add c*7 to addr
            ldi  %A[t], 7
            mul  %A[t], %[c]
            add  %A[addr], r0
            adc  %B[addr], r1
            clr  __zero_reg__
            adc  %C[addr], __zero_reg__

            out  %[spdr], %C[addr]

            rcall L%=_delay_17

            out  %[spdr], %B[addr]

            rjmp 1f
        L%=_delay_17:
            nop
        L%=_delay_16:
            nop
        L%=_delay_15:
            nop
        L%=_delay_14:
            nop
        L%=_delay_13:
            nop
        L%=_delay_12:
            nop
        L%=_delay_11:
            nop
        L%=_delay_10:
            nop
        L%=_delay_9:
            rjmp .+0
        L%=_delay_7:
            ret
        1:
            rcall L%=_delay_15

            out  %[spdr], %A[addr]

            lds  %A[addr], %[font]+0
            lds  %B[addr], %[font]+1
            lds  %C[addr], %[font]+2
            ldi  %A[t], lo8(-%[HEADER])
            sub  %A[addr], %A[t]
            ldi  %A[t], hi8(-%[HEADER])
            sbc  %B[addr], %A[t]
            ldi  %A[t], 0xff
            sbc  %C[addr], %A[t]
            rjmp .+0
            rjmp .+0

            out  %[spdr], __zero_reg__
            rcall L%=_delay_15
            
            cli
            out  %[spdr], __zero_reg__
            in   %A[m], %[spdr]
            sei

            ld   %A[xv], %a[xp]+
            ld   %B[xv], %a[xp]
            movw %A[t], %A[xv]
            add  %A[t], %[m]
            adc  %B[t], __zero_reg__
            st   %a[xp], %B[t]
            st   -%a[xp], %A[t]
            rjmp .+0

            cli
            out  %[spdr], __zero_reg__
            in   %[m], %[spdr]
            sei

            add  %A[xv], %[m]
            adc  %B[xv], __zero_reg__
            sbrc %[m], 7
            dec  %B[xv]
            rcall L%=_delay_9
            
            cli
            out  %[spdr], __zero_reg__
            in   %[m], %[spdr]
            sei

            add  %A[y], %[m]
            adc  %B[y], __zero_reg__
            sbrc %[m], 7
            dec  %B[y]
            rcall L%=_delay_9
            
            cli
            out  %[spdr], __zero_reg__
            in   %A[t], %[spdr]
            sei

            rcall L%=_delay_13
            
            cli
            out  %[spdr], __zero_reg__
            in   %B[t], %[spdr]
            sei

            add  %A[addr], %A[t]
            adc  %B[addr], %B[t]
            rcall L%=_delay_11
            
            cli
            out  %[spdr], __zero_reg__
            in   %[c], %[spdr]
            sei

            rcall L%=_delay_14
            
            in   %[h], %[spdr]
            sbi  %[fxport], %[fxbit]

        )"
        : [addr]   "=&r" (addr)
        , [t]      "=&d" (t)
        , [m]      "=&r" (m)
        , [xv]     "=&r" (xv)
        , [c]      "+&r" (c) // reused as w
        , [h]      "=&r" (h)
        , [y]      "+&r" (y)
        : [xp]     "e"   (&x) // it's modified but then restored
        , [fxport] "I"   (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]  "I"   (FX_BIT)
        , [spdr]   "I"   (_SFR_IO_ADDR(SPDR))
        , [page]   "i"   (&FX::programDataPage)
        , [font]   "i"   (&ards::vm.text_font)
        , [HEADER] "i"   (FONT_HEADER_BYTES)
        );
        SpritesABC::drawBasic(
            xv, y, (uint8_t)c, h, addr, 0,
            ards::vm.text_mode);
#endif
}

static void sys_wrap_text()
{
    auto ptr = vm_pop_begin();
    uint16_t tn = vm_pop<uint16_t>(ptr);
    char* p     = reinterpret_cast<char*>(vm_pop<uint16_t>(ptr));
    uint8_t  w  = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    uint24_t font = ards::vm.text_font;
    (void)FX::readEnd();

    char c;
    uint8_t cw = 0; // current width
    char* tp = p;   // pointer after last word break
    uint8_t tw = 0; // width at last word break
    uint16_t ttn;   // tn at last word break
    while((c = ld_inc(p)) != '\0' && tn != 0)
    {
        --tn;
        cw += font_get_x_advance(c);
        if(c == '\n')
        {
            cw = 0;
            tw = 0;
            tp = p;
            continue;
        }
        if(c == ' ')
        {
            tp = p;
            tw = cw;
            ttn = tn;
        }
        if(cw <= w) continue;
        if(tw == 0) continue;
        p = tp;
        *(tp - 1) = '\n';
        cw = 0;
        tw = 0;
        tn = ttn;
    }

    seek_to_pc();
}

static void sys_draw_text()
{
    auto ptr = vm_pop_begin();
    int16_t  x    = vm_pop<int16_t> (ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    uint24_t font = ards::vm.text_font;
    uint16_t tn   = vm_pop<uint16_t>(ptr);
    uint16_t tb   = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    
    (void)FX::readEnd();
    if(uint8_t(font >> 16) == 0xff)
        vm_error(ards::ERR_FNT);
    uint8_t line_height = font_get_line_height();
    int16_t bx = x;
    
    char const* p = reinterpret_cast<char const*>(tb);
    char c;
    while((c = ld_inc(p)) != '\0' && tn != 0)
    {
        --tn;
        if(c == '\n')
        {
            x = bx;
            y += line_height;
            continue;
        }
        
        draw_char(x, y, c);
    }
    
    seek_to_pc();
}

static void sys_draw_text_P()
{
    auto ptr = vm_pop_begin();
    int16_t  x    = vm_pop<int16_t> (ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    uint24_t font = ards::vm.text_font;
    uint24_t tn   = vm_pop<uint24_t>(ptr);
    uint24_t tb   = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    
    (void)FX::readEnd();
    if(uint8_t(font >> 16) == 0xff)
        vm_error(ards::ERR_FNT);
    uint8_t line_height = font_get_line_height();
    int16_t bx = x;
    
    char c;
    while(tn != 0)
    {
        c = fx_read_byte_inc(tb);
        if(c == '\0') break;
        --tn;
        
        if(c == '\n')
        {
            x = bx;
            y += line_height;
            continue;
        }
        
        draw_char(x, y, c);
    }
    
    seek_to_pc();
}

static void sys_text_width()
{
    auto ptr = vm_pop_begin();
    uint24_t font = ards::vm.text_font;
    uint16_t tn   = vm_pop<uint16_t>(ptr);
    uint16_t tb   = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    if(uint8_t(font >> 16) == 0xff)
        vm_error(ards::ERR_FNT);
    
    (void)FX::readEnd();
    char const* p = reinterpret_cast<char const*>(tb);
    char c;
    uint16_t wmax = 0;
    uint16_t w = 0;
    while((c = ld_inc(p)) != '\0' && tn != 0)
    {
        --tn;
        if(c == '\n')
        {
            if(w > wmax) wmax = w;
            w = 0;
            continue;
        }
        
        w += font_get_x_advance(c);;
    }
    if(w > wmax) wmax = w;
    
    vm_push<uint16_t>(wmax);
    
    seek_to_pc();
}

static void sys_text_width_P()
{
    auto ptr = vm_pop_begin();
    uint24_t font = ards::vm.text_font;
    uint24_t tn   = vm_pop<uint24_t>(ptr);
    uint24_t tb   = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    if(uint8_t(font >> 16) == 0xff)
        vm_error(ards::ERR_FNT);
    
    (void)FX::readEnd();
    char const* p = reinterpret_cast<char const*>(tb);
    char c;
    uint16_t wmax = 0;
    uint16_t w = 0;
    while(tn != 0)
    {
        c = fx_read_byte_inc(tb);
        if(c == '\0') break;
        --tn;
        
        if(c == '\n')
        {
            if(w > wmax) wmax = w;
            w = 0;
            continue;
        }
        
        w += font_get_x_advance(c);
    }
    if(w > wmax) wmax = w;
    
    vm_push<uint16_t>(wmax);
    
    seek_to_pc();
}

static void sys_strlen()
{
    auto ptr = vm_pop_begin();
    uint16_t n = vm_pop<uint16_t>(ptr);
    uint16_t b = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    char const* p = reinterpret_cast<char const*>(b);
    uint16_t t = 0;
    while(*p++ != '\0')
    {
        ++t;
        if(--n == 0) break;
    }
    vm_push(t);
}

static void sys_strlen_P()
{
    auto ptr = vm_pop_begin();
    uint24_t n = vm_pop<uint24_t>(ptr);
    uint24_t b = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
    fx_seek_data(b);
    uint24_t t = 0;
    while(FX::readPendingUInt8() != '\0')
    {
        ++t;
        if(--n == 0) break;
    }
    vm_push(t);
    (void)FX::readEnd();
    seek_to_pc();
}

static void sys_strcmp()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint16_t n1 = vm_pop<uint16_t>(ptr);
    uint16_t b1 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    char const* p0 = reinterpret_cast<char const*>(b0);
    char const* p1 = reinterpret_cast<char const*>(b1);
    char c0, c1;
    for(;;)
    {
        c0 = (char)ld_inc(p0);
        c1 = (char)ld_inc(p1);
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    vm_push<uint8_t>(c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
}

static void sys_strcmp_P()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
    fx_seek_data(b1);
    char const* p0 = reinterpret_cast<char const*>(b0);
    char c0, c1;
    for(;;)
    {
        c0 = (char)ld_inc(p0);
        c1 = (char)FX::readPendingUInt8();
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    vm_push<uint8_t>(c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
    (void)FX::readEnd();
    seek_to_pc();
}

static void sys_strcmp_PP()
{
    auto ptr = vm_pop_begin();
    uint24_t n0 = vm_pop<uint24_t>(ptr);
    uint24_t b0 = vm_pop<uint24_t>(ptr);
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
    char c0, c1;
    for(;;)
    {
        c0 = fx_read_byte_inc(b0);
        c1 = fx_read_byte_inc(b1);
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    vm_push<uint8_t>(c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
    seek_to_pc();
}

static void sys_strcpy()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint16_t n1 = vm_pop<uint16_t>(ptr);
    uint16_t b1 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    uint16_t nr = n0;
    uint16_t br = b0;
    char* p0 = reinterpret_cast<char*>(b0);
    char const* p1 = reinterpret_cast<char const*>(b1);
    for(;;)
    {
        uint8_t c = ld_inc(p1);
        st_inc(p0, c);
        if(c == 0) break;
        if(--n0 == 0) break;
        if(--n1 == 0) { st_inc(p0, 0); break; }
    }
    vm_push<uint16_t>(br);
    vm_push<uint16_t>(nr);
}

static void sys_strcpy_P()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
    fx_seek_data(b1);
    uint16_t nr = n0;
    uint16_t br = b0;
    char* p0 = reinterpret_cast<char*>(b0);
    for(;;)
    {
        uint8_t c = FX::readPendingUInt8();
        st_inc(p0, c);
        if(c == 0) break;
        if(--n0 == 0) break;
        if(--n1 == 0) { st_inc(p0, 0); break; }
    }
    (void)FX::readEnd();
    vm_push<uint16_t>(br);
    vm_push<uint16_t>(nr);
    seek_to_pc();
}

static void sys_memcpy()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint16_t n1 = vm_pop<uint16_t>(ptr);
    uint16_t b1 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    if(n0 != n1)
        vm_error(ards::ERR_CPY);
    memcpy(
        reinterpret_cast<void*>(b0),
        reinterpret_cast<void const*>(b1),
        n0);
}

static void sys_memcpy_P()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    FX::disable();
    if(n0 != n1)
        vm_error(ards::ERR_CPY);
    ards::detail::fx_read_data_bytes(
        b1, reinterpret_cast<void*>(b0), n0);
    seek_to_pc();
}

using format_char_func = void(*)(char c);

static void format_add_string(format_char_func f, char* tb, uint16_t tn)
{
    while(tn != 0)
    {
        uint8_t c = ld_inc(tb);
        if(c == '\0') return;
        f(c);
        --tn;
    }
}

static void format_add_prog_string(format_char_func f, uint24_t tb, uint24_t tn)
{
    while(tn != 0)
    {
        uint8_t c = fx_read_byte_inc(tb);
        if(c == '\0') return;
        f(c);
        --tn;
    }
}

static void format_add_int(
    format_char_func f, uint32_t x, bool sign, uint8_t base, int8_t w)
{
    char buf[10];
    char* tp = buf;
    
    if(sign && (int32_t)x < 0)
    {
        f('-');
        x = -x;
        --w;
    }

    if(x == 0)
    {
        st_inc(tp, '0');
        --w;
    }
    while(x != 0)
    {
        uint8_t r = x % base;
        x /= base;
        st_inc(tp, char(r + (r < 10 ? '0' : 'a' - 10)));
        --w;
    }

    while(w > 0)
    {
        st_inc(tp, '0');
        --w;
    }
    
    while(tp != buf)
        f(ld_predec(tp));
}

// TODO: store this stuff in a struct on stack
static void* format_user;

static void format_add_float(format_char_func f, float x, uint8_t prec)
{
    if(isnan(x))
    {
        f('n');
        f('a');
        f('n');
        return;
    }
    if(isinf(x))
    {
        f('i');
        f('n');
        f('f');
        return;
    }
    if(x > 4294967040.f || x < -4294967040.f)
    {
        f('o');
        f('v');
        f('f');
        return;
    }
    
    if(x < 0.0)
    {
        f('-');
        x = -x;
    }
    
    {
        float r = 0.5f;
        for(uint8_t i = 0; i < prec; ++i)
            r *= 0.1f;
        x += r;
    }
    
    uint32_t n = (uint32_t)x;
    float r = x - (double)n;
    format_add_int(f, n, false, 10, 0);
    
    if(prec > 0)
        f('.');
    while(int8_t(--prec) >= 0)
    {
        r *= 10.f;
        uint8_t t = (uint8_t)r;
        f((char)('0' + t));
        r -= t;
    }
}

__attribute__((flatten))
static void format_exec(format_char_func f)
{
    uint24_t fn;
    uint24_t fb;
    {
        auto ptr = vm_pop_begin();
        fn = vm_pop<uint24_t>(ptr);
        fb = vm_pop<uint24_t>(ptr);
        vm_pop_end(ptr);
    }
        
    while(fn != 0)
    {
        char c = (char)fx_read_byte_inc(fb);
        --fn;
        if(c != '%')
        {
            f(c);
            continue;
        }
        c = (char)fx_read_byte_inc(fb);
        --fn;
        switch(c)
        {
        case 'c':
        {
            auto ptr = vm_pop_begin();
            c = vm_pop<char>(ptr);
            vm_pop_end(ptr);
        }
            // fallthrough
        case '%':
            f(c);
            break;
        case 's':
        {
            uint16_t tn;
            uint16_t tb;
            {
                auto ptr = vm_pop_begin();
                tn = vm_pop<uint16_t>(ptr);
                tb = vm_pop<uint16_t>(ptr);
                vm_pop_end(ptr);
            }
            format_add_string(f, reinterpret_cast<char*>(tb), tn);
            break;
        }
        case 'S':
        {
            uint24_t tn;
            uint24_t tb;
            {
                auto ptr = vm_pop_begin();
                tn = vm_pop<uint24_t>(ptr);
                tb = vm_pop<uint24_t>(ptr);
                vm_pop_end(ptr);
            }
            format_add_prog_string(f, tb, tn);
            break;
        }
        case 'd':
        case 'u':
        case 'x':
        {
            uint32_t x;
            {
                auto ptr = vm_pop_begin();
                x = vm_pop<uint32_t>(ptr);
                vm_pop_end(ptr);
            }
            int8_t w = (int8_t)(fx_read_byte_inc(fb) - '0');
            --fn;
            format_add_int(f, x, c == 'd', c == 'x' ? 16 : 10, w);
            break;
        }
        case 'f':
        {
            float x;
            {
                auto ptr = vm_pop_begin();
                x = vm_pop<float>(ptr);
                vm_pop_end(ptr);
            }
            uint8_t prec = fx_read_byte_inc(fb) - '0';
            --fn;
            format_add_float(f, x, prec);
            break;
        }
        default:
            break;
        }
    }
    
}

struct format_user_buffer
{
    char* p;
    uint16_t n;
};

__attribute__((naked))
static void format_exec_to_buffer(char c)
{
    asm volatile(R"(
        lds  r30, %[format_user]+0
        lds  r31, %[format_user]+1
        ldd  r22, Z+2
        ldd  r23, Z+3
        cp   r22, __zero_reg__
        cpc  r23, __zero_reg__
        breq 1f
        subi r22, 1
        sbc  r23, __zero_reg__
        std  Z+2, r22
        std  Z+3, r23
    1:  ld   r26, Z
        ldd  r27, Z+1
        st   X+, r24
        st   Z, r26
        std  Z+1, r27
        ret
        )"
        :
        : [format_user] "" (format_user)
    );

#if 0
    // C implementation
    format_user_buffer* u = (format_user_buffer*)format_user;
    uint16_t n = u->n;
    if(n == 0) return;
    --n;
    u->n = n;
    st_inc(u->p, (uint8_t)c);
#endif
}

struct format_user_draw
{
    int16_t bx;
    int16_t x;
    int16_t y;
    uint8_t line_height;
};

static void format_exec_draw(char c)
{
    format_user_draw* u = (format_user_draw*)format_user;
    
    if(c == '\n')
    {
        u->x = u->bx;
        u->y += u->line_height;
    }
    else
    {
        draw_char(u->x, u->y, c);
    }
}

static void format_exec_debug_printf(char c)
{
    UEDATX = c;
}

static void sys_format()
{
    auto ptr = vm_pop_begin();
    uint16_t dn = vm_pop<uint16_t>(ptr);
    uint16_t db = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
    
    format_user_buffer user;
    user.p = reinterpret_cast<char*>(db);
    user.n = dn;
    format_user = &user;
    format_exec(format_exec_to_buffer);
    
    if(user.n != 0)
        *user.p = '\0';
    
    seek_to_pc();
}

static void sys_debug_printf()
{
    (void)FX::readEnd();
    
    format_exec(format_exec_debug_printf);
    
    seek_to_pc();
}

static void sys_draw_textf()
{
    format_user_draw user;
    format_user = &user;
    
    {
        auto ptr = vm_pop_begin();
        user.x = user.bx = vm_pop<int16_t> (ptr);
        user.y = vm_pop<int16_t> (ptr);
        vm_pop_end(ptr);
    }
    (void)FX::readEnd();

    {
        uint8_t t = uint8_t(ards::vm.text_font >> 16);
        if(t == 0xff)
            vm_error(ards::ERR_FNT);
    }
    user.line_height = font_get_line_height();
    
    format_exec(format_exec_draw);
    
    seek_to_pc();
}

static void sys_tones_play()
{
    auto ptr = vm_pop_begin();
    uint24_t song = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    
    if(!Arduboy2Audio::enabled())
        return;
    
    (void)FX::readEnd();
    
    ards::Tones::play(song);
    
    seek_to_pc();
}

static void sys_tones_playing()
{
    vm_push<bool>(ards::Tones::playing());
}

static void sys_tones_stop()
{
    ards::Tones::stop();
}

static void sys_audio_enabled()
{
    vm_push<bool>(Arduboy2Audio::enabled());
}

static void sys_audio_toggle()
{
    Arduboy2Audio::toggle();
    ards::Tones::setup();
}

static void sys_save_exists()
{
    uint16_t save_size;
    (void)FX::readEnd();
    fx_seek_data(10);
    union
    {
        uint16_t save_size;
        uint8_t b[2];
    } u;
    u.b[0] = FX::readPendingUInt8();
    u.b[1] = FX::readPendingLastUInt8();
    bool r = false;
    if(u.save_size > 0 && u.save_size <= 1024)
    {
        uint16_t t;
        FX::seekSave(0);
        r = (FX::readPendingLastUInt16() == u.save_size);
    }
    vm_push<bool>(r);
    seek_to_pc();
}

static void sys_save()
{
    uint16_t save_size;
    (void)FX::readEnd();
    fx_seek_data(10);
    union
    {
        uint16_t save_size;
        uint8_t b[2];
    } u;
    u.b[0] = FX::readPendingUInt8();
    u.b[1] = FX::readPendingLastUInt8();
    if(u.save_size > 0 && u.save_size <= 1024)
        FX::saveGameState(&ards::vm.globals[0], u.save_size);
    FX::waitWhileBusy();
    seek_to_pc();
}

static void sys_load()
{
    (void)FX::readEnd();
    fx_seek_data(10);
    union
    {
        uint16_t save_size;
        uint8_t b[2];
    } u;
    u.b[0] = FX::readPendingUInt8();
    u.b[1] = FX::readPendingLastUInt8();
    bool r = false;
    if(u.save_size > 0 && u.save_size <= 1024)
        r = (bool)FX::loadGameState(&ards::vm.globals[0], u.save_size);
    vm_push<bool>(r);
    seek_to_pc();
}

__attribute__((naked))
static void sys_sin()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            ret
        )"
        :
        : [f] "" (sinf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, sinf(x));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_cos()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            ret
        )"
        :
        : [f] "" (cosf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, cosf(x));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_tan()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            ret
        )"
        :
        : [f] "" (tanf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, tanf(x));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_atan2()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            ld   r21, -Y
            ld   r20, -Y
            ld   r19, -Y
            ld   r18, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            sts  %[vmsp], r28
            ret
        )"
        :
        : [vmsp] "i" (&ards::vm.sp)
        , [f]    ""  (atan2)
        );
#else
    auto ptr = vm_pop_begin();
    float y = vm_pop<float>(ptr);
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, atan2(y, x));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_floor()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            ret
        )"
        :
        : [f] "" (floorf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, floorf(x));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_ceil()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            ret
        )"
        :
        : [f] "" (ceilf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, ceilf(x));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_round()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            ret
        )"
        :
        : [f] "" (roundf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, roundf(x));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_mod()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            ld   r21, -Y
            ld   r20, -Y
            ld   r19, -Y
            ld   r18, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            sts  %[vmsp], r28
            ret
        )"
        :
        : [vmsp] "i" (&ards::vm.sp)
        , [f]    ""  (fmodf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    float y = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, fmodf(x, y));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_pow()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            ld   r21, -Y
            ld   r20, -Y
            ld   r19, -Y
            ld   r18, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            sts  %[vmsp], r28
            ret
        )"
        :
        : [vmsp] "i" (&ards::vm.sp)
        , [f]    ""  (powf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    float y = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, powf(x, y));
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_sqrt()
{
#if 1
    asm volatile(R"(
            ld   r25, -Y
            ld   r24, -Y
            ld   r23, -Y
            ld   r22, -Y
            call %x[f]
            st   Y+, r22
            st   Y+, r23
            st   Y+, r24
            st   Y+, r25
            ret
        )"
        :
        : [f] "" (sqrtf)
        );
#else
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push_unsafe<float>(ptr, sqrtf(x));
    vm_pop_end(ptr);
#endif
}

static void sys_generate_random_seed()
{
    vm_push<uint32_t>(Arduboy2Base::generateRandomSeed());
}

static void sys_init_random_seed()
{
    Arduboy2Base::initRandomSeed();
}

static void sys_random()
{
    vm_push<uint32_t>((uint32_t)random());
}

static void sys_random_range()
{
    auto ptr = vm_pop_begin();
    uint32_t a = vm_pop<uint32_t>(ptr);
    uint32_t b = vm_pop<uint32_t>(ptr);
    uint32_t t = a;
    if(a < b)
        t += random() % (b - a);
    vm_push_unsafe<uint32_t>(ptr, t);
    vm_pop_end(ptr);
}

__attribute__((naked))
static void sys_set_text_font()
{
#if 1
    asm volatile(R"(
            ld   r24, -Y
            sts  %[font]+2, r24
            ld   r24, -Y
            sts  %[font]+1, r24
            ld   r24, -Y
            sts  %[font]+0, r24
            sts  %[vmsp], r28
            ret
        )"
        :
        : [font] "i" (&ards::vm.text_font)
        , [vmsp] "i" (&ards::vm.sp)
        );
#else
    auto ptr = vm_pop_begin();
    ards::vm.text_font = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
#endif
}

__attribute__((naked))
static void sys_set_text_color()
{
#if 1
    asm volatile(R"(
            ld   r24, -Y
            sts  %[vmsp], r28
            ldi  r25, %[modb]
            cpse r24, __zero_reg__
            ldi  r25, %[modw]
            sts  %[mode], r25
            ret
        )"
        :
        : [mode] "i" (&ards::vm.text_mode)
        , [vmsp] "i" (&ards::vm.sp)
        , [modw] "i" (SpritesABC::MODE_SELFMASK)
        , [modb] "i" (SpritesABC::MODE_SELFMASK_ERASE)
        );
#else
    auto ptr = vm_pop_begin();
    uint8_t t = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    ards::vm.text_mode = t ? SpritesABC::MODE_SELFMASK : SpritesABC::MODE_SELFMASK_ERASE;
#endif
}

sys_func_t const SYS_FUNCS[] PROGMEM =
{
    sys_display,
    sys_display_noclear,
    sys_get_pixel,
    sys_draw_pixel,
    sys_draw_hline,
    sys_draw_vline,
    sys_draw_line,
    sys_draw_rect,
    sys_draw_filled_rect,
    sys_draw_circle,
    sys_draw_filled_circle,
    sys_draw_sprite,
    sys_draw_sprite_selfmask,
    sys_draw_text,
    sys_draw_text_P,
    sys_draw_textf,
    sys_text_width,
    sys_text_width_P,
    sys_wrap_text,
    sys_set_text_font,
    sys_set_text_color,
    sys_set_frame_rate,
    sys_next_frame,
    sys_idle,
    sys_debug_break,
    sys_debug_printf,
    sys_assert,
    sys_poll_buttons,
    sys_just_pressed,
    sys_just_released,
    sys_pressed,
    sys_any_pressed,
    sys_not_pressed,
    sys_millis,
    sys_memcpy,
    sys_memcpy_P,
    sys_strlen,
    sys_strlen_P,
    sys_strcmp,
    sys_strcmp_P,
    sys_strcmp_PP,
    sys_strcpy,
    sys_strcpy_P,
    sys_format,
    sys_tones_play,
    sys_tones_playing,
    sys_tones_stop,
    sys_audio_enabled,
    sys_audio_toggle,
    sys_save_exists,
    sys_save,
    sys_load,
    sys_sin,
    sys_cos,
    sys_tan,
    sys_atan2,
    sys_floor,
    sys_ceil,
    sys_round,
    sys_mod,
    sys_pow,
    sys_sqrt,
    sys_generate_random_seed,
    sys_init_random_seed,
    sys_random,
    sys_random_range,

};
