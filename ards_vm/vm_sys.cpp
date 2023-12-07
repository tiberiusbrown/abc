#include <Arduboy2.h>
#include <ArduboyFX.h>

#define SPRITESU_IMPLEMENTATION
#define SPRITESU_FX
#define SPRITESU_RECT
#include "SpritesU.hpp"

#include "ards_vm.hpp"

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
        "lds  %A[r], 0x0660\n"
        "ldi  %B[r], 1\n"
        : [r] "=&d" (r));
    return r;
}

inline void vm_pop_end(uint8_t* ptr)
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
inline void vm_push(T x)
{
    if(ards::vm.sp + sizeof(T) >= 256)
        vm_error(ards::ERR_DST);
    union
    {
        T r;
        uint8_t b[sizeof(T)];
    } u;
    u.r = x;
    uint8_t* ptr;
    asm volatile(
        "lds  %A[ptr], 0x0660\n"
        "ldi  %B[ptr], 1\n"
        : [ptr] "=&e" (ptr));
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

static void sys_display()
{
    (void)FX::readEnd();
    FX::display(true);
    FX::seekData(ards::vm.pc);
}

static void sys_draw_pixel()
{
    auto ptr = vm_pop_begin();
    uint8_t color = vm_pop<uint8_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    int16_t x = vm_pop<int16_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::drawPixel(x, y, color);
}

static void sys_draw_filled_rect()
{
    auto ptr = vm_pop_begin();
    uint8_t color = vm_pop<uint8_t>(ptr);
    uint8_t h = vm_pop<uint8_t>(ptr);
    uint8_t w = vm_pop<uint8_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    int16_t x = vm_pop<int16_t>(ptr);
    vm_pop_end(ptr);
    SpritesU::fillRect(x, y, w, h, color);
}

static void sys_set_frame_rate()
{
    auto ptr = vm_pop_begin();
    uint8_t fr = vm_pop<uint8_t>(ptr);
    vm_pop_end(ptr);
    Arduboy2Base::setFrameRate(fr);
}

static void sys_next_frame()
{
    bool x = Arduboy2Base::nextFrame();
    vm_push(uint8_t(x));
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

static void sys_uhexstr()
{
    auto ptr = vm_pop_begin();
    uint32_t x = vm_pop<uint32_t>(ptr);
    vm_pop_end(ptr);
    
    struct { char d[8]; } str{};
    
    char* p = str.d;
    if(x == 0)
        st_inc(p, '0');
    while(x != 0)
    {
        uint8_t r = x & 15;
        x >>= 4;
        st_inc(p, char(r + (r < 10 ? '0' : 'a' - 10)));
    }
    
    // reverse
    reverse_str(str.d, p);
    
    vm_push(str);
}

static void sys_udecstr()
{
    auto ptr = vm_pop_begin();
    uint32_t x = vm_pop<uint32_t>(ptr);
    vm_pop_end(ptr);
    
    struct { char d[10]; } str{};
    
    char* p = str.d;
    if(x == 0)
        st_inc(p, '0');
    while(x != 0)
    {
        uint8_t r = x % 10;
        x /= 10;
        st_inc(p, char(r + '0'));
    }
    
    // reverse
    reverse_str(str.d, p);
    
    vm_push(str);
}

static void sys_draw_sprite()
{
    auto ptr = vm_pop_begin();
    uint16_t frame = vm_pop<uint16_t>(ptr);
    uint24_t image = vm_pop<uint24_t>(ptr);
    int16_t y = vm_pop<int16_t>(ptr);
    int16_t x = vm_pop<int16_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
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
    uint16_t tn   = vm_pop<uint16_t>(ptr);
    uint16_t tb   = vm_pop<uint16_t>(ptr);
    uint24_t font = vm_pop<uint24_t>(ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    int16_t  x    = vm_pop<int16_t> (ptr);
    vm_pop_end(ptr);
    
    (void)FX::readEnd();
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
    uint24_t tn   = vm_pop<uint24_t>(ptr);
    uint24_t tb   = vm_pop<uint24_t>(ptr);
    uint24_t font = vm_pop<uint24_t>(ptr);
    int16_t  y    = vm_pop<int16_t> (ptr);
    int16_t  x    = vm_pop<int16_t> (ptr);
    vm_pop_end(ptr);
    
    (void)FX::readEnd();
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
    FX::seekData(b);
    uint24_t t = 0;
    while(FX::readPendingUInt8() != '\0')
    {
        ++t;
        if(--n == 0) break;
    }
    vm_push(t);
    (void)FX::readEnd();
    FX::seekData(ards::vm.pc);
}

static void sys_strcmp()
{
    auto ptr = vm_pop_begin();
    uint16_t n1 = vm_pop<uint16_t>(ptr);
    uint16_t b1 = vm_pop<uint16_t>(ptr);
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
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
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
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
    (void)FX::readEnd();
    FX::seekData(ards::vm.pc);
}

static void sys_strcpy()
{
    auto ptr = vm_pop_begin();
    uint16_t n1 = vm_pop<uint16_t>(ptr);
    uint16_t b1 = vm_pop<uint16_t>(ptr);
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
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
}

static void sys_strcpy_P()
{
    auto ptr = vm_pop_begin();
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    (void)FX::readEnd();
    FX::seekData(b1);
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
    FX::seekData(ards::vm.pc);
}

static void sys_strcat()
{
    auto ptr = vm_pop_begin();
    uint16_t n1 = vm_pop<uint16_t>(ptr);
    uint16_t b1 = vm_pop<uint16_t>(ptr);
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    
    char* p0 = reinterpret_cast<char*>(b0);
    char const* p1 = reinterpret_cast<char*>(b1);
    while(*p0 != '\0')
    {
        if(n0 == 0) return;
        ++p0;
        --n0;
    }
    
    for(;;)
    {
        uint8_t c = ld_inc(p1);
        if(c == 0) return;
        st_inc(p0, c);
        if(--n0 == 0) return;
        if(--n1 == 0)
        {
            *p0 = '\0';
            return;
        }
    }
}

static void sys_strcat_P()
{
    auto ptr = vm_pop_begin();
    uint24_t n1 = vm_pop<uint24_t>(ptr);
    uint24_t b1 = vm_pop<uint24_t>(ptr);
    uint16_t n0 = vm_pop<uint16_t>(ptr);
    uint16_t b0 = vm_pop<uint16_t>(ptr);
    vm_pop_end(ptr);
    
    char* p0 = reinterpret_cast<char*>(b0);
    while(*p0 != '\0')
    {
        if(n0 == 0) return;
        ++p0;
        --n0;
    }
    
    (void)FX::readEnd();
    FX::seekData(b1);
    for(;;)
    {
        uint8_t c = FX::readPendingUInt8();
        if(c == 0) goto done;
        st_inc(p0, c);
        if(--n0 == 0) goto done;
        if(--n1 == 0)
        {
            *p0 = '\0';
            goto done;
        }
    }
    
done:
    (void)FX::readEnd();
    FX::seekData(ards::vm.pc);
}

sys_func_t const SYS_FUNCS[] __attribute__((aligned(256))) PROGMEM =
{
    sys_display,
    sys_draw_pixel,
    sys_draw_filled_rect,
    sys_draw_sprite,
    sys_draw_text,
    sys_draw_text_P,
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
    sys_uhexstr,
    sys_udecstr,
    //sys_idecstr,
    sys_strlen,
    sys_strlen_P,
    sys_strcmp,
    sys_strcmp_P,
    sys_strcpy,
    sys_strcpy_P,
    sys_strcat,
    sys_strcat_P,
};
