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

typedef struct {
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
    SDL_AudioStream* stream;
    Sound* sounds[16];  // Max 16 simultaneous sounds
    int soundCount;
    Sound* bgm;  // Background music
    float masterVolume;
    int bufferSize;  // Store buffer size separately
} Audio;

// Audio functions
bool audio_init(Audio* audio);
void audio_cleanup(Audio* audio);
void audio_update(Audio* audio);

// Sound management
Sound* audio_create_sound(int16_t* data, int length, bool loop);
void audio_free_sound(Sound* sound);
void audio_play_sound(Audio* audio, Sound* sound);
void audio_stop_sound(Audio* audio, Sound* sound);
void audio_set_bgm(Audio* audio, Sound* bgm);

// Sound generation (for built-in effects)
Sound* audio_generate_shoot(void);
Sound* audio_generate_explosion(void);
Sound* audio_generate_ufo(void);
Sound* audio_generate_bgm(void);

#endif // AUDIO_H
