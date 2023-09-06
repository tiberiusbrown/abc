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

inline uint8_t* vm_pop_begin()
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
    for(size_t i = 0; i < sizeof(T); ++i)
    {
        ards::vm.stack[ards::vm.sp++] = uint8_t(x);
        x >>= 8;
    }
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

sys_func_t const SYS_FUNCS[] __attribute__((aligned(256))) PROGMEM =
{
    sys_display,
    sys_draw_pixel,
    sys_draw_filled_rect,
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
};