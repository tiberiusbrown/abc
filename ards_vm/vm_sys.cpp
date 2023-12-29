#ifndef EEPROM_h
#define EEPROM_h
#endif

#include <Arduboy2.h>
#include <Arduboy2Audio.h>
#include <ArduboyFX.h>

#define SPRITESU_IMPLEMENTATION
#define SPRITESU_FX
#define SPRITESU_RECT
#include "SpritesU.hpp"

#include "ards_tone.hpp"

#include "ards_vm.hpp"

#include <math.h>

using sys_func_t = void(*)();
extern sys_func_t const SYS_FUNCS[] PROGMEM;
extern "C" void vm_error(ards::error_t e);

__attribute__((noinline)) static void check_overlap(uint16_t b0, uint16_t n0, uint16_t b1, uint16_t n1)
{
    if(b0 + n0 <= b1) return;
    if(b1 + n1 <= b0) return;
    vm_error(ards::ERR_STR);
}

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
        "lds  %A[r], 0x0660\n"
        "ldi  %B[r], 1\n"
        : [r] "=&d" (r));
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

template<class T>
inline void vm_push(uint8_t*& ptr, T x)
{
    if((uint8_t)(uintptr_t)ptr + sizeof(T) >= 256)
        vm_error(ards::ERR_DST);
    union
    {
        T r;
        uint8_t b[sizeof(T)];
    } u;
    u.r = x;
    for(size_t i = 0; i < sizeof(T); ++i)
    {
        asm volatile(
            "st %a[ptr]+, %[t]\n"
            : [ptr] "+&e" (ptr)
            : [t] "r" (u.b[i])
            );
    }
    ards::vm.sp = (uint8_t)(uintptr_t)ptr;
}

template<class T>
inline void vm_push(T x)
{
    uint8_t* ptr;
    asm volatile(
        "lds  %A[ptr], 0x0660\n"
        "ldi  %B[ptr], 1\n"
        : [ptr] "=&e" (ptr));
    vm_push<T>(ptr, x);
    ards::vm.sp = (uint8_t)(uintptr_t)ptr;
}

static void sys_display()
{
    FX::disable();
    FX::display(true);
    FX::seekData(ards::vm.pc);
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

static void sys_draw_hline()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesU::fillRect(x, y, w, 1, color);
}

static void sys_draw_vline()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesU::fillRect(x, y, 1, h, color);
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
    SpritesU::fillRect(x, y, w, 1, color);
    SpritesU::fillRect(x, y + h - 1, w, 1, color);
    SpritesU::fillRect(x, y, 1, h, color);
    SpritesU::fillRect(x + w - 1, y, 1, h, color);
}

static void sys_draw_filled_rect()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    SpritesU::fillRect(x, y, w, h, color);
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

static void sys_draw_filled_circle()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint8_t r = vm_pop<uint8_t>(ptr);
    uint8_t color = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::fillCircle(x, y, r, color);
}

static void sys_set_frame_rate()
{
    auto ptr = vm_pop_begin();
    uint8_t fr = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    ards::vm.frame_dur = 1000 / fr;
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
    ards::Tones::update();
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

static void sys_draw_sprite()
{
    auto ptr = vm_pop_begin();
    int16_t x = vm_pop<int16_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    uint24_t image = vm_pop<uint24_t>(ptr);
    uint16_t frame = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    FX::disable();
    FX::seekData(image);
    uint8_t w = FX::readPendingUInt8();
    uint8_t h = FX::readPendingUInt8();
    uint16_t num = FX::readPendingUInt8();
    num += ((uint16_t)FX::readPendingUInt8() << 8);
    uint8_t masked = FX::readPendingLastUInt8();
    if(frame >= num)
        vm_error(ards::ERR_FRM);
    SpritesU::drawBasic(
        x, y, w, h, image + 5, frame,
        masked ? SpritesU::MODE_PLUSMASKFX : SpritesU::MODE_OVERWRITEFX);
    FX::seekData(ards::vm.pc);
}

static void draw_char(uint24_t font, int16_t x, int16_t y, uint8_t w, uint8_t h, char c)
{
    SpritesU::drawBasic(
        x, y, w, h, font + 513 + 5, uint8_t(c),
        SpritesU::MODE_SELFMASKFX);
        //SpritesU::MODE_OVERWRITEFX);
}

static void sys_draw_text()
{
    auto ptr = vm_pop_begin();
    int16_t  x    = vm_pop<int16_t> (ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    uint24_t font = vm_pop<uint24_t>(ptr);
    uint16_t tn   = vm_pop<uint16_t>(ptr);
    uint16_t tb   = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    
    FX::disable();
    uint8_t w, h, line_height;
    FX::seekData(font + 512);
    line_height = FX::readPendingUInt8();
    w = FX::readPendingUInt8();
    h = FX::readPendingLastUInt8();
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
        
        FX::seekData(font + uint8_t(c) * 2);
        int8_t lsb = (int8_t)FX::readPendingUInt8();
        uint8_t adv = FX::readPendingLastUInt8();
        draw_char(font, x + lsb, y, w, h, c);
        x += adv;
    }
    
    FX::seekData(ards::vm.pc);
}

static void sys_draw_text_P()
{
    auto ptr = vm_pop_begin();
    int16_t  x    = vm_pop<int16_t> (ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    uint24_t font = vm_pop<uint24_t>(ptr);
    uint24_t tn   = vm_pop<uint24_t>(ptr);
    uint24_t tb   = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    
    FX::disable();
    uint8_t w, h, line_height;
    FX::seekData(font + 512);
    line_height = FX::readPendingUInt8();
    w = FX::readPendingUInt8();
    h = FX::readPendingLastUInt8();
    int16_t bx = x;
    
    char c;
    while(tn != 0)
    {
        FX::seekData(tb++);
        c = FX::readPendingLastUInt8();
        if(c == '\0') break;
        --tn;
        
        if(c == '\n')
        {
            x = bx;
            y += line_height;
            continue;
        }
        
        FX::seekData(font + uint8_t(c) * 2);
        int8_t lsb = (int8_t)FX::readPendingUInt8();
        uint8_t adv = FX::readPendingLastUInt8();
        draw_char(font, x + lsb, y, w, h, c);
        x += adv;
    }
    
    FX::seekData(ards::vm.pc);
}

static void sys_text_width()
{
    auto ptr = vm_pop_begin();
    uint24_t font = vm_pop<uint24_t>(ptr) + 1;
    uint16_t tn   = vm_pop<uint16_t>(ptr);
    uint16_t tb   = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    
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
        
        FX::seekData(font + uint8_t(c) * 2);
        uint8_t adv = FX::readPendingLastUInt8();
        w += adv;
    }
    if(w > wmax) wmax = w;
    
    vm_push<uint16_t>(wmax);
    
    FX::seekData(ards::vm.pc);
}

static void sys_text_width_P()
{
    auto ptr = vm_pop_begin();
    uint24_t font = vm_pop<uint24_t>(ptr) + 1;
    uint24_t tn   = vm_pop<uint24_t>(ptr);
    uint24_t tb   = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    
    FX::disable();
    char const* p = reinterpret_cast<char const*>(tb);
    char c;
    uint16_t wmax = 0;
    uint16_t w = 0;
    while(tn != 0)
    {
        FX::seekData(tb++);
        c = FX::readPendingLastUInt8();
        if(c == '\0') break;
        --tn;
        
        if(c == '\n')
        {
            if(w > wmax) wmax = w;
            w = 0;
            continue;
        }
        
        FX::seekData(font + uint8_t(c) * 2);
        uint8_t adv = FX::readPendingLastUInt8();
        w += adv;
    }
    if(w > wmax) wmax = w;
    
    vm_push<uint16_t>(wmax);
    
    FX::seekData(ards::vm.pc);
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
    FX::disable();
    FX::seekData(b);
    uint24_t t = 0;
    while(FX::readPendingUInt8() != '\0')
    {
        ++t;
        if(--n == 0) break;
    }
    vm_push(t);
    FX::disable();
    FX::seekData(ards::vm.pc);
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
    FX::disable();
    FX::seekData(b1);
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
    FX::disable();
    FX::seekData(ards::vm.pc);
}

//static void sys_strcpy()
//{
//    auto ptr = vm_pop_begin();
//    uint16_t n0 = vm_pop<uint16_t>(ptr);
//    uint16_t b0 = vm_pop<uint16_t>(ptr);
//    uint16_t n1 = vm_pop<uint16_t>(ptr);
//    uint16_t b1 = vm_pop<uint16_t>(ptr);
//    vm_pop_end(ptr);
//    char* p0 = reinterpret_cast<char*>(b0);
//    char const* p1 = reinterpret_cast<char const*>(b1);
//    for(;;)
//    {
//        uint8_t c = ld_inc(p1);
//        st_inc(p0, c);
//        if(c == 0) break;
//        if(--n0 == 0) break;
//        if(--n1 == 0) { st_inc(p0, 0); break; }
//    }
//}
//
//static void sys_strcpy_P()
//{
//    auto ptr = vm_pop_begin();
//    uint16_t n0 = vm_pop<uint16_t>(ptr);
//    uint16_t b0 = vm_pop<uint16_t>(ptr);
//    uint24_t n1 = vm_pop<uint24_t>(ptr);
//    uint24_t b1 = vm_pop<uint24_t>(ptr);
//    vm_pop_end(ptr);
//    FX::disable();
//    FX::seekData(b1);
//    char* p0 = reinterpret_cast<char*>(b0);
//    for(;;)
//    {
//        uint8_t c = FX::readPendingUInt8();
//        st_inc(p0, c);
//        if(c == 0) break;
//        if(--n0 == 0) break;
//        if(--n1 == 0) { st_inc(p0, 0); break; }
//    }
//    FX::disable();
//    FX::seekData(ards::vm.pc);
//}

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
        FX::seekData(tb);
        uint8_t c = FX::readPendingLastUInt8();
        if(c == '\0') return;
        f(c);
        ++tb;
        --tn;
    }
}

static void format_add_int(format_char_func f, uint32_t x, bool sign, uint8_t base)
{
    char buf[10];
    char* tp = buf;
    
    if(sign && (int32_t)x < 0)
    {
        f('-');
        x = -x;
    }

    if(x == 0)
        st_inc(tp, '0');
    while(x != 0)
    {
        uint8_t r = x % base;
        x /= base;
        st_inc(tp, char(r + (r < 10 ? '0' : 'a' - 10)));
    }
    
    // reverse
    reverse_str(buf, tp);
    
    char* bb = buf;
    while(bb < tp)
    {
        uint8_t c = ld_inc(bb);
        f(c);
    }
}

// TODO: store this stuff in a struct on stack
static void* format_user;
static uint16_t format_db;
static uint16_t format_dn;

static void format_add_float(format_char_func f, float x, uint8_t frac)
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
        for(uint8_t i = 0; i < frac; ++i)
            r *= 0.1f;
        x += r;
    }
    
    uint32_t n = (uint32_t)x;
    float r = x - (double)n;
    format_add_int(f, n, false, 10);
    
    if(frac > 0)
        f('.');
    while(frac-- > 0)
    {
        r *= 10.f;
        uint8_t t = (uint8_t)r;
        f((char)('0' + t));
        r -= t;
    }
}

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
        FX::seekData(fb++);
        char c = (char)FX::readPendingLastUInt8();
        --fn;
        if(c != '%')
        {
            f(c);
            continue;
        }
        FX::seekData(fb++);
        c = (char)FX::readPendingLastUInt8();
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
            check_overlap(tb, tn, format_db, format_dn);
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
            format_add_int(f, x, c == 'd', c == 'x' ? 16 : 10);
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
            format_add_float(f, x, 4);
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

static void format_exec_to_buffer(char c)
{
    format_user_buffer* u = (format_user_buffer*)format_user;
    uint16_t n = u->n;
    if(n == 0) return;
    --n;
    u->n = n;
    st_inc(u->p, (uint8_t)c);
}

struct format_user_draw
{
    uint24_t font;
    int16_t bx;
    int16_t x;
    int16_t y;
    uint8_t w;
    uint8_t h;
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
        FX::seekData(u->font + uint8_t(c) * 2);
        int8_t lsb = (int8_t)FX::readPendingUInt8();
        uint8_t adv = FX::readPendingLastUInt8();
        draw_char(u->font, u->x + lsb, u->y, u->w, u->h, c);
        u->x += adv;
    }
}

static void sys_format()
{
    auto ptr = vm_pop_begin();
    uint16_t dn = vm_pop<uint16_t>(ptr);
    uint16_t db = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    FX::disable();
    
    format_db = db;
    format_dn = dn;
    format_user_buffer user;
    user.p = reinterpret_cast<char*>(db);
    user.n = dn;
    format_user = &user;
    format_exec(format_exec_to_buffer);
    
    if(user.n != 0)
        *user.p = '\0';
    
    FX::seekData(ards::vm.pc);
}

static void sys_draw_textf()
{
    format_user_draw user;
    format_user = &user;
    format_db = 0;
    format_dn = 0;
    
    {
        auto ptr = vm_pop_begin();
        user.x = user.bx = vm_pop<int16_t> (ptr);
        user.y = vm_pop<int16_t> (ptr);
        user.font = vm_pop<uint24_t>(ptr);
        vm_pop_end(ptr);
        FX::disable();
    }
    
    FX::seekData(user.font + 512);
    user.line_height = FX::readPendingUInt8();
    user.w = FX::readPendingUInt8();
    user.h = FX::readPendingLastUInt8();
    
    format_exec(format_exec_draw);
    
    FX::seekData(ards::vm.pc);
}

static void sys_tones_play()
{
    auto ptr = vm_pop_begin();
    uint24_t song = vm_pop<uint24_t>(ptr);
    vm_pop_end(ptr);
    
    if(!Arduboy2Audio::enabled())
        return;
    
    FX::disable();
    
    ards::Tones::play(song);
    
    FX::seekData(ards::vm.pc);
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
    FX::disable();
    FX::seekData(10);
    save_size = FX::readPendingUInt8();
    save_size |= ((uint16_t)FX::readPendingLastUInt8() << 8);
    //FX::readDataBytes(10, (uint8_t*)&save_size, 2);
    bool r = false;
    if(save_size > 0 && save_size <= 1024)
    {
        uint16_t t;
        FX::seekSave(0);
        r = (FX::readPendingLastUInt16() == save_size);
    }
    FX::seekData(ards::vm.pc);
    vm_push<bool>(r);
}

static void sys_save()
{
    uint16_t save_size;
    FX::disable();
    FX::seekData(10);
    save_size = FX::readPendingUInt8();
    save_size |= ((uint16_t)FX::readPendingLastUInt8() << 8);
    //FX::readDataBytes(10, (uint8_t*)&save_size, 2);
    if(save_size > 0 && save_size <= 1024)
        FX::saveGameState(&ards::vm.globals[0], save_size);
    FX::seekData(ards::vm.pc);
}

static void sys_load()
{
    uint16_t save_size;
    FX::disable();
    FX::readDataBytes(10, (uint8_t*)&save_size, 2);
    bool r = false;
    if(save_size > 0 && save_size <= 1024)
        r = (bool)FX::loadGameState(&ards::vm.globals[0], save_size);
    FX::seekData(ards::vm.pc);
    vm_push<bool>(r);
}

static void sys_sin()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, sinf(x));
    vm_pop_end(ptr);
}

static void sys_cos()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, cosf(x));
    vm_pop_end(ptr);
}

static void sys_tan()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, tanf(x));
    vm_pop_end(ptr);
}

static void sys_atan2()
{
    auto ptr = vm_pop_begin();
    float y = vm_pop<float>(ptr);
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, atan2(y, x));
    vm_pop_end(ptr);
}

static void sys_floor()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, floorf(x));
    vm_pop_end(ptr);
}

static void sys_ceil()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, ceilf(x));
    vm_pop_end(ptr);
}

static void sys_round()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, roundf(x));
    vm_pop_end(ptr);
}

static void sys_mod()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    float y = vm_pop<float>(ptr);
    vm_push<float>(ptr, fmodf(x, y));
    vm_pop_end(ptr);
}

static void sys_pow()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    float y = vm_pop<float>(ptr);
    vm_push<float>(ptr, powf(x, y));
    vm_pop_end(ptr);
}

static void sys_sqrt()
{
    auto ptr = vm_pop_begin();
    float x = vm_pop<float>(ptr);
    vm_push<float>(ptr, sqrtf(x));
    vm_pop_end(ptr);
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
    auto ptr = vm_pop_begin();
    uint32_t a = vm_pop<uint32_t>(ptr);
    uint32_t b = vm_pop<uint32_t>(ptr);
    uint32_t t;
    if(a >= b)
        t = a;
    else
        t = random() % (b - a) + a;
    vm_push<uint32_t>(ptr, t);
    vm_pop_end(ptr);
}

sys_func_t const SYS_FUNCS[] PROGMEM =
{
    sys_display,
    sys_draw_pixel,
    sys_draw_hline,
    sys_draw_vline,
    sys_draw_line,
    sys_draw_rect,
    sys_draw_filled_rect,
    sys_draw_circle,
    sys_draw_filled_circle,
    sys_draw_sprite,
    sys_draw_text,
    sys_draw_text_P,
    sys_draw_textf,
    sys_text_width,
    sys_text_width_P,
    sys_set_frame_rate,
    sys_next_frame,
    sys_idle,
    sys_debug_break,
    sys_assert,
    sys_poll_buttons,
    sys_just_pressed,
    sys_just_released,
    sys_pressed,
    sys_any_pressed,
    sys_not_pressed,
    sys_millis,
    sys_strlen,
    sys_strlen_P,
    sys_strcmp,
    sys_strcmp_P,
    //sys_strcpy,
    //sys_strcpy_P,
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
};
