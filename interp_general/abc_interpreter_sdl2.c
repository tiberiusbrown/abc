#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <abc_interp.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

static abc_interp_t interp;
static abc_host_t host;

static void* data;
static size_t data_size;
static uint64_t start_ticks;

static uint32_t display[128 * 64];

static SDL_AudioSpec audio_desired;
static SDL_AudioSpec audio_obtained;
static SDL_AudioDeviceID audio_device;

static void SDLCALL audio_callback(void* user, uint8_t* stream, int len)
{
    (void)user;
    int16_t* samples = (int16_t*)stream;
    uint32_t num_samples = (uint32_t)len / 2;
    uint32_t sample_rate = (uint32_t)audio_obtained.freq;
    abc_audio(&interp, &host, samples, num_samples, sample_rate);
}

static uint8_t host_prog(void* user, uint32_t addr)
{
    (void)user;
    if(addr < data_size)
        return ((uint8_t*)data)[addr];
    return 0;
}

static uint32_t host_millis(void* user)
{
    (void)user;
    return (uint32_t)(SDL_GetTicks64() - start_ticks);
}

static uint8_t host_buttons(void* user)
{
    (void)user;
    uint8_t b = 0;
    uint8_t const* k = SDL_GetKeyboardState(NULL);
    if(k[SDL_SCANCODE_UP   ]) b |= ABC_BUTTON_U;
    if(k[SDL_SCANCODE_DOWN ]) b |= ABC_BUTTON_D;
    if(k[SDL_SCANCODE_LEFT ]) b |= ABC_BUTTON_L;
    if(k[SDL_SCANCODE_RIGHT]) b |= ABC_BUTTON_R;
    if(k[SDL_SCANCODE_A    ]) b |= ABC_BUTTON_A;
    if(k[SDL_SCANCODE_Z    ]) b |= ABC_BUTTON_A;
    if(k[SDL_SCANCODE_B    ]) b |= ABC_BUTTON_B;
    if(k[SDL_SCANCODE_S    ]) b |= ABC_BUTTON_B;
    if(k[SDL_SCANCODE_X    ]) b |= ABC_BUTTON_B;
    return b;
}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <data.bin>\n", argv[0]);
        return 1;
    }

    {
        FILE* f = fopen(argv[1], "rb");
        if(!f)
        {
            fprintf(stderr, "Unable to open \"%s\"\n", argv[1]);
            return 1;
        }

        fseek(f, 0, SEEK_END);
        data_size = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);

        data = malloc(data_size);
        if(!data)
        {
            fprintf(stderr, "Unable to allocate buffer for \"%s\"\n", argv[1]);
            fclose(f);
            return 1;
        }

        size_t r = fread(data, 1, data_size, f);
        if(r != data_size)
        {
            fprintf(stderr, "Unable to read \"%s\"\n", argv[1]);
            free(data);
            fclose(f);
            return 1;
        }

        fclose(f);
    }

    int r = 0;

    memset(&interp, 0, sizeof(interp));
    memset(&host, 0, sizeof(host));

    host.prog = host_prog;
    host.millis = host_millis;
    host.buttons = host_buttons;

    if(0 != SDL_Init(SDL_INIT_EVERYTHING))
    {
        fprintf(stderr, "Unable to initialize SDL\n");
        r = 1;
        goto sdl_quit;
    }

    memset(&audio_desired, 0, sizeof(audio_desired));
    audio_desired.freq = 44100;
    audio_desired.format = AUDIO_S16;
    audio_desired.channels = 1;
    audio_desired.samples = 1024;
    audio_desired.callback = audio_callback;
    audio_desired.userdata = NULL;
    audio_device = SDL_OpenAudioDevice(
        NULL, 0,
        &audio_desired,
        &audio_obtained,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    SDL_PauseAudioDevice(audio_device, 0);

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    SDL_Window* window = SDL_CreateWindow(
        "ABC Interpreter",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        6 * 128,
        6 * 64,
        0);
    if(!window)
    {
        fprintf(stderr, "Unable to create window\n");
        r = 1;
        goto sdl_quit;
    }

    SDL_Renderer* renderer;
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(!renderer)
    {
        fprintf(stderr, "Unable to create renderer\n");
        r = 1;
        goto sdl_destroy_window;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 128, 64);
    if(!texture)
    {
        fprintf(stderr, "Unable to create display texture\n");
        r = 1;
        goto sdl_destroy_renderer;
    }

    bool quit = false;
    while(!quit)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                quit = true;
        }

        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        bool idle = false;
        for(unsigned i = 0; !idle && i < 100000; ++i)
        {
            SDL_LockAudioDevice(audio_device);
            for(unsigned j = 0; !idle && j < 10; ++j)
            {
                abc_result_t t = abc_run(&interp, &host);
                if(t == ABC_RESULT_ERROR)
                    __debugbreak();
                if(t == ABC_RESULT_IDLE)
                    idle = true;
            }
            SDL_UnlockAudioDevice(audio_device);
        }

        for(unsigned y = 0; y < 64; ++y)
        {
            for(unsigned x = 0; x < 128; ++x)
            {
                uint32_t t = 0xff000000;
                if(interp.display[(y >> 3) * 128 + x] & (1 << (y & 7)))
                    t = 0xffc0c0c0;
                display[y * 128 + x] = t;
            }
        }
        SDL_UpdateTexture(texture, NULL, display, 128 * sizeof(uint32_t));

        SDL_RenderCopy(renderer, texture, NULL, NULL);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
sdl_destroy_renderer:
    SDL_DestroyRenderer(renderer);
sdl_destroy_window:
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(audio_device);
sdl_quit:
    SDL_Quit();
    free(data);

    return r;
}
