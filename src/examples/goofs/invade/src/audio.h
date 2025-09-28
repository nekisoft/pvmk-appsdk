#ifndef AUDIO_H
#define AUDIO_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_FORMAT SDL_AUDIO_S16
#define AUDIO_CHANNELS 2
#define AUDIO_BUFFER_SIZE 512  // Buffer size in samples

typedef struct {
    int16_t* data;
    int length;
    int position;
    bool loop;
} Sound;

// Audio functions
bool audio_init(void);
void audio_cleanup(void);
void audio_update(void);

// Sound management
Sound* audio_create_sound(int16_t* data, int length, bool loop);
void audio_free_sound(Sound* sound);
void audio_play_sound(Sound* sound);
void audio_stop_sound(Sound* sound);
void audio_set_bgm(Sound* bgm);

// Sound generation (for built-in effects)
Sound* audio_generate_shoot(void);
Sound* audio_generate_explosion(void);
Sound* audio_generate_ufo(void);
Sound* audio_generate_bgm(void);

#endif // AUDIO_H
