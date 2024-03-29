#pragma once

#include <stdint.h>

/* masks for the "buttons" method in abc_host_t to indicate button state */
enum
{
    ABC_BUTTON_A = 1 << 3,
    ABC_BUTTON_B = 1 << 2,
    ABC_BUTTON_U = 1 << 7,
    ABC_BUTTON_D = 1 << 4,
    ABC_BUTTON_L = 1 << 5,
    ABC_BUTTON_R = 1 << 6,
};

typedef struct abc_host_t
{
    /* the following must not be NULL */
    
    uint8_t(*prog)(void* user, uint32_t addr);
    uint32_t(*millis)(void* user);
    
    /* the following may be NULL, but functionality may be reduced if so */
    
    uint8_t(*buttons)(void* user);
    
    void(*tones_play)(void* user, uint32_t tone);
    uint8_t(*tones_playing)(void* user);
    void(*tones_stop)(void* user);
    
    uint8_t(*audio_enabled)(void* user);
    void(*audio_toggle)(void* user);
    
    void* user;
    
} abc_host_t;

/* zero-initialize to reset interpreter */
typedef struct abc_interp_t
{
    uint8_t  display[1024];
    uint8_t  display_buffer[1024];
    uint8_t  saved[1024];
    uint8_t  globals[1024];
    uint8_t  stack[256];
    uint32_t call_stack[24];
    uint32_t pc;
    uint32_t tone;
    uint8_t  csp;
    uint8_t  sp;
    uint8_t  has_save;
    uint8_t  buttons_prev;
    uint8_t  buttons_curr;
} abc_interp_t;

typedef enum abc_result_t
{
    ABC_NORMAL,     /* do nothing (continue executing) */
    ABC_IDLE,       /* wait until next host frame */
    ABC_ERROR,      /* halt: a runtime error occurred */
} abc_result_t;

/*
Execute a single instruction.
Returns a status for the action the host should take (see above)
*/
abc_result_t run(abc_interp_t* interp, abc_host_t const* host);
