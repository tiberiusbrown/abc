#include <abc_interp.h>

#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__APPLE__)
#define SOKOL_METAL
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE33
#endif

#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_gp.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_time.h"

static abc_interp_t interp;
static abc_host_t host;

static int16_t* audio_buffer = NULL;
static int audio_buffer_size = 0;

static void* data = NULL;
static uint32_t data_size = 0;
static uint64_t start_tick;

static uint32_t display[128 * 64];
static sg_image display_image;
static uint8_t buttons = 0;

static uint8_t host_prog(void* user, uint32_t addr)
{
    (void)user;
    return data && addr < data_size ? ((uint8_t*)data)[addr] : 0;
}

static uint32_t host_millis(void* user)
{
    (void)user;
    return (uint32_t)stm_ms(stm_since(start_tick));
}

static uint8_t host_buttons(void* user)
{
    (void)user;
    return buttons;
}

static uint32_t host_rand_seed(void* user)
{
    (void)user;
    return (uint32_t)stm_now();
}

static void cb_stream(float* buffer, int num_frames, int num_channels)
{
    if(audio_buffer_size < num_frames)
    {
        free(audio_buffer);
        audio_buffer_size = num_frames;
        audio_buffer = (int16_t*)malloc(sizeof(int16_t) * audio_buffer_size);
    }

    if(!audio_buffer)
        return;

    abc_audio(&interp, &host, audio_buffer, audio_buffer_size, saudio_sample_rate());

    for(int n = 0; n < num_frames; ++n)
    {
        float sample = (float)(audio_buffer[n]) * (1.f / 32767);
        for(int c = 0; c < num_channels; ++c)
            buffer[n * num_channels + c] = sample;
    }
}

static void cb_init(void)
{
    stm_setup();

    host.user = NULL;
    host.prog = host_prog;
    host.millis = host_millis;
    host.buttons = host_buttons;
    host.debug_putc = NULL;
    host.rand_seed = host_rand_seed;

    memset(&interp, 0, sizeof(interp));

    saudio_setup(&(saudio_desc) {
        .buffer_frames = 512,
        .stream_cb = cb_stream,
        .logger.func = slog_func
    });

    sg_setup(&(sg_desc) {
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    memset(display, 0, sizeof(display));
    display_image = sg_make_image(&(sg_image_desc) {
        .width = 128,
        .height = 64,
        .usage = SG_USAGE_STREAM,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .label = "Display",
    });

    sgp_setup(&(sgp_desc) { 0 });

    start_tick = stm_now();
}

static void cb_frame(void)
{
    if(data != NULL)
    {
        int idle = 0;
        for(unsigned i = 0; !idle && i < 100; ++i)
        {
            for(unsigned j = 0; !idle && j < 1000; ++j)
            {
                abc_result_t r = abc_run(&interp, &host);
                if(r == ABC_RESULT_IDLE)
                    idle = 1;
            }
        }

        for(unsigned y = 0; y < 64; ++y)
        {
            for(unsigned x = 0; x < 128; ++x)
            {
                uint32_t t = 0xff000000;
                uint32_t b = interp.display[y * 128 + x];
                b = (b * 0xc0) >> 8;
                b += 0x10;
                t += (b << 0);
                t += (b << 8);
                t += (b << 16);
                display[y * 128 + x] = t;
            }
        }
    }

    sg_update_image(display_image, &(sg_image_data) {
        .subimage[0][0] = { display, sizeof(display) },
    });

    int w = sapp_width();
    int h = sapp_height();
    sgp_begin(w, h);
    sgp_viewport(0, 0, w, h);
    sgp_project(0.f, 1.f, 0.f, 1.f);

    sgp_set_color(0.2f, 0.2f, 0.2f, 1.f);
    sgp_clear();

    if(data != NULL)
    {
        sgp_set_color(1.f, 1.f, 1.f, 1.f);
        sgp_set_image(0, display_image);
        sgp_draw_filled_rect(0.f, 0.f, 1.f, 1.f);
    }

    sg_begin_pass(&(sg_pass) { .swapchain = sglue_swapchain() });
    sgp_flush();
    sgp_end();
    sg_end_pass();
    sg_commit();
}

static void cb_cleanup(void)
{
    sgp_shutdown();
    sg_destroy_image(display_image);
    sg_shutdown();
    saudio_shutdown();
    free(audio_buffer);
}

static void cb_event(sapp_event const* e)
{
    if(e->type == SAPP_EVENTTYPE_KEY_DOWN)
    {
        switch(e->key_code)
        {
        case SAPP_KEYCODE_UP:    buttons |= ABC_BUTTON_U; break;
        case SAPP_KEYCODE_DOWN:  buttons |= ABC_BUTTON_D; break;
        case SAPP_KEYCODE_LEFT:  buttons |= ABC_BUTTON_L; break;
        case SAPP_KEYCODE_RIGHT: buttons |= ABC_BUTTON_R; break;
        case SAPP_KEYCODE_A:
        case SAPP_KEYCODE_Z:
            buttons |= ABC_BUTTON_A; break;
        case SAPP_KEYCODE_B: 
        case SAPP_KEYCODE_S: 
        case SAPP_KEYCODE_X: 
            buttons |= ABC_BUTTON_B; break;
        default: break;
        }
    }
    else if(e->type == SAPP_EVENTTYPE_KEY_UP)
    {
        switch(e->key_code)
        {
        case SAPP_KEYCODE_UP:    buttons &= ~ABC_BUTTON_U; break;
        case SAPP_KEYCODE_DOWN:  buttons &= ~ABC_BUTTON_D; break;
        case SAPP_KEYCODE_LEFT:  buttons &= ~ABC_BUTTON_L; break;
        case SAPP_KEYCODE_RIGHT: buttons &= ~ABC_BUTTON_R; break;
        case SAPP_KEYCODE_A:
        case SAPP_KEYCODE_Z:
            buttons &= ~ABC_BUTTON_A; break;
        case SAPP_KEYCODE_B:
        case SAPP_KEYCODE_S:
        case SAPP_KEYCODE_X:
            buttons &= ~ABC_BUTTON_B; break;
        default: break;
        }
    }
    else
    {
        switch(e->type)
        {
        case SAPP_EVENTTYPE_UNFOCUSED:
        case SAPP_EVENTTYPE_SUSPENDED:
        case SAPP_EVENTTYPE_ICONIFIED:
            buttons = 0;
            break;
        default:
            break;
        }
    }
}

//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\summer_camp\\build\\fxdata.bin"

//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\abc\\examples\\basic\\fxdata.bin"
//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\abc\\examples\\circle\\fxdata.bin"
//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\abc\\examples\\font\\fxdata.bin"
//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\abc\\examples\\gray\\fxdata.bin"
//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\abc\\examples\\platformer\\fxdata.bin"
//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\abc\\examples\\pong\\fxdata.bin"
//#define OVERRIDE "C:\\Users\\Brown\\Documents\\GitHub\\abc\\examples\\snake\\fxdata.bin"

sapp_desc sokol_main(int argc, char* argv[])
{
#ifndef OVERRIDE
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <data.bin>\n", argv[0]);
        goto error;
    }
    char const* fname = argv[1];
#else
    (void)argc;
    (void)argv;
    char const* fname = OVERRIDE;
#endif


    {
        FILE* f = fopen(fname, "rb");
        if(!f)
        {
            fprintf(stderr, "Unable to open \"%s\"\n", fname);
            goto error;
        }

        fseek(f, 0, SEEK_END);
        data_size = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);

        data = malloc(data_size);
        if(!data)
        {
            fprintf(stderr, "Unable to allocate buffer for \"%s\"\n", fname);
            fclose(f);
            goto error;
        }

        size_t r = fread(data, 1, data_size, f);
        if(r != data_size)
        {
            fprintf(stderr, "Unable to read \"%s\"\n", fname);
            free(data);
            fclose(f);
            goto error;
        }

        fclose(f);
    }

error:
    return (sapp_desc) {
        .window_title = "ABC Interpreter",
        .width = 6 * 128,
        .height = 6 * 64,
        .init_cb = cb_init,
        .frame_cb = cb_frame,
        .cleanup_cb = cb_cleanup,
        .event_cb = cb_event,
    };
}
