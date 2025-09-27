#include "audio.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void audio_callback(void* userdata, SDL_AudioStream* stream, 
                          int additional_amount, int total_amount) {
    (void)additional_amount;
    (void)total_amount;
    
    Audio* audio = (Audio*)userdata;
    int16_t* buffer = (int16_t*)SDL_malloc(audio->bufferSize * audio->spec.channels * sizeof(int16_t));
    
    if (!buffer) return;
    
    // Clear buffer
    memset(buffer, 0, audio->bufferSize * audio->spec.channels * sizeof(int16_t));
    
    // Mix background music
    if (audio->bgm && audio->bgm->data) {
        for (int i = 0; i < audio->bufferSize * audio->spec.channels; i++) {
            if (audio->bgm->position < audio->bgm->length) {
                buffer[i] = (int16_t)(audio->bgm->data[audio->bgm->position] * audio->masterVolume);
                audio->bgm->position++;
                
                // Loop if needed
                if (audio->bgm->position >= audio->bgm->length && audio->bgm->loop) {
                    audio->bgm->position = 0;
                }
            }
        }
    }
    
    // Mix sound effects
    for (int s = 0; s < audio->soundCount; s++) {
        Sound* sound = audio->sounds[s];
        if (!sound || !sound->data) continue;
        
        for (int i = 0; i < audio->bufferSize * audio->spec.channels; i++) {
            if (sound->position < sound->length) {
                int32_t mixed = buffer[i] + (int16_t)(sound->data[sound->position] * audio->masterVolume);
                
                // Clamp to prevent overflow
                if (mixed > 32767) mixed = 32767;
                if (mixed < -32768) mixed = -32768;
                
                buffer[i] = (int16_t)mixed;
                sound->position++;
                
                // Remove finished non-looping sounds
                if (sound->position >= sound->length && !sound->loop) {
                    // Mark for removal
                    audio->sounds[s] = NULL;
                }
            }
        }
    }
    
    // Clean up finished sounds
    int writeIndex = 0;
    for (int i = 0; i < audio->soundCount; i++) {
        if (audio->sounds[i] != NULL) {
            audio->sounds[writeIndex++] = audio->sounds[i];
        }
    }
    audio->soundCount = writeIndex;
    
    // Write to stream
    SDL_PutAudioStreamData(stream, buffer, 
        audio->bufferSize * audio->spec.channels * sizeof(int16_t));
    
    SDL_free(buffer);
}

bool audio_init(Audio* audio) {
    memset(audio, 0, sizeof(Audio));
    
    audio->spec.freq = AUDIO_SAMPLE_RATE;
    audio->spec.format = AUDIO_FORMAT;
    audio->spec.channels = AUDIO_CHANNELS;
    audio->bufferSize = AUDIO_BUFFER_SIZE;
    audio->masterVolume = 0.7f;
    
    audio->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, 
        &audio->spec, audio_callback, audio);
    
    if (!audio->stream) {
        return false;
    }
    
    audio->device = SDL_GetAudioStreamDevice(audio->stream);
    SDL_ResumeAudioDevice(audio->device);
    
    return true;
}

void audio_cleanup(Audio* audio) {
    if (audio->device) {
        SDL_PauseAudioDevice(audio->device);
        SDL_CloseAudioDevice(audio->device);
    }
    
    // Free all sounds
    for (int i = 0; i < audio->soundCount; i++) {
        if (audio->sounds[i]) {
            audio_free_sound(audio->sounds[i]);
        }
    }
    
    if (audio->bgm) {
        audio_free_sound(audio->bgm);
    }
}

void audio_update(Audio* audio) {
    // Audio is handled in the callback
    (void)audio;
}

Sound* audio_create_sound(int16_t* data, int length, bool loop) {
    Sound* sound = (Sound*)malloc(sizeof(Sound));
    if (!sound) return NULL;
    
    sound->data = data;
    sound->length = length;
    sound->position = 0;
    sound->loop = loop;
    
    return sound;
}

void audio_free_sound(Sound* sound) {
    if (sound) {
        if (sound->data) {
            free(sound->data);
        }
        free(sound);
    }
}

void audio_play_sound(Audio* audio, Sound* sound) {
    if (!sound || audio->soundCount >= 16) return;
    
    // Reset position for replayable sounds
    sound->position = 0;
    
    // Add to playing sounds
    audio->sounds[audio->soundCount++] = sound;
}

void audio_stop_sound(Audio* audio, Sound* sound) {
    for (int i = 0; i < audio->soundCount; i++) {
        if (audio->sounds[i] == sound) {
            audio->sounds[i] = NULL;
            return;
        }
    }
}

void audio_set_bgm(Audio* audio, Sound* bgm) {
    audio->bgm = bgm;
    if (bgm) {
        bgm->position = 0;
    }
}

// Sound generation functions
Sound* audio_generate_shoot(void) {
    int length = AUDIO_SAMPLE_RATE / 4;  // 0.25 seconds
    int16_t* data = (int16_t*)malloc(length * AUDIO_CHANNELS * sizeof(int16_t));
    if (!data) return NULL;
    
    // Generate a simple laser sound (frequency sweep)
    for (int i = 0; i < length; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float frequency = 800.0f * (1.0f - t * 3.0f);  // Sweep from 800Hz to 400Hz
        float amplitude = 0.3f * (1.0f - t * 4.0f);   // Fade out
        
        int16_t sample = (int16_t)(sin(2.0f * M_PI * frequency * t) * amplitude * 32767);
        
        // Stereo
        data[i * 2] = sample;
        data[i * 2 + 1] = sample;
    }
    
    return audio_create_sound(data, length * AUDIO_CHANNELS, false);
}

Sound* audio_generate_explosion(void) {
    int length = AUDIO_SAMPLE_RATE / 2;  // 0.5 seconds
    int16_t* data = (int16_t*)malloc(length * AUDIO_CHANNELS * sizeof(int16_t));
    if (!data) return NULL;
    
    // Generate white noise with envelope
    for (int i = 0; i < length; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float amplitude = 0.5f * exp(-t * 8.0f);  // Exponential decay
        
        // Add some low frequency rumble
        float rumble = sin(2.0f * M_PI * 50.0f * t) * 0.3f;
        
        int16_t sample = (int16_t)(((rand() / (float)RAND_MAX) * 2.0f - 1.0f + rumble) 
                                  * amplitude * 32767);
        
        // Stereo
        data[i * 2] = sample;
        data[i * 2 + 1] = sample;
    }
    
    return audio_create_sound(data, length * AUDIO_CHANNELS, false);
}

Sound* audio_generate_ufo(void) {
    int length = AUDIO_SAMPLE_RATE;  // 1 second
    int16_t* data = (int16_t*)malloc(length * AUDIO_CHANNELS * sizeof(int16_t));
    if (!data) return NULL;
    
    // Generate UFO wobble sound
    for (int i = 0; i < length; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float wobble = sin(2.0f * M_PI * 7.0f * t);  // 7Hz wobble
        float frequency = 400.0f + wobble * 100.0f;   // 300-500Hz
        
        int16_t sample = (int16_t)(sin(2.0f * M_PI * frequency * t) * 0.2f * 32767);
        
        // Stereo with slight phase shift for spatial effect
        data[i * 2] = sample;
        data[i * 2 + 1] = (int16_t)(sin(2.0f * M_PI * frequency * t + 0.1f) * 0.2f * 32767);
    }
    
    return audio_create_sound(data, length * AUDIO_CHANNELS, true);
}

Sound* audio_generate_bgm(void) {
    int length = AUDIO_SAMPLE_RATE * 8;  // 8 seconds loop
    int16_t* data = (int16_t*)malloc(length * AUDIO_CHANNELS * sizeof(int16_t));
    if (!data) return NULL;
    
    // Generate simple bass line
    float notes[] = {110.0f, 110.0f, 165.0f, 110.0f, 
                     146.8f, 110.0f, 165.0f, 220.0f};  // A2, A2, E3, A2, D3, A2, E3, A3
    
    for (int i = 0; i < length; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        int noteIndex = (int)(t * 2) % 8;  // Change note every 0.5 seconds
        float frequency = notes[noteIndex];
        
        // Bass sound with harmonics
        float sample = sin(2.0f * M_PI * frequency * t) * 0.15f;
        sample += sin(2.0f * M_PI * frequency * 2.0f * t) * 0.05f;  // Octave
        sample += sin(2.0f * M_PI * frequency * 3.0f * t) * 0.03f;  // Fifth
        
        int16_t output = (int16_t)(sample * 32767);
        
        // Stereo
        data[i * 2] = output;
        data[i * 2 + 1] = output;
    }
    
    return audio_create_sound(data, length * AUDIO_CHANNELS, true);
}
