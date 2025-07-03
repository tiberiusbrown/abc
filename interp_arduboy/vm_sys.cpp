#ifndef EEPROM_h
#define EEPROM_h
#endif

#include <Arduboy2.h>
#include <Arduboy2Audio.h>
#include <ArduboyFX.h>

#include "SpritesABC.hpp"

#include "ards_shades.hpp"
#include "ards_tone.hpp"

#include "ards_vm.hpp"

#include <math.h>

#define TILEMAP_USE_DRAW_SPRITE_HELPER 0

extern uint8_t abc_seed[4];
uint8_t abc_seed[4];

constexpr uint8_t FONT_HEADER_PER_CHAR = 7;
constexpr uint8_t FONT_HEADER_OFFSET = 1;
constexpr uint16_t FONT_HEADER_BYTES =
    FONT_HEADER_PER_CHAR * 256 + FONT_HEADER_OFFSET;

using sys_func_t = void(*)();
extern sys_func_t const SYS_FUNCS[] PROGMEM;
extern "C" void vm_error(ards::error_t e);

template<class T>
[[gnu::always_inline]] inline uint8_t ld_inc(T*& p)
{
    uint8_t t;
    asm volatile("ld %[t], %a[p]+\n" : [t]"=&r"(t), [p]"+&e"(p));
    return t;
}

template<class T>
[[gnu::always_inline]] inline uint8_t ld_predec(T*& p)
{
    uint8_t t;
    asm volatile("ld %[t], -%a[p]\n" : [t]"=&r"(t), [p]"+&e"(p));
    return t;
}

template<class T>
[[gnu::always_inline]] inline void st_inc(T*& p, uint8_t t)
{
    asm volatile("st %a[p]+, %[t]\n" : [p]"+&e"(p) : [t]"r"(t));
}

[[gnu::always_inline]] inline uint8_t* vm_pop_begin()
{
    uint8_t* r;
    asm volatile(
        "lds  %A[r], %[vm_sp]\n"
        "ldi  %B[r], 1\n"
        : [r]     "=&d" (r)
        : [vm_sp] ""    (&ards::vm.sp));
    return r;
}

[[gnu::always_inline]] inline void vm_pop_end(uint8_t* ptr)
{
    ards::vm.sp = (uint8_t)(uintptr_t)ptr;
}

template<class T>
[[gnu::always_inline]] inline T vm_pop(uint8_t*& ptr)
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
[[ gnu::always_inline ]]
inline void vm_push_n_unsafe(uint8_t*& ptr, byte_storage<N> x)
{
    static_assert(N <= 4, "");
    if(N >= 1) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[0]));
    if(N >= 2) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[1]));
    if(N >= 3) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[2]));
    if(N >= 4) asm volatile("st %a[ptr]+, %[t]\n" : [ptr] "+&e" (ptr) : [t] "r" (x.d[3]));
}

template<size_t N>
[[ gnu::always_inline ]]
inline void vm_push_n(uint8_t* ptr, byte_storage<N> x)
{
    if((uint8_t)(uintptr_t)ptr < uint8_t(256 - N))
    {
        vm_push_n_unsafe<N>(ptr, x);
        ards::vm.sp = (uint8_t)(uintptr_t)ptr;
        return;
    }
    vm_error(ards::ERR_DST);
}

template<class T>
[[gnu::always_inline]]
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

extern "C" void vm_push_u8(uint8_t x);
[[ gnu::used, gnu::flatten ]]
void vm_push_u8(uint8_t x)
{
    vm_push<uint8_t>(x);
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
)"

#if ABC_SHADES != 2
R"(
            ; see if we need to call shades_display()
            lds  r16, %[needs_render]
            cp   r16, __zero_reg__
            breq no_shades_display
            clr  r1
            rcall L%=_delay_13
            sbi  %[fxport], %[fxbit]
            call %x[shades_display]
            rjmp 1b
        no_shades_display:
)"
#endif

R"(
            ; see if we need to call ards::Tones::update()
            lds  r16, %[tones_reload]
            cp   r16, __zero_reg__
            breq 2f
            clr  r1
            rcall L%=_delay_13
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
            lpm
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
        , [tones_reload]    ""  (&ards::detail::reload_needed)
#if ABC_SHADES != 2
        , [needs_render]    ""  (&ards::vm.needs_render)
        , [shades_display]  ""  (shades_display)
#endif
        );
}

__attribute__((noinline, naked))
static void fx_read_byte_inc_helper(uint24_t& fb)
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
        rcall L%=_delay_11
        nop  ; avoid TCO from linker relaxing
        ret

    L%=_delay_17:
        lpm
        lpm
    L%=_delay_11:
        rjmp .+0
        rjmp .+0
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

[[gnu::always_inline]]
static uint8_t fx_read_byte_inc(uint24_t& fb)
{
    uint8_t r;
    fx_read_byte_inc_helper(fb);
    asm volatile(R"(
            in   %[r], %[spdr]
            sbi  %[fxport], %[fxbit]
        )"
        : [r]        "=&r" (r)
        : [fxport]   "i"   (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]    "i"   (FX_BIT)
        , [spdr]     "i"   (_SFR_IO_ADDR(SPDR))
        );
    return r;
}

static uint16_t fx_read_word_inc(uint24_t& fb)
{
    uint16_t r;
    void* p;
    uint8_t t;
    fx_read_byte_inc_helper(fb);
    asm volatile(R"(
            in   %A[r], %[spdr]
            movw %A[p], %A[fb]
            ld   %A[fb], %a[p]+
            ld   %B[fb], %a[p]+
            ld   %[t], %a[p]+
            adiw %A[fb], 1
            adc  %[t], __zero_reg__
            st   -%a[p], %[t]
            st   -%a[p], %B[fb]
            st   -%a[p], %A[fb]
            rjmp .+0
            in   %B[r], %[spdr]
            sbi  %[fxport], %[fxbit]
        )"
        : [r]        "=&r" (r)
        , [p]        "=&e" (p)
        , [t]        "=&r" (t)
        , [fb]       "+&w" (fb)
        : [fxport]   "i"   (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]    "i"   (FX_BIT)
        , [spdr]     "i"   (_SFR_IO_ADDR(SPDR))
        );
    return r;
}

extern unsigned long volatile timer0_overflow_count;
void wait_for_frame_timing()
{
    uint8_t fd = ards::vm.frame_dur;
    uint8_t fs = ards::vm.frame_start;
    ards::vm.frame_start = fs + fd;
#if ABC_SHADES != 2
    (void)FX::readEnd();
#endif
    for(;;)
    {
        sei();

        uint8_t n;
        asm volatile("lds %[n], %[t]\n" : [n] "=&r" (n) : [t] "" (&timer0_overflow_count));
        
        n -= fs;
        if(n >= fd)
            break;

        if(++n < fd)
        {
            Arduboy2Base::idle();
        }

#if ABC_SHADES != 2
        asm volatile("lds %[n], %[t]\n" : [n] "=&r" (n) : [t] "" (&ards::vm.needs_render));
        if(n) shades_display();
#endif
    }
    Arduboy2Base::pollButtons();
#if ABC_SHADES != 2
    seek_to_pc();
#endif
}

#if ABC_SHADES == 2
static void sys_display()
{
    FX::disable();
    FX::display(true);
    wait_for_frame_timing();
    seek_to_pc();
}
#endif

#if ABC_SHADES == 2
static void sys_display_noclear()
{
    FX::disable();
    FX::display(false);
    wait_for_frame_timing();
    seek_to_pc();
}
#endif

static void sys_draw_pixel()
{
#if ABC_SHADES == 2
#if 0
    int16_t x;// = vm_pop<int16_t>(ptr);
    int16_t y;// = vm_pop<int16_t>(ptr);
    uint8_t color;// = vm_pop<uint8_t>(ptr);
    //if(uint16_t(x) >= WIDTH || uint16_t(y) >= HEIGHT)
    //    return;
    uint8_t* p;
    asm volatile(R"(
            lds  %A[p], %[vm_sp]
            ldi  %B[p], 1
            ld   %B[x], -%a[p]
            ld   %A[x], -%a[p]
            ld   %B[y], -%a[p]
            ld   %A[y], -%a[p]
            ld   %[c], -%a[p]
            sts  %[vm_sp], %A[p]

            cpi  %A[x], 128
            cpc  %B[x], __zero_reg__
            brsh 1f
            cpi  %A[y], 64
            cpc  %B[y], __zero_reg__
            brsh 1f

            ldi  %B[y], 1
            sbrc %A[y], 1
            ldi  %B[y], 4
            sbrc %A[y], 0
            lsl  %B[y]
            sbrc %A[y], 2
            swap %B[y]
            ldi  %A[p], 128/8
            andi %A[y], 0xf8
            mul  %A[y], %A[p]
            movw %A[p], r0
            subi %A[p], lo8(-(%[buf]))
            sbci %B[p], hi8(-(%[buf]))
            clr  __zero_reg__
            add  %A[p], %A[x]
            adc  %B[p], __zero_reg__
            ld   r0, %a[p]
            or   r0, %B[y]
            sbrs %[c], 0
            eor  r0, %B[y]
            st   %a[p], r0
        1:
        )"
        : [p]     "+&e" (p)
        , [x]     "=&d" (x)
        , [y]     "=&d" (y)
        , [c]     "=&r" (color)
        : [buf]   ""    (Arduboy2Base::sBuffer)
        , [vm_sp] ""    (&ards::vm.sp)
    );
#else
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::drawPixel(x, y, color);
#endif
#else
    ards::vm.sp -= 5;
#endif
}

static void sys_get_pixel()
{
#if ABC_SHADES == 2
#if 1
    uint8_t x;
    uint8_t y;
    uint8_t m;
    uint8_t color;
    uint8_t* p;
    uint8_t* b;
    asm volatile(R"(
            lds  %A[p], %[vm_sp]
            ldi  %B[p], 1
            ld   %[x], -%a[p]
            ld   %[y], -%a[p]

            clr  %[c]
            cpi  %[x], 128
            brsh 1f
            cpi  %[y], 64
            brsh 1f

            ldi  %[m], 1
            sbrc %[y], 1
            ldi  %[m], 4
            sbrc %[y], 0
            lsl  %[m]
            sbrc %[y], 2
            swap %[m]
            ldi  %A[b], 128/8
            andi %[y], 0xf8
            mul  %[y], %A[b]
            movw %A[b], r0
            clr  __zero_reg__
            subi %A[b], lo8(-(%[buf]))
            sbci %B[b], hi8(-(%[buf]))
            add  %A[b], %[x]
            adc  %B[b], __zero_reg__
            ld   r0, %a[b]
            and  r0, %[m]
            breq .+2
            inc  %[c]

        1:  st   %a[p]+, %[c]
            sts  %[vm_sp], %A[p]
        )"
        : [p]     "=&e" (p)
        , [b]     "=&e" (b)
        , [x]     "=&r" (x)
        , [y]     "=&d" (y)
        , [m]     "=&d" (m)
        , [c]     "=&r" (color)
        : [buf]   ""    (Arduboy2Base::sBuffer)
        , [vm_sp] ""    (&ards::vm.sp)
    );
#else
    // TODO: optimize
    auto ptr = vm_pop_begin();
    uint8_t x = vm_pop<uint8_t>(ptr);
    uint8_t y = vm_pop<uint8_t>(ptr);
    uint8_t c = x < 128 && y < 64 ? Arduboy2Base::getPixel(x, y) : 0;
    vm_push_unsafe<uint8_t>(ptr, c);
    vm_pop_end(ptr);
#endif
#else
    auto ptr = vm_pop_begin();
    ptr -= 2;
    vm_push_unsafe<uint8_t>(ptr, 0);
    vm_pop_end(ptr);
#endif
}

#if ABC_SHADES == 2
__attribute__((naked))
static void sys_draw_hline()
{
#if 0
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, w, 1, color);
#endif
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
}
#else
static void sys_draw_hline()
{
    // TODO
    ards::vm.sp -= 6;
}
#endif

#if ABC_SHADES == 2
__attribute__((naked))
static void sys_draw_vline()
{
#if 0
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, 1, h, color);
#endif
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
}
#else
static void sys_draw_vline()
{
    // TODO
    ards::vm.sp -= 6;
}
#endif

static void sys_draw_line()
{
#if ABC_SHADES == 2
    auto ptr = vm_pop_begin();
    int16_t x0 = vm_pop<int16_t>(ptr);
    int16_t y0 = vm_pop<int16_t>(ptr);
    int16_t x1 = vm_pop<int16_t>(ptr);
    int16_t y1 = vm_pop<int16_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::drawLine(x0, y0, x1, y1, color);
#else
    ards::vm.sp -= 9;
#endif
}

#if ABC_SHADES != 2
static void shades_sys_draw_rect(bool filled)
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t c = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    shades_draw_rect(x, y, w, h, c, filled);
}
#endif

static void sys_draw_rect()
{
#if ABC_SHADES == 2
    auto ptr = vm_pop_begin();
#if 1
    int16_t x;
    int16_t y;
    uint8_t w;
    uint8_t h;
    uint8_t c;
    asm volatile(R"(
            ld %B[x], -%a[p]
            ld %A[x], -%a[p]
            ld %B[y], -%a[p]
            ld %A[y], -%a[p]
            ld %[w], -%a[p]
            ld %[h], -%a[p]
            ld %[c], -%a[p]
        )"
        : [p] "+&e" (ptr)
        , [x] "=&r" (x)
        , [y] "=&r" (y)
        , [w] "=&r" (w)
        , [h] "=&r" (h)
        , [c] "=&r" (c)
    );
#else
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t c = vm_pop<uint8_t>(ptr);
#endif
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, w, 1, c);
    SpritesABC::fillRect(x, y, 1, h, c);
    SpritesABC::fillRect(x, y + h - 1, w, 1, c);
    SpritesABC::fillRect(x + w - 1, y, 1, h, c);
#else
    shades_sys_draw_rect(false);
#endif
}

#if ABC_SHADES == 2
__attribute__((naked))
static void sys_draw_filled_rect()
{
#if 0
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t c = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesABC::fillRect(x, y, w, h, c);
#endif
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
}
#else
static void sys_draw_filled_rect()
{
    shades_sys_draw_rect(true);
}
#endif

static void sys_draw_circle()
{
#if ABC_SHADES == 2
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t r = vm_pop<uint8_t>(ptr);
    uint8_t c = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::drawCircle(x, y, r, c);
#else
    ards::vm.sp -= 6;
#endif
}

[[gnu::always_inline]]
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
#if ABC_SHADES == 2
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t r = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);

    draw_fast_vline(x, y-r, 2 * r + 1, color);
    fill_circle_helper(x, y, r, 3, 0, color);
    //Arduboy2Base::fillCircle(x, y, r, color);
#else
    ards::vm.sp -= 6;
#endif
}

template<int N>
static void sys_pop_n()
{
    ards::vm.sp -= N;
}

static void sys_set_frame_rate()
{
    auto ptr = vm_pop_begin();
    uint8_t fr = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    ards::vm.frame_dur = fr > 10 ? 1024u / fr : fr == 0 ? 0 : 100;
}

#if 1
#define sys_idle Arduboy2Base::idle
#else
static void sys_idle()
{
    Arduboy2Base::idle();
}
#endif

#if 1
extern "C" void sys_debug_break(); // defined in I_P0
#else
static void sys_debug_break()
{
    asm volatile("break\n");
}
#endif

#if 1
extern "C" void sys_assert(); // defined in I_P0
#else
static void sys_assert()
{
    auto ptr = vm_pop_begin();
    uint8_t b = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    if(!b) vm_error(ards::ERR_ASS);
}
#endif

static void sys_buttons()
{
    vm_push_u8((uint8_t)Arduboy2Base::buttonsState());
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

[[ gnu::naked, gnu::noinline ]]
static void draw_sprite_helper(uint8_t selfmask_bit)
{
    asm volatile(R"(

            ; PTR      Z
            ; PTR_A    r30
            ; PTR_B    r31
            ; NUM      r19
            ; FRAME_A  r26
            ; FRAME_B  r27
            ; MASK     r21

            ; X_A      r24
            ; X_B      r25
            ; Y_A      r22
            ; Y_B      r23
            ; W        r20
            ; H        r18
            ; IMAGE_A  r14
            ; IMAGE_B  r15
            ; IMAGE_C  r16
            ; MODE     r12

            sbi  %[fxport], %[fxbit]
            cbi  %[fxport], %[fxbit]
            ldi  r20, 3
            out  %[spdr], r20

            lds  r30, %[vmsp]
            ldi  r31, 1
            sbiw r30, 4
            ld   r20, -Z
            ld   r19, -Z
            ld   r18, -Z
            lds  r26, %[datapage]+0
            lds  r27, %[datapage]+1
            add  r26, r19
            adc  r27, r20

            out  %[spdr], r27

            push r12
            push r14
            push r15
            push r16

            movw r14, r18
            mov  r16, r20

            ldd  r22, Z+3
            ldd  r23, Z+4

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
        draw_sprite_helper_delay_7:
            ret

        1:  ldi  r20, 5

            out  %[spdr], r26
            mov  r21, r24
            ldd  r24, Z+5
            ldd  r25, Z+6
            ld   r27, -Z
            ld   r26, -Z
            sts  %[vmsp], r30
            lpm
            lpm

            out  %[spdr], r14
            add  r14, r20
            adc  r15, __zero_reg__
            adc  r16, __zero_reg__
            rcall draw_sprite_helper_delay_13
            out  %[spdr], __zero_reg__
            rcall draw_sprite_helper_delay_15

            cli
            out  %[spdr], __zero_reg__
            in   r20, %[spdr]
            sei
            rcall draw_sprite_helper_delay_13

            cli
            out  %[spdr], __zero_reg__
            in   r18, %[spdr]
            sei
            subi r18, -7
            andi r18, 0xf8

            ; add frame offset to image
            mov  r30, r18
            lsr  r30
            lsr  r30
            lsr  r30

            rjmp 1f
        draw_sprite_helper_error:
            ldi  r24, %[err]
            jmp  %x[vm_error]
        1:
            lpm
            rjmp .+0

            cli
            out  %[spdr], __zero_reg__
            in   r12, %[spdr]
            sei

            ; add frame offset to image
            or   r12, r21
            sbrc r12, 0
            lsl  r30
            mul  r30, r20
            movw r30, r0
            mul  r30, r27
            add  r15, r0
            adc  r16, r1
            clr  __zero_reg__
            rjmp .+0

            cli
            out  %[spdr], __zero_reg__
            in   r19, %[spdr]
            sei

            ; add frame offset to image
            mul  r31, r27
            add  r16, r0
            mul  r31, r26
            add  r15, r0
            adc  r16, r1
            mul  r30, r26
            add  r14, r0
            adc  r15, r1
            clr  __zero_reg__
            adc  r16, __zero_reg__
            cp   r26, r19

            in   r19, %[spdr]
            sbi  %[fxport], %[fxbit]

            cpc  r27, r19
            brsh draw_sprite_helper_error

            call %x[draw]

            pop  r16
            pop  r15
            pop  r14
            pop  r12
)"
#if !TILEMAP_USE_DRAW_SPRITE_HELPER
            "jmp  %x[seek]\n"
#else
            "ret\n"
#endif
        :
        : [vmsp]     ""    (&ards::vm.sp)
        , [fxport]   "i"   (_SFR_IO_ADDR(FX_PORT))
        , [fxbit]    "i"   (FX_BIT)
        , [spdr]     "i"   (_SFR_IO_ADDR(SPDR))
        , [spsr]     "i"   (_SFR_IO_ADDR(SPSR))
        , [sreg]     "i"   (_SFR_IO_ADDR(SREG))
        , [err]      "i"   (ards::ERR_FRM)
        , [datapage] ""    (&FX::programDataPage)
        , [vm_error] ""    (vm_error)
        , [draw]     ""    (SpritesABC::drawBasicFX)
        , [seek]     ""    (seek_to_pc)
    );
}

static void sys_draw_sprite()
{
#if ABC_SHADES == 2
    draw_sprite_helper(0);
#if TILEMAP_USE_DRAW_SPRITE_HELPER
    seek_to_pc();
#endif
#else
    auto ptr = vm_pop_begin();
#if 1
    int16_t x, y;
    uint24_t img;
    uint16_t frame;
    asm volatile(R"(
            ld %B[x], -%a[ptr]
            ld %A[x], -%a[ptr]
            ld %B[y], -%a[ptr]
            ld %A[y], -%a[ptr]
            ld %C[img], -%a[ptr]
            ld %B[img], -%a[ptr]
            ld %A[img], -%a[ptr]
            ld %B[frame], -%a[ptr]
            ld %A[frame], -%a[ptr]
        )"
        : [x]     "=&r" (x)
        , [y]     "=&r" (y)
        , [img]   "=&r" (img)
        , [frame] "=&r" (frame)
        , [ptr]   "+&e" (ptr)
    );
#else
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint24_t img = vm_pop<uint24_t>(ptr);
    uint16_t frame = vm_pop<uint16_t>(ptr);
#endif
    vm_pop_end(ptr);
    FX::disable();
    shades_draw_sprite(x, y, img, frame);
    seek_to_pc();
#endif
}

static void sys_draw_sprite_selfmask()
{
#if ABC_SHADES == 2
    draw_sprite_helper(4);
#if TILEMAP_USE_DRAW_SPRITE_HELPER
    seek_to_pc();
#endif
#else
    ards::vm.sp -= 9; // x:2 y:2 img:3 frame:2
#endif
}

[[ gnu::always_inline ]]
static uint24_t mul_24_16_16(uint16_t a, uint16_t b)
{
    uint24_t r;
    /*
              A1 A0
              B1 B0
              =====
              A0*B0
           A1*B0
           A0*B1
        A1*B1
        ===========
           R2 R1 R0
    */
    asm volatile(R"(
            mul  %A[a], %A[b]
            movw %A[r], r0
            mul  %B[a], %B[b]
            mov  %C[r], r0
            mul  %A[a], %B[b]
            add  %B[r], r0
            adc  %C[r], r1
            mul  %B[a], %A[b]
            add  %B[r], r0
            adc  %C[r], r1
            clr  __zero_reg__
        )"
        : [r] "=&r" (r)
        : [a] "r"   (a)
        , [b] "r"   (b)
    );
    return r;
}

static uint16_t fx_read_pending_uint16_le()
{
    uint16_t t = FX::readPendingUInt16();
    asm volatile(R"(
            mov r0, %A[t]
            mov %A[t], %B[t]
            mov %B[t], r0
        )"
        : [t] "+&r" (t)
    );
    return t;
}

static uint16_t fx_read_pending_last_uint16_le()
{
    uint16_t t = FX::readPendingLastUInt16();
    asm volatile(R"(
            mov r0, %A[t]
            mov %A[t], %B[t]
            mov %B[t], r0
        )"
        : [t] "+&r" (t)
    );
    return t;
}

static void sys_draw_tilemap()
{
    auto ptr = vm_pop_begin();
#if 1
    int16_t x,y;
    uint24_t img, tm;
    asm volatile(R"(
            ld %B[x], -%a[ptr]
            ld %A[x], -%a[ptr]
            ld %B[y], -%a[ptr]
            ld %A[y], -%a[ptr]
            ld %C[img], -%a[ptr]
            ld %B[img], -%a[ptr]
            ld %A[img], -%a[ptr]
            ld %C[tm], -%a[ptr]
            ld %B[tm], -%a[ptr]
            ld %A[tm], -%a[ptr]
        )"
        : [x]   "=&r" (x)
        , [y]   "=&r" (y)
        , [img] "=&r" (img)
        , [tm]  "=&r" (tm)
        , [ptr] "+&e" (ptr)
    );
#else
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint24_t img = vm_pop<uint24_t>(ptr);
    uint24_t tm = vm_pop<uint24_t>(ptr);
#endif
    vm_pop_end(ptr);
    FX::disable();

    if(x >= 128) goto end;
    if(y >=  64) goto end;

    fx_seek_data(tm);
    uint8_t format = FX::readPendingUInt8();
    asm volatile("dec %0\n" : "+r" (format)); // better codegen than format -= 1;
    uint16_t nrow = fx_read_pending_uint16_le();
    uint16_t ncol = fx_read_pending_last_uint16_le();
    tm += 5;

    // sprite dimensions and number
    fx_seek_data(img);
    uint8_t sw = FX::readPendingUInt8();
#if TILEMAP_USE_DRAW_SPRITE_HELPER
    uint8_t sh = FX::readPendingLastUInt8();
#else
    uint8_t sh = FX::readPendingUInt8();
    uint8_t mode = FX::readPendingUInt8();
    uint16_t num_frames = fx_read_pending_last_uint16_le();
#if ABC_SHADES == 2
    img += 5;
#endif
#endif

    uint16_t r = 0, c = 0;
#if 1
    asm volatile(R"(
        1:  add  %A[y], %[sh]
            adc  %B[y], __zero_reg__
            brge 2f
            subi %A[r], lo8(-1)
            sbci %B[r], hi8(-1)
            rjmp 1b
        2:  sub  %A[y], %[sh]
            sbc  %B[y], __zero_reg__
        3:  add  %A[x], %[sw]
            adc  %B[x], __zero_reg__
            brge 4f
            subi %A[c], lo8(-1)
            sbci %B[c], hi8(-1)
            rjmp 3b
        4:  sub  %A[x], %[sw]
            sbc  %B[x], __zero_reg__
        )"
        : [y]  "+&r" (y)
        , [x]  "+&r" (x)
        , [r]  "+&d" (r)
        , [c]  "+&d" (c)
        : [sh] "r"   (sh)
        , [sw] "r"   (sw)
    );
#elif 0
    while((y += sh) <= 0) { ++r; } y -= sh;
    while((x += sw) <= 0) { ++c; } x -= sw;
#else
    if(y < 0)
    {
        r -= y / sh;
        y += r * sh;
    }
    if(x < 0)
    {
        c -= x / sw;
        x += c * sw;
    }
#endif

    constexpr uint8_t BUF_BYTES = 16;

    for(; r < nrow && y < HEIGHT; ++r, y += sh)
    {
        int8_t tx = (int8_t)x;
        uint8_t buf[BUF_BYTES];
        uint8_t i = 0;
        for(uint16_t tc = c; tc < ncol /*&& tx < WIDTH*/; ++tc, /*tx += sw,*/ i += format + 1)
        {
            uint16_t frame;
            if((i &= (BUF_BYTES - 1)) == 0)
            {
                uint24_t t = mul_24_16_16(r, ncol) + tc;
                if(format != 0)
                    t += t;
                t += tm;
                ards::detail::fx_read_data_bytes(t, buf, BUF_BYTES);
            }
            {
                uint8_t const* p = &buf[i];
                asm volatile(R"(
                        ld   %A[frame], %a[p]+
                        clr  %B[frame]
                        cpse %[format], __zero_reg__
                        ld   %B[frame], %a[p]
                    1:
                    )"
                    : [p]      "+&e" (p)
                    , [frame]  "=&r" (frame)
                    : [format] "d"   (format)
                );
            }
            if(frame != 0)
            {
                frame -= 1;
#if ABC_SHADES == 2
#if TILEMAP_USE_DRAW_SPRITE_HELPER
                ptr = vm_pop_begin();
                vm_push_unsafe<uint16_t>(ptr, frame);
                vm_push_unsafe<uint24_t>(ptr, img);
                vm_push_unsafe<int16_t>(ptr, y);
                vm_push_unsafe<int16_t>(ptr, tx);
                vm_pop_end(ptr);
                draw_sprite_helper(0);
#else
                if(frame >= num_frames)
                    vm_error(ards::ERR_FRM);
                SpritesABC::drawSizedFX(tx, y, sw, sh, img, mode, frame);
#endif
#else
                shades_draw_sprite(tx, y, img, frame);
                if(ards::vm.needs_render)
                    shades_display();
#endif
            }
            if(int8_t(tx += sw) < 0)
                break;
        }
    }
end:

    seek_to_pc();
}

static void sys_tilemap_get()
{
    uint16_t r = 0;
    auto ptr = vm_pop_begin();
    uint24_t tm = vm_pop<uint24_t>(ptr);
    uint16_t x = vm_pop<uint16_t>(ptr);
    uint16_t y = vm_pop<uint16_t>(ptr);
    FX::disable();

    FX::seekData(tm);
    uint8_t format = FX::readPendingUInt8();
    uint16_t nrow = fx_read_pending_uint16_le();
    uint16_t ncol = fx_read_pending_last_uint16_le();

    if(x < ncol && y < nrow)
    {
        tm += 5;
        uint24_t t = mul_24_16_16(y, ncol) + x;
        if(format == 2) t += t;
        tm += t;
        FX::seekData(tm);
        if(format == 1)
            r = FX::readPendingLastUInt8();
        else
            r = fx_read_pending_last_uint16_le();
    }

    vm_push_unsafe<uint16_t>(ptr, r);
    vm_pop_end(ptr);
    seek_to_pc();
}

#if ABC_SHADES == 2
[[ gnu::noinline ]]
#endif
void draw_char(
    int16_t& x, int16_t y, char c)
{
    /*
        BEFORE
        =================
        &x    r24 r25
        y     r22 r23
        c     r20

        AFTER
        =================
        x     r24 r25
        y     r22 r23
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
            rjmp .+0
        L%=_delay_15:
            nop
        L%=_delay_14:
            nop
        L%=_delay_13:
            nop
        L%=_delay_12:
            nop
        L%=_delay_11:
            rjmp .+0
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
            in   %[m], %[spdr]
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
            in   %[c], %[spdr]
            sei

            rcall L%=_delay_11

            cp  %[c], __zero_reg__
            breq 1f

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

            rcall L%=_delay_12

            in   %[h], %[spdr]

        1:  sbi  %[fxport], %[fxbit]

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
        if(c == 0)
            return;
        SpritesABC::drawBasicFX(
            xv, y, (uint8_t)c, h, addr, ards::vm.text_mode);
}

static void sys_wrap_text()
{
    auto ptr = vm_pop_begin();
#if 1
    uint16_t tn;
    char* p;
    uint8_t w;
    asm volatile(R"(
            ld %B[tn], -%a[ptr]
            ld %A[tn], -%a[ptr]
            ld %B[p], -%a[ptr]
            ld %A[p], -%a[ptr]
            ld %[w], -%a[ptr]
        )"
        : [tn]  "=&r" (tn)
        , [p]   "=&r" (p)
        , [w]   "=&r" (w)
        , [ptr] "+&e" (ptr)
    );
#else
    uint16_t tn = vm_pop<uint16_t>(ptr);
    char* p     = reinterpret_cast<char*>(vm_pop<uint16_t>(ptr));
    uint8_t  w  = vm_pop<uint8_t>(ptr);
#endif
    uint24_t font = ards::vm.text_font;
    FX::disable();
    if(uint8_t(font >> 16) == 0xff)
        vm_error(ards::ERR_FNT);

    char c;
    uint8_t cw = 0; // current width
    char* tp = p;   // pointer after last word break
    uint8_t tw = 0; // width at last word break
    uint16_t ttn = tn; // tn at last word break
    uint16_t num_lines = 1;
    while((c = ld_inc(p)) != '\0' && tn != 0)
    {
        --tn;
        cw += font_get_x_advance(c);
        if(c == '\n')
        {
            cw = 0;
            tw = 0;
            tp = p;
            num_lines += 1;
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
        num_lines += 1;
        cw = 0;
        tw = 0;
        tn = ttn;
    }

    vm_push_unsafe<uint16_t>(ptr, num_lines);
    vm_pop_end(ptr);

    seek_to_pc();
}

static void sys_draw_text()
{
    auto ptr = vm_pop_begin();
#if 1
    int16_t  x;
    int16_t  y;
    uint24_t font;
    uint16_t tn;
    uint16_t tb;
    asm volatile(R"(
        ld  %B[x], -%a[ptr]
        ld  %A[x], -%a[ptr]
        ld  %B[y], -%a[ptr]
        ld  %A[y], -%a[ptr]
        lds %C[font], %[vmfont]+2
        lds %B[font], %[vmfont]+1
        lds %A[font], %[vmfont]+0
        ld  %B[tn], -%a[ptr]
        ld  %A[tn], -%a[ptr]
        ld  %B[tb], -%a[ptr]
        ld  %A[tb], -%a[ptr]
        )"
        : [ptr]    "+&e" (ptr)
        , [x]      "=&r" (x)
        , [y]      "=&r" (y)
        , [font]   "=&r" (font)
        , [tn]     "=&r" (tn)
        , [tb]     "=&r" (tb)
        : [vmfont] ""    (&ards::vm.text_font)
    );
#else
    int16_t  x    = vm_pop<int16_t> (ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    uint24_t font = ards::vm.text_font;
    uint16_t tn   = vm_pop<uint16_t>(ptr);
    uint16_t tb   = vm_pop<uint16_t>(ptr);
#endif
    vm_pop_end(ptr);
    
    FX::disable();
    if(uint8_t(font >> 16) == 0xff)
        vm_error(ards::ERR_FNT);
#if ABC_SHADES == 2
    uint8_t line_height = font_get_line_height();
    int16_t bx = x;
#else
    shades_draw_chars_begin(x, y);
#endif
    
    char const* p = reinterpret_cast<char const*>(tb);
    char c;
    while((c = ld_inc(p)) != '\0' && tn != 0)
    {
        --tn;
#if ABC_SHADES == 2
        if(c == '\n')
        {
            x = bx;
            y += line_height;
            continue;
        }
        draw_char(x, y, c);
#else
        shades_draw_char(c);
#endif
    }
    
#if ABC_SHADES != 2
    shades_draw_chars_end();
#endif
    seek_to_pc();
}

#define DRAW_TEXT_P_BUFSIZE 8

static void sys_draw_text_P()
{
    auto ptr = vm_pop_begin();
#if 1
    int16_t  x;
    int16_t  y;
    uint24_t font;
    uint24_t tn;
    uint24_t tb;
    asm volatile(R"(
        ld  %B[x], -%a[ptr]
        ld  %A[x], -%a[ptr]
        ld  %B[y], -%a[ptr]
        ld  %A[y], -%a[ptr]
        lds %C[font], %[vmfont]+2
        lds %B[font], %[vmfont]+1
        lds %A[font], %[vmfont]+0
        ld  %C[tn], -%a[ptr]
        ld  %B[tn], -%a[ptr]
        ld  %A[tn], -%a[ptr]
        ld  %C[tb], -%a[ptr]
        ld  %B[tb], -%a[ptr]
        ld  %A[tb], -%a[ptr]
        )"
        : [ptr]    "+&e" (ptr)
        , [x]      "=&r" (x)
        , [y]      "=&r" (y)
        , [font]   "=&r" (font)
        , [tn]     "=&r" (tn)
        , [tb]     "=&r" (tb)
        : [vmfont] ""    (&ards::vm.text_font)
    );
#else
    int16_t  x    = vm_pop<int16_t> (ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    uint24_t font = ards::vm.text_font;
    uint24_t tn   = vm_pop<uint24_t>(ptr);
    uint24_t tb   = vm_pop<uint24_t>(ptr);
#endif
    vm_pop_end(ptr);
    
    FX::disable();
    if(uint8_t(font >> 16) == 0xff)
        vm_error(ards::ERR_FNT);
#if ABC_SHADES == 2
    uint8_t line_height = font_get_line_height();
    int16_t bx = x;
#else
    shades_draw_chars_begin(x, y);
#endif

#if DRAW_TEXT_P_BUFSIZE != 0
    char buf[DRAW_TEXT_P_BUFSIZE];
    char* bp = &buf[DRAW_TEXT_P_BUFSIZE];
#endif
    
    char c;
    while(tn != 0)
    {
#if DRAW_TEXT_P_BUFSIZE != 0
        if(bp == &buf[DRAW_TEXT_P_BUFSIZE])
        {
            ards::detail::fx_read_data_bytes(tb, buf, DRAW_TEXT_P_BUFSIZE);
            tb += DRAW_TEXT_P_BUFSIZE;
            bp = buf;
        }
        c = (char)ld_inc(bp);
#else
        c = fx_read_byte_inc(tb);
#endif
        if(c == '\0') break;
        --tn;
        
#if ABC_SHADES == 2
        if(c == '\n')
        {
            x = bx;
            y += line_height;
            continue;
        }
        draw_char(x, y, c);
#else
        shades_draw_char(c);
#endif
    }
    
#if ABC_SHADES != 2
    shades_draw_chars_end();
#endif
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
    
    FX::disable();
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
        
        w += font_get_x_advance(c);
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
    
    FX::disable();
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
    if(n != 0)
    {
        while(*p++ != '\0')
        {
            ++t;
            if(--n == 0) break;
        }
    }
    vm_push(t);
}

static void sys_strlen_P()
{
    auto ptr = vm_pop_begin();
    uint24_t n = vm_pop<uint24_t>(ptr);
    uint24_t b = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    uint24_t t = 0;
    if(n != 0)
    {
        FX::disable();
        fx_seek_data(b);
        while(FX::readPendingUInt8() != '\0')
        {
            ++t;
            if(--n == 0) break;
        }
        (void)FX::readEnd();
    }
    vm_push(t);
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
    uint8_t const* p0 = reinterpret_cast<uint8_t const*>(b0);
    uint8_t const* p1 = reinterpret_cast<uint8_t const*>(b1);
    uint8_t c0, c1;
    for(;;)
    {
        c0 = ld_inc(p0);
        c1 = ld_inc(p1);
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    vm_push_u8(c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
}

static void sys_strcmp_P()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    FX::disable();
    fx_seek_data(b1);
    uint8_t const* p0 = reinterpret_cast<uint8_t const*>(b0);
    uint8_t c0, c1;
    for(;;)
    {
        c0 = ld_inc(p0);
        c1 = FX::readPendingUInt8();
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    vm_push_u8(c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
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
    FX::disable();
    uint8_t c0, c1;
    for(;;)
    {
        c0 = fx_read_byte_inc(b0);
        c1 = fx_read_byte_inc(b1);
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    vm_push_u8(c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
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
    if(n0 != 0)
    {
        for(;;)
        {
            uint8_t c = ld_inc(p1);
            st_inc(p0, c);
            if(c == 0) break;
            if(--n0 == 0) break;
            if(--n1 == 0) { st_inc(p0, 0); break; }
        }
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
    FX::disable();
    fx_seek_data(b1);
    uint16_t nr = n0;
    uint16_t br = b0;
    char* p0 = reinterpret_cast<char*>(b0);
    if(n0 != 0)
    {
        for(;;)
        {
            uint8_t c = FX::readPendingUInt8();
            st_inc(p0, c);
            if(c == 0) break;
            if(--n0 == 0) break;
            if(--n1 == 0) { st_inc(p0, 0); break; }
        }
    }
    (void)FX::readEnd();
    vm_push<uint16_t>(br);
    vm_push<uint16_t>(nr);
    seek_to_pc();
}

#if 1
extern "C" void sys_memset();
#else
static void sys_memset()
{
    auto ptr = vm_pop_begin();
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint8_t val = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    memset(
        reinterpret_cast<void*>(b0),
        val,
        n0);
}
#endif

static void sys_memcpy()
{
    auto ptr = vm_pop_begin();
#if 1
    uint16_t n0;
    uint16_t b0;
    uint16_t n1;
    uint16_t b1;
    asm volatile(R"(
            ld %B[n0], -%a[p]
            ld %A[n0], -%a[p]
            ld %B[b0], -%a[p]
            ld %A[b0], -%a[p]
            ld %B[n1], -%a[p]
            ld %A[n1], -%a[p]
            ld %B[b1], -%a[p]
            ld %A[b1], -%a[p]
        )"
        : [p]  "+&e" (ptr)
        , [n0] "=&r" (n0)
        , [n1] "=&r" (n1)
        , [b0] "=&r" (b0)
        , [b1] "=&r" (b1)
    );
#else
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint16_t n1 = vm_pop<uint16_t>(ptr);
    uint16_t b1 = vm_pop<uint16_t>(ptr);
#endif
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
#if 1
    uint16_t n0;
    uint16_t b0;
    uint24_t b1;
    asm volatile(R"(
            ld   %B[n0], -%a[p]
            ld   %A[n0], -%a[p]
            ld   %B[b0], -%a[p]
            ld   %A[b0], -%a[p]

            ; load and compare n1 against n0
            ld   r0, -%a[p]
            cp   r0, __zero_reg__
            ld   r0, -%a[p]
            cpc  r0, %B[n0]
            ld   r0, -%a[p]
            cpc  r0, %A[n0]
            
            ld   %C[b1], -%a[p]
            ld   %B[b1], -%a[p]
            ld   %A[b1], -%a[p]

            ; if n1 != n0, error
            breq 1f
            ldi  r24, %[ERR_CPY]
            jmp  %x[vm_error]
        1:
        )"
        : [p]        "+&e" (ptr)
        , [n0]       "=&r" (n0)
        , [b0]       "=&r" (b0)
        , [b1]       "=&r" (b1)
        : [ERR_CPY]  "I"   (ards::ERR_CPY)
        , [vm_error] ""    (vm_error)
    );
#else
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
#endif
    vm_pop_end(ptr);
    FX::disable();
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

//struct u32_div_t { uint32_t q, r; };
//extern "C" u32_div_t __udivmodsi4(uint32_t n, uint32_t d);

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
    if(base == 10)
    {
        while(x != 0)
        {
#if 0
            u32_div_t d = __udivmodsi4(x, 10);
            uint8_t r = d.r;
            x = d.q;
#else
            uint8_t r = x % 10;
            x /= 10;
#endif
            st_inc(tp, char(r + '0'));
            --w;
        }
    }
    else
    {
        while(x != 0)
        {
            uint8_t r = (uint8_t)x & 0xf;
            x >>= 4;
            st_inc(tp, char(r + (r < 10 ? '0' : 'a' - 10)));
            --w;
        }
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

//[[gnu::noinline]]
static void format_add_float(format_char_func f, float x, uint8_t prec)
{
    //if(isnan(x))
    //{
    //    f('n');
    //    f('a');
    //    f('n');
    //    return;
    //}
    //if(isinf(x))
    //{
    //    f('i');
    //    f('n');
    //    f('f');
    //    return;
    //}
    if(fabs(x) > 4294967040.f)
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
#if 1
        static float const PRECS[10] PROGMEM =
        {
            5e-1f, 5e-2f, 5e-3f, 5e-4f, 5e-5f, 5e-6f, 5e-7f, 5e-8f, 5e-9f, 5e-10f,
        };
        float r;
        float const* p = &PRECS[prec];
        asm volatile(R"(
            lpm %A[r], %a[p]+
            lpm %B[r], %a[p]+
            lpm %C[r], %a[p]+
            lpm %D[r], %a[p]+
            )"
            : [r] "=&r" (r)
            , [p] "+&z" (p)
        );
#else
        float r = 0.5f;
        for(uint8_t i = 0; i < prec; ++i)
            r *= 0.1f;
#endif
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
        r -= t;
        f((char)('0' + t));
    }
}

#define FORMAT_EXEC_BUFFER_SIZE 0

#if FORMAT_EXEC_BUFFER_SIZE != 0
[[ gnu::always_inline ]]
static char format_exec_read_char(char* buf, char*& fbp, uint24_t& fb)
{
    if(fbp == &buf[FORMAT_EXEC_BUFFER_SIZE])
    {
        ards::detail::fx_read_data_bytes(fb, buf, FORMAT_EXEC_BUFFER_SIZE);
        fb += FORMAT_EXEC_BUFFER_SIZE;
        fbp = buf;
    }
    return (char)ld_inc(fbp);
}
#define FORMAT_EXEC_NEXT_CHAR format_exec_read_char(format_buf, fbp, fb)
#else
#define FORMAT_EXEC_NEXT_CHAR (char)fx_read_byte_inc(fb)
#endif

[[gnu::flatten]]
static void format_exec(format_char_func f)
{
    uint24_t fn;
    uint24_t fb;
    {
        auto ptr = vm_pop_begin();
#if 1
        asm volatile(R"(
                ld %C[fn], -%a[ptr]
                ld %B[fn], -%a[ptr]
                ld %A[fn], -%a[ptr]
                ld %C[fb], -%a[ptr]
                ld %B[fb], -%a[ptr]
                ld %A[fb], -%a[ptr]
            )"
            : [fn]  "=&r" (fn)
            , [fb]  "=&r" (fb)
            , [ptr] "+&e" (ptr)
        );
#else
        fn = vm_pop<uint24_t>(ptr);
        fb = vm_pop<uint24_t>(ptr);
#endif
        vm_pop_end(ptr);
    }

#if FORMAT_EXEC_BUFFER_SIZE != 0
    char format_buf[FORMAT_EXEC_BUFFER_SIZE];
    char* fbp = &format_buf[FORMAT_EXEC_BUFFER_SIZE];
#endif
  
    while(fn != 0)
    {
#if ABC_SHADES != 2
        if(ards::vm.needs_render)
            shades_display();
#endif
        char c = FORMAT_EXEC_NEXT_CHAR;
        --fn;
        if(c != '%')
        {
            f(c);
            continue;
        }
        c = FORMAT_EXEC_NEXT_CHAR;
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
#if 1
                asm volatile(R"(
                        ld %B[tn], -%a[ptr]
                        ld %A[tn], -%a[ptr]
                        ld %B[tb], -%a[ptr]
                        ld %A[tb], -%a[ptr]
                    )"
                    : [tn]  "=&r" (tn)
                    , [tb]  "=&r" (tb)
                    , [ptr] "+&e" (ptr)
                );
#else
                tn = vm_pop<uint16_t>(ptr);
                tb = vm_pop<uint16_t>(ptr);
#endif
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
#if 1
                asm volatile(R"(
                        ld %C[tn], -%a[ptr]
                        ld %B[tn], -%a[ptr]
                        ld %A[tn], -%a[ptr]
                        ld %C[tb], -%a[ptr]
                        ld %B[tb], -%a[ptr]
                        ld %A[tb], -%a[ptr]
                    )"
                    : [tn]  "=&r" (tn)
                    , [tb]  "=&r" (tb)
                    , [ptr] "+&e" (ptr)
                );
#else
                tn = vm_pop<uint24_t>(ptr);
                tb = vm_pop<uint24_t>(ptr);
#endif
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
            int8_t w = (int8_t)(FORMAT_EXEC_NEXT_CHAR - '0');
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
            uint8_t prec = FORMAT_EXEC_NEXT_CHAR - '0';
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

[[gnu::naked]]
static void format_exec_to_buffer(char c)
{
    asm volatile(R"(
        lds  r30, %[format_user]+0
        lds  r31, %[format_user]+1
        ldd  r26, Z+2
        ldd  r27, Z+3
        cp   r26, __zero_reg__
        cpc  r27, __zero_reg__
        breq 1f
        sbiw r26, 1
        std  Z+2, r26
        std  Z+3, r27
        ld   r26, Z
        ldd  r27, Z+1
        st   X+, r24
        st   Z, r26
        std  Z+1, r27
    1:  ret
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

#if ABC_SHADES == 2
struct format_user_draw
{
    int16_t bx;
    int16_t x;
    int16_t y;
    uint8_t line_height;
};
#endif

#if ABC_SHADES == 2
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
#endif

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
    FX::disable();
    
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
    FX::disable();
    
    format_exec(format_exec_debug_printf);
    
    seek_to_pc();
}

static void sys_draw_textf()
{
#if ABC_SHADES == 2
    format_user_draw user;
    format_user = &user;
    
    {
        auto ptr = vm_pop_begin();
        user.x = user.bx = vm_pop<int16_t> (ptr);
        user.y = vm_pop<int16_t> (ptr);
        vm_pop_end(ptr);
    }
#else
    {
        auto ptr = vm_pop_begin();
        int16_t x = vm_pop<int16_t> (ptr);
        int16_t y = vm_pop<int16_t> (ptr);
        vm_pop_end(ptr);
        shades_draw_chars_begin(x, y);
    }
#endif
    FX::disable();

    {
        uint8_t t = uint8_t(ards::vm.text_font >> 16);
        if(t == 0xff)
            vm_error(ards::ERR_FNT);
    }
#if ABC_SHADES == 2
    user.line_height = font_get_line_height();
#endif
    
#if ABC_SHADES == 2
    format_exec(format_exec_draw);
#else
    format_exec(shades_draw_char);
#endif

#if ABC_SHADES != 2
    shades_draw_chars_end();
#endif
    
    seek_to_pc();
}

static void tones_play_helper(void(*f)(uint24_t))
{
    auto ptr = vm_pop_begin();
    uint24_t song = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    
    if(!Arduboy2Audio::enabled())
        return;
    
    FX::disable();
    
    f(song);
    
    seek_to_pc();
}

static void sys_music_play()
{
    tones_play_helper(ards::Tones::music_play);
}

static void sys_tones_play()
{
    tones_play_helper(ards::Tones::tones_play);
}

static void sys_tones_play_primary()
{
    tones_play_helper(ards::Tones::tones_play_primary);
}

static void sys_tones_play_auto()
{
    tones_play_helper(ards::Tones::tones_play_auto);
}

static void sys_music_playing()
{
    vm_push_u8(ards::Tones::music_playing());
}

static void sys_tones_playing()
{
    vm_push_u8(ards::Tones::tones_playing());
}

static void sys_music_stop()
{
    ards::Tones::music_stop();
}

static void sys_tones_stop()
{
    ards::Tones::tones_stop();
}

#if 1
extern "C" void sys_audio_enabled(); // I_P1
#else
static void sys_audio_enabled()
{
    vm_push_u8(Arduboy2Audio::enabled());
}
#endif

static void sys_audio_toggle()
{
    Arduboy2Audio::toggle();
    ards::Tones::setup();
}

static void sys_audio_stop()
{
    ards::Tones::stop();
}

static void sys_audio_playing()
{
    vm_push_u8(ards::Tones::playing());
}

constexpr size_t MAX_SAVE_SIZE = (ABC_SHADES == 2 ? 1024 : 256);

static void sys_save_exists()
{
    uint16_t save_size;
    FX::disable();
    fx_seek_data(10);
    union
    {
        uint16_t save_size;
        uint8_t b[2];
    } u;
    u.b[0] = FX::readPendingUInt8();
    u.b[1] = FX::readPendingLastUInt8();
    bool r = false;
    if(u.save_size > 0 && u.save_size <= MAX_SAVE_SIZE)
    {
        uint16_t t;
        FX::seekSave(0);
        r = (FX::readPendingLastUInt16() == u.save_size);
    }
    vm_push_u8(r);
    seek_to_pc();
}

static void sys_save()
{
    uint16_t save_size;
    FX::disable();
    FX::seekData(10);
    union
    {
        uint16_t save_size;
        uint8_t b[2];
    } u;
    u.b[0] = FX::readPendingUInt8();
    u.b[1] = FX::readPendingLastUInt8();
    if(u.save_size > 0 && u.save_size <= MAX_SAVE_SIZE)
        FX::saveGameState(&ards::vm.globals[0], u.save_size);
    FX::waitWhileBusy();
    seek_to_pc();
}

static void sys_load()
{
    FX::disable();
    fx_seek_data(10);
    union
    {
        uint16_t save_size;
        uint8_t b[2];
    } u;
    u.b[0] = FX::readPendingUInt8();
    u.b[1] = FX::readPendingLastUInt8();
    bool r = false;
    if(u.save_size > 0 && u.save_size <= MAX_SAVE_SIZE)
        r = (bool)FX::loadGameState(&ards::vm.globals[0], u.save_size);
    vm_push_u8(r);
    seek_to_pc();
}

#if 1
extern "C" void sys_sin();
#else
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
#endif

#if 1
extern "C" void sys_cos();
#else
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
#endif

#if 1
extern "C" void sys_tan(); // I_P1
#else
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
#endif

#if 1
extern "C" void sys_atan2();
#else
[[gnu::naked]]
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
#endif

#if 1
extern "C" void sys_floor();
#else
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
#endif

#if 1
extern "C" void sys_ceil();
#else
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
#endif

#if 1
extern "C" void sys_round();
#else
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
#endif

#if 1
extern "C" void sys_mod();
#else
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
#endif

#if 1
extern "C" void sys_pow();
#else
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
#endif

#if 1
extern "C" void sys_sqrt();
#else
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
#endif

static void sys_generate_random_seed()
{
    vm_push<uint32_t>(Arduboy2Base::generateRandomSeed());
}

static void sys_init_random_seed()
{
    uint32_t seed = Arduboy2Base::generateRandomSeed();
    asm volatile(R"(
            sts %[P]+0, %A[s]
            sts %[P]+1, %B[s]
            sts %[P]+2, %C[s]
            sts %[P]+3, %D[s]
        )"
        :
        : [s] "r" (seed)
        , [P] ""  (&abc_seed[0])
    );
}

static void sys_set_random_seed()
{
    auto ptr = vm_pop_begin();
    uint32_t seed;
    asm volatile(R"(
            ld  %D[s], -%a[ptr]
            ld  %C[s], -%a[ptr]
            ld  %B[s], -%a[ptr]
            ld  %A[s], -%a[ptr]
            sts %[P]+0, %A[s]
            sts %[P]+1, %B[s]
            sts %[P]+2, %C[s]
            sts %[P]+3, %D[s]
        )"
        : [ptr] "+&e" (ptr)
        , [s]   "=&r" (seed)
        : [P]   ""    (&abc_seed[0])
    );
    vm_pop_end(ptr);
}

static uint32_t abc_random()
{
    uint32_t r;
    uint16_t t;
    
    // xorshift-32: (a, b, c) = (8, 9, 23)

    //           D        C        B        A
    // r      abcdefgh ijklmnop qrstuvwx 01234567
    // r<<8   ijklmnop qrstuvwx 01234567 ........
    // r>>9   ........ .abcdefg hijklmno pqrstuvw
    // r<<23  x0123456 7....... ........ ........

    asm volatile(R"(
            lds  %A[r], %[P]+0
            lds  %B[r], %[P]+1
            lds  %C[r], %[P]+2
            lds  %D[r], %[P]+3

            ; r ^= r << 8
            eor  %D[r], %C[r]
            eor  %C[r], %B[r]
            eor  %B[r], %A[r]

            ; r ^= r >> 9
            movw %A[t], %C[r]
            lsr  %B[t]
            ror  %A[t]
            eor  %C[r], %B[t]
            mov  %B[t], %B[r]
            eor  %B[r], %A[t]
            ror  %B[t]
            eor  %A[r], %B[t]

            ; r ^= r << 23
            movw %A[t], %A[r]
            lsr  %B[t]
            ror  %A[t]
            eor  %D[r], %A[t]
            clr  %A[t]
            ror  %A[t]
            eor  %C[r], %A[t]

            sts  %[P]+0, %A[r]
            sts  %[P]+1, %B[r]
            sts  %[P]+2, %C[r]
            sts  %[P]+3, %D[r]
        )"
        : [r] "=&r" (r)
        , [t] "=&r" (t)
        : [P] ""    (&abc_seed[0])
    );
    return r;
}

static void sys_random()
{
    vm_push<uint32_t>(abc_random());
}

static void sys_random_range()
{
    auto ptr = vm_pop_begin();
#if 1
    uint32_t a, b;
    asm volatile(R"(
            ld  %D[a], -%a[ptr]
            ld  %C[a], -%a[ptr]
            ld  %B[a], -%a[ptr]
            ld  %A[a], -%a[ptr]
            ld  %D[b], -%a[ptr]
            ld  %C[b], -%a[ptr]
            ld  %B[b], -%a[ptr]
            ld  %A[b], -%a[ptr]
        )"
        : [ptr] "+&e" (ptr)
        , [a]   "=&r" (a)
        , [b]   "=&r" (b)
    );
#else
    uint32_t a = vm_pop<uint32_t>(ptr);
    uint32_t b = vm_pop<uint32_t>(ptr);
#endif
    uint32_t t = a;
    if(a < b)
        t += abc_random() % (b - a);
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

#if ABC_SHADES == 2
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
#else
static void sys_set_text_color()
{
    auto ptr = vm_pop_begin();
    ards::vm.text_mode = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
}
#endif

sys_func_t const SYS_FUNCS[] PROGMEM =
{

#if ABC_SHADES == 2
    sys_display,
    sys_display_noclear,
#else
    shades_swap,
    shades_swap,
#endif

    sys_get_pixel,

#if ABC_SHADES == 2
    sys_draw_pixel,
    sys_draw_hline,
    sys_draw_vline,
    sys_draw_line,
#else
    sys_pop_n<5>,
    sys_pop_n<6>,
    sys_pop_n<6>,
    sys_pop_n<9>,
#endif

    sys_draw_rect,
    sys_draw_filled_rect,

#if ABC_SHADES == 2
    sys_draw_circle,
    sys_draw_filled_circle,
#else
    sys_pop_n<6>,
    sys_pop_n<6>,
#endif

    sys_draw_sprite,

#if ABC_SHADES == 2
    sys_draw_sprite_selfmask,
#else
    sys_draw_sprite,
#endif

    sys_draw_tilemap,
    sys_draw_text,
    sys_draw_text_P,
    sys_draw_textf,
    sys_text_width,
    sys_text_width_P,
    sys_wrap_text,
    sys_set_text_font,
    sys_set_text_color,
    sys_set_frame_rate,
    sys_idle,
    sys_debug_break,
    sys_debug_printf,
    sys_assert,
    sys_buttons,
    sys_just_pressed,
    sys_just_released,
    sys_pressed,
    sys_any_pressed,
    sys_not_pressed,
    sys_millis,
    sys_memset,
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
    
    sys_music_play,
    sys_music_playing,
    sys_music_stop,

    sys_tones_play,
    sys_tones_play_primary,
    sys_tones_play_auto,
    sys_tones_playing,
    sys_tones_stop,

    sys_audio_enabled,
    sys_audio_toggle,
    sys_audio_playing,
    ards::Tones::stop,

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
    sys_set_random_seed,
    sys_random,
    sys_random_range,
    sys_tilemap_get,

};
