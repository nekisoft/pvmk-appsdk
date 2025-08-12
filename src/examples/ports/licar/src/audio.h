#ifndef _LCR_AUDIO
#define _LCR_AUDIO

/** @file audio.h

  Licar audio

  This file implements the audio system. The module only computes audio samples
  (playing them must be done by the frontend). The audio format is 8bit, 8 KHz
  mono. This module does NOT deal with music (music is stored in a separate
  file, left to be optionally loaded and played by the frontend). Audio samples
  are generated procedurally.
*/

#ifdef LCR_MODULE_NAME
  #undef LCR_MODULE_NAME
#endif

#define LCR_MODULE_NAME "audio"

#define LCR_SOUND_NONE            0
#define LCR_SOUND_CLICK           1
#define LCR_SOUND_CRASH_SMALL     2
#define LCR_SOUND_CRASH_BIG       3
#define LCR_SOUND_ACCELERATOR     4
#define LCR_SOUND_FAN             5

#define LCR_AUDIO_CRASH_SOUND_LEN 2048
#define LCR_AUDIO_FAN_SOUND_LEN   4096

struct
{
  uint32_t frame;
  uint8_t on;
  uint8_t soundPlayed;
  uint16_t soundPlayedFrame;
  uint32_t noise;
  uint8_t crashSample;
  int engineIntensity;
  int engineOsc;
  int engineInc;
} LCR_audio;

/**
  Initializes the audio system, only call once.
*/
void LCR_audioInit(void)
{
  LCR_LOG0("initializing audio");
  LCR_audio.frame = 0;
  LCR_audio.on = 1;
  LCR_audio.soundPlayed = LCR_SOUND_NONE;
  LCR_audio.soundPlayedFrame = 0;
  LCR_audio.noise = 0;
  LCR_audio.crashSample = 0;
  LCR_audio.engineOsc = 0;
  LCR_audio.engineInc = 1;
  LCR_audio.engineIntensity = 0;
}

/**
  Sets the intensity of car engine sound that's constantly played (unless when
  intensitiy has been set to 0). Maximum value is 255.
*/
void LCR_audioSetEngineIntensity(uint8_t value)
{
  LCR_audio.engineIntensity = value;
}

/**
  Tells the audio system to play a certain sound (see the sound constants).
*/
void LCR_audioPlaySound(uint8_t sound)
{
#ifdef LCR_PLAY_SOUND_CALLBACK
  LCR_PLAY_SOUND_CALLBACK(sound)
#endif

  LCR_LOG2("playing sound");
  LCR_audio.soundPlayed = sound;
  LCR_audio.soundPlayedFrame = 0;
}

/**
  Same as LCR_audioPlaySound, but only plays the sound if no other sound is
  playing.
*/
void LCR_audioPlaySoundIfFree(uint8_t sound)
{
  if (LCR_audio.soundPlayed == LCR_SOUND_NONE)
    LCR_audioPlaySound(sound);
}

uint8_t _LCR_audioNoise(void)
{
  LCR_audio.noise = LCR_audio.noise * 32310901 + 37;
  return LCR_audio.noise >> 16;
}

/**
  Gets the next audio sample.
*/
uint8_t LCR_audioGetNextSample(void)
{
  unsigned char result = 128;

  if (!LCR_audio.on)
    return result;

  switch (LCR_audio.soundPlayed)
  {
    case LCR_SOUND_CRASH_SMALL: 
    case LCR_SOUND_CRASH_BIG:
    {
      int limit = (LCR_audio.soundPlayed == LCR_SOUND_CRASH_BIG ? 256 : 96) - 
        (LCR_audio.soundPlayedFrame * 256) / LCR_AUDIO_CRASH_SOUND_LEN;

      if (LCR_audio.frame % 2 || // lower frequency
        LCR_audio.soundPlayedFrame < LCR_AUDIO_CRASH_SOUND_LEN / 4)
        LCR_audio.crashSample = _LCR_audioNoise();

      if (LCR_audio.crashSample > limit)
        LCR_audio.crashSample = (_LCR_audioNoise() % 2) ? 0 : limit;

      result += LCR_audio.crashSample - limit / 2;

      if (limit == 0)
        LCR_audio.soundPlayed = LCR_SOUND_NONE;

      break;
    }

    case LCR_SOUND_ACCELERATOR:
      result = (LCR_audio.soundPlayedFrame * 14 +
        (((LCR_audio.soundPlayedFrame >> 2) * (LCR_audio.soundPlayedFrame >> 6))
        >> 4)) & 0x0f;

      if (LCR_audio.soundPlayedFrame >= 4000)
        LCR_audio.soundPlayed = LCR_SOUND_NONE;
      break;

    case LCR_SOUND_FAN:
    {
      int limit = LCR_AUDIO_FAN_SOUND_LEN - LCR_audio.soundPlayedFrame;

      if (limit > LCR_AUDIO_FAN_SOUND_LEN / 2)
        limit = LCR_AUDIO_FAN_SOUND_LEN - limit;

      limit = (limit * 256) / (LCR_AUDIO_FAN_SOUND_LEN / 2);

      result = _LCR_audioNoise();

      if (result > limit)
        result = limit;

      result = 128 - limit / 2 + result;

      if (LCR_audio.soundPlayedFrame >= LCR_AUDIO_FAN_SOUND_LEN)
        LCR_audio.soundPlayed = LCR_SOUND_NONE;

      break;
    }

    case LCR_SOUND_CLICK:
    {
      int v = ((LCR_audio.soundPlayedFrame >> 6) *
        (LCR_audio.soundPlayedFrame >> 1)
        + LCR_audio.soundPlayedFrame * 35) & 0x1c;

      if (LCR_audio.soundPlayedFrame < 250)
        result = v;
      else if (LCR_audio.soundPlayedFrame < 400)
        result = (v + 128) / 2;
      else if (LCR_audio.soundPlayedFrame < 600)
        result = (v + 3 * 128) / 4;
      else
        LCR_audio.soundPlayed = LCR_SOUND_NONE;

      break;
    }

    default:
      break;
  }

  if (LCR_audio.soundPlayed != LCR_SOUND_NONE)
    LCR_audio.soundPlayedFrame++;
  else if (LCR_audio.engineIntensity)
  {
    LCR_audio.engineOsc += LCR_audio.engineInc ? (((_LCR_audioNoise() % 256) <
      (10 + LCR_audio.engineIntensity))) : -31;

    if (LCR_audio.engineInc && LCR_audio.engineOsc >
      (90 + (LCR_audio.engineIntensity / 8)))
      LCR_audio.engineInc = 0;
    else if ((!LCR_audio.engineInc) && LCR_audio.engineOsc < 10)
      LCR_audio.engineInc = 1;

    result += LCR_audio.engineIntensity < 20 ?
      LCR_audio.engineOsc / 2 : LCR_audio.engineOsc;
  }

  LCR_audio.frame++;

  return result;
}

#endif // guard
