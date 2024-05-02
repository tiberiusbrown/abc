#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* audio channels for tones */
typedef enum
{
    ABC_CHANNEL_PRIMARY,
    ABC_CHANNEL_SECONDARY,
    ABC_CHANNEL_TONES,
} abc_channel_t;

typedef enum
{
    ABC_RESULT_NORMAL,     /* do nothing (continue executing) */
    ABC_RESULT_IDLE,       /* wait one millisecond */
    ABC_RESULT_BREAK,      /* debug break occurred */
    ABC_RESULT_ERROR,      /* halt: a runtime error occurred */
} abc_result_t;

typedef struct abc_host_t
{
    /****************************************************************
    * The following must not be NULL.                               *
    ****************************************************************/
    
    uint8_t (*prog)      (void* user, uint32_t addr);
    uint32_t(*millis)    (void* user);
    
    /****************************************************************
    * The following may be NULL, but functionality will be reduced. *
    ****************************************************************/

    uint8_t (*buttons)   (void* user);
    void    (*debug_putc)(void* user, char c);
    void    (*tones)     (void* user, abc_channel_t channel, uint32_t addr);
    
    void* user;
    
} abc_host_t;

/* Interpreter state: zero-initialize to reset interpreter. */
typedef struct abc_interp_t
{
    
    /* The host should use this for rendering. */
    uint8_t  display[1024];
    
    uint8_t  display_buffer[1024];
    uint8_t  saved[1024];
    uint8_t  globals[1024];
    uint8_t  stack[256];
    uint32_t call_stack[24];
    
    /* program counter */
    uint32_t pc;
    
    /* whether audio is enabled */
    uint8_t audio_enabled;
    
    /* call stack pointer */
    uint8_t  csp;
    
    /* stack pointer */
    uint8_t  sp;
    
    /* whether a save exists */
    uint8_t  has_save;
    
    /* button state */
    uint8_t  buttons_prev;
    uint8_t  buttons_curr;
    
} abc_interp_t;

/*
Execute a single instruction.
*/
abc_result_t run(abc_interp_t* interp, abc_host_t const* host);

#ifdef __cplusplus
}
#endif
