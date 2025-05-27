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
    ABC_RESULT_IDLE,       /* defer to host for a bit */
    ABC_RESULT_BREAK,      /* debug break occurred */
    ABC_RESULT_ERROR,      /* halt: a runtime error occurred */
} abc_result_t;

typedef struct abc_interp_t abc_interp_t;

/********************************************************************
* Host platform interface.                                          *
********************************************************************/
typedef struct abc_host_t
{
    /****************************************************************
    * The following must not be NULL.                               *
    ****************************************************************/
    
    /* Access a single byte in the compiled bytecode. */
    uint8_t (*prog)         (void* user, uint32_t addr);
    
    /****************************************************************
    * The following may be NULL, but games may break. *
    ****************************************************************/

    /* Get a bitmask of which buttons are pressed (ABC_BUTTON_*). */
    uint8_t (*buttons)      (void* user);

    /* Get the number of milliseconds elapsed since start. */
    uint32_t(*millis)       (void* user);

    /* Push a single character to some debug output stream. */
    void    (*debug_putc)   (void* user, char c);

    /* Generate some entropy for seeding a PRNG. */
    uint32_t(*rand_seed)    (void* user);

    /* Store interp->saved into persistent memory. */
    void    (*save)         (void* user, abc_interp_t const* interp);
    
    void* user;
    
} abc_host_t;

/********************************************************************
* Interpreter state. To initialize:                                 *
*     1. Clear to all-zero (e.g., with memset).                     *
*     2. If save exists, copy to 'saved' and set 'has_save' to 1.   *
********************************************************************/
struct abc_interp_t
{
    
    /* The host can use this for rendering: 128x64 8-bit pixels. */
    uint8_t  display[8192];

    /* The host can persist this to support saved games. */
    uint8_t  saved[1024];
    
    uint8_t  display_buffer[1024];
    uint8_t  globals[1024];
    uint8_t  stack[256];
    uint32_t call_stack[24];
    
    /* Program counter */
    uint32_t pc;
    
    /* Audio state */
    uint32_t audio_ns_rem;
    uint32_t audio_phase[3];
    uint32_t audio_addrs[3];
    uint8_t  audio_tones[3];
    uint8_t  audio_ticks[3];
    uint8_t  music_active;
    uint8_t  audio_enabled;
    
    /* Call stack pointer */
    uint8_t  csp;
    
    /* Stack pointer */
    uint8_t  sp;
    
    /* Whether a save exists */
    uint8_t  has_save;
    
    /* Other state */
    uint8_t  shades;
    uint32_t seed;
    uint32_t text_font;
    uint8_t  text_color;
    uint8_t  buttons_prev;
    uint8_t  buttons_curr;
    uint8_t  waiting_for_frame;
    uint32_t frame_start;
    uint32_t frame_dur;
    uint16_t cmd_ptr;
    uint16_t batch_ptr;
    uint8_t  current_plane;
    int8_t   batch_px;
    int8_t   batch_py;
    int8_t   batch_dx;
    int8_t   batch_dy;
    
};

/*
Execute a single instruction.
*/
abc_result_t abc_run(
    abc_interp_t* interp,
    abc_host_t const* host
);

/*
Fill audio buffer with tones data.
The host should call this function regularly
*/
void abc_audio(
    abc_interp_t* interp,
    abc_host_t const* host,
    int16_t* samples,     /* Audio sample buffer to fill */
    uint32_t num_samples, /* Number of requested samples */
    uint32_t sample_rate  /* Sample rate in Hz */
);

#ifdef __cplusplus
}
#endif
