#ifndef _LCR_GAME_H
#define _LCR_GAME_H

/** @file game.h

  Licar: game module

  This file implements the backend of a complete, actually playable game with
  graphics, sound etc., and is meant to be included and used by specific
  frontends (which will handle each platform's hardware details and I/O). See
  the frontend info below for help with porting the game to a new platform. This
  module (with the help of other modules) will perform all the computations
  (physics, graphics, audio, ...) and only use the frontend's quite primitive
  functions to actually present the results to the user.

  UNITS: There are various kinds of units used to ensure independence of the
  modules. Here is a summary:

  - LCR_GameUnit: data type, abstract unit of the game (racing module). One map
    square is LCR_GAME_UNITs long, a full angle is also LCR_GAME_UNITs.
  - LCR_GAME_UNIT: Size of one game square and full angle in LCR_GameUnits.
      Square height is only half of this.
  - S3L_Unit: data type, small3dlib's unit. May change with renderer change.
  - S3L_F: small3dlib's value representing 1.0.
  - LCR_RENDERER_UNIT: for the renderer one map square is this many S3L_Units.
  - TPE_Unit: tinyphysicsengine's unit. May change with phys. engine change.
  - TPE_F: tinyphysicsengine's value representing value 1.0.
  - LCR_PHYSICS_UNIT: for the phys. eng. one map square is this many TPE_Units.

  COORDINATE SYSTEM AND ROTATIONS: The game itself (racing module) is
  independent of rendering and physics libraries, but out of convenient adopts
  their coordinate system (X right, Y up, Z forward) and rotations (Euler
  angles, by Z, then by X, then Y).

  DATA FILE: The game uses so called data file to store various resources,
  mainly maps and replays. There is considered to be one abstract global file
  which is just a long text string. Internally the global file is composed of
  a hardcoded internal data file string (stored in assets) with very basic maps,
  and an optional user file (appended to the internal file) that the frontend
  may provide, allowing adding more resources without recompiling the game. The
  user file may be disabled on platforms that e.g. don't have file systems, but
  the internal file will be always present. The format of the data file is
  following: it consists of data strings separated by '#' character (data
  strings cannot contain this character). Each data string starts with a magic
  number: a single character identifying the type of the resource (map, replay,
  ...), then the name of the data follows, then character ';' and then the data
  string itself (up until next '#' or end of file). The format of the string
  depends on the type of the data, i.e. the format of map string has a special
  format (described in the map module) etc.
*/

#include <stdint.h>

#define LCR_KEY_UP     0x00
#define LCR_KEY_RIGHT  0x01
#define LCR_KEY_DOWN   0x02
#define LCR_KEY_LEFT   0x03
#define LCR_KEY_A      0x04 ///< confirm, restart race
#define LCR_KEY_B      0x05 ///< cancel, open menu

#define LCR_KEYS_TOTAL 6

/*
  FOR FRONTENDS (porting to other platforms):

  - Implement the frontend functions given below according to their description.
  - Implement the main program and game loop.
  - Call some of the frontend funtions given below in your main program in
    places as described in the description of the function.
  - If you want to support music, make your code load and play the "music"
    file in the asset directory. It is in raw format, storing 8bit unsigned
    samples at 8 KHz mono. Use the LCR_gameMusicOn function to check whether the
    music is currently enabled (if not, stop playing it). If you don't want to
    support music, set LCR_SETTING_MUSIC to 0 in your frontend code (before
    including this module) so that the game knows music is disabled.
  - LCR_ARG_HELP_STR is a string containing help for CLI arguments that can be
    passed to LCR_gameInit. You can use this in your CLI help. You can also use
    other strings such as LCR_VERSION.
*/

/**
  Implement this in your frontend. Returns 1 if given key (see LCR_KEY_*
  constants) is pressed or 0 otherwise.
*/
uint8_t LCR_keyPressed(uint8_t key);

/**
  Implement this in your frontend. This function pauses program execution for
  given amount of milliseconds and relieves the CPU usage. On platforms that
  don't support this the function may simply do nothing.
*/
void LCR_sleep(uint16_t timeMs);

/**
  Implement this in your frontend. This function draws a pixel of given color
  to the screen back buffer (i.e. NOT directly to screen, back buffer shall
  only be copied to front buffer once the LCR_gameStep function finishes all
  rendering). This function should NOT check for out-of-screen coordinates, this
  is handled by the game internals and out-of-screen pixels will never be drawn
  (your code unnecessarily checking it again would likely slow down rendering a
  lot, as drawing pixels is a bottleneck). The color value depends on game
  settings but is normally an RGB565 value. The index parameter says the
  coordinate at which to write the pixel, assuming the pixels are stored by
  rows, from top to bottom.
*/
void LCR_drawPixel(unsigned long index, uint16_t color);

/**
  Implement this in your frontend. This function will be called to log what the
  program is doing. Typically this function should print the string to console.
  If you want to ignore logging, simply make the function do nothing.
*/
void LCR_log(const char *str);

/**
  Implement this in your frontend. This function serves for loading optional
  data file that allows adding more maps, replays etc. If your frontend won't
  support this, just make the function return 0. Otherwise it must return
  characters from the data file one by one; after reaching the end of file 0
  must be returned and the reading position will be reset to start from
  beginning again.
*/
char LCR_getNextDataFileChar(void);

/**
  Implement this in your frontend. This serves to store data in the optional
  data file (e.g. replays and info about beaten maps). If your frontend doesn't
  support this (e.g. because the file is read only), the function may ignore the
  append, but if the file is otherwise supported, a rewind of the read position
  must still be performed. If appending is supported, the function must append
  the provided string to the data file AND then reset the data file reading
  position back to the start.
*/
void LCR_appendDataStr(const char *str);

/**
  Call this function in your frontend at the start of the program. Pass program
  arguments if there are any.
*/
void LCR_gameInit(int argc, const char **argv);

/**
  Call this function in your frontend right before program end.
*/
void LCR_gameEnd(void);

/**
  Call this function in your frontend repeatedly inside the main loop, pass the
  current time as the number of milliseconds since program start. This function
  will perform the game step AND other things such as checking the input states,
  rendering or sleeping (all using the above described functions you should
  implement). Returns 0 if program should end, otherwise 1.
*/
uint8_t LCR_gameStep(uint32_t timeMs);

/**
  Returns 1 if music is currently on, else 0. If your frontend supports music,
  use this function at each frame to know whether you should start/stop playing
  music. Otherwise ignore this.
*/
uint8_t LCR_gameMusicOn(void);

/**
  Gets the next audio sample (unsigned 8bit samples, 8 KHz mono). If your
  frontend supports sound, call this continuously and play the stream of
  samples, otherwise ignore this.
*/
uint8_t LCR_gameGetNextAudioSample(void);

/**
  This macro may be redefined by frontend to a command that will be periodically
  performed during map loading. This exists to prevent making the program seem
  unresponsive or being killed by a watchdog.
*/
#ifndef LCR_LOADING_COMMAND
  #define LCR_LOADING_COMMAND ;
#endif

/**
  This macro may be redefined to a command that will be executed upon certain
  sound. This can be used if the platform can't play the game's 8 bit audio but
  wants to play it's own sounds, e.g. beeps.
*/
#ifndef LCR_PLAY_SOUND_CALLBACK
  #define LCR_PLAY_SOUND_CALLBACK(s) ;
#endif

/*
  To make the game log FPS define a macro named LCR_FPS_GET_MS to a command that
  evaluates to the current number of milliseconds since some given time. It
  will be used to measure frame duration and average values will be logged.
*/

//------------------------------------------------------------------------------

#define LCR_LOG0(s) ;
#define LCR_LOG1(s) ;
#define LCR_LOG2(s) ;

#define LCR_LOG0_NUM(x) ;
#define LCR_LOG1_NUM(x) ;
#define LCR_LOG2_NUM(x) ;

#define LCR_MODULE_NAME "game"

#if LCR_SETTING_LOG_LEVEL > 0

void _LCR_logNum(uint32_t n)
{
  char s[9];

  s[8] = 0;

  for (int i = 7; i >= 0; --i)
  {
    s[i] = n % 16;
    s[i] = s[i] < 10 ? ('0' + s[i]) : ('a' - 10 + s[i]);
    n /= 16;
  }

  LCR_log(s);
}

  #undef LCR_LOG0
  #undef LCR_LOG0_NUM
  #define LCR_LOG0(s) LCR_log("[" LCR_MODULE_NAME "] " s);
  #define LCR_LOG0_NUM(x) _LCR_logNum(x);

  #if LCR_SETTING_LOG_LEVEL > 1
    #undef LCR_LOG1
    #undef LCR_LOG1_NUM
    #define LCR_LOG1(s) LCR_log("[" LCR_MODULE_NAME "] " s);
    #define LCR_LOG1_NUM(x) _LCR_logNum(x);

    #if LCR_SETTING_LOG_LEVEL > 2
      #undef LCR_LOG2
      #undef LCR_LOG2_NUM
      #define LCR_LOG2(s) LCR_log("[" LCR_MODULE_NAME "] " s);
      #define LCR_LOG2_NUM(x) _LCR_logNum(x);
    #endif
  #endif
#endif

#define LCR_CAMERA_MODE_DRIVE       0x00
#define LCR_CAMERA_MODE_DRIVE2      0x01
#define LCR_CAMERA_MODE_INSIDE      0x02
#define LCR_CAMERA_MODE_FREE        0x03

#define LCR_GAME_STATE_MENU         0x00
#define LCR_GAME_STATE_RUN_STARTING 0x01
#define LCR_GAME_STATE_RUN          0x02
#define LCR_GAME_STATE_RUN_FINISHED 0x03
#define LCR_GAME_STATE_LOADING      0x04
#define LCR_GAME_STATE_LOADED       0x05

#define LCR_GAME_STATE_END          0xff

// forward decls of pixel drawing functions for the renderer

/**
  Internal function for drawing pixels, takes into account setting etc., should
  be used instead of directly calling LCR_drawPixel.
*/
static inline void LCR_gameDrawPixel(unsigned long index, uint16_t color);

/**
  Internal pixel drawing function that puts a pixel at specified screen coords
  without checking for safety (it's faster but can only be done if we know for
  sure we're not drawing outside the screen).
*/
static inline void LCR_gameDrawPixelXYUnsafe(unsigned int x, unsigned int y,
  uint16_t color);

/**
  Internal pixel drawing function that checks for out-of-screen coordinates. Use
  this if the pixel can potentially lie of screen (however if you know it won't,
  use the normal unsafe function in sake of performance).
*/
static inline void LCR_gameDrawPixelXYSafe(unsigned int x, unsigned int y,
  uint_fast16_t color);

#include "general.h"
#include "racing.h"
#include "audio.h"
#include "assets.h"
#include "renderer.h"

#ifdef LCR_MODULE_NAME
  #undef LCR_MODULE_NAME
#endif

#define LCR_MODULE_NAME "game"

#define LCR_MENU_MAX_ITEMS 9 // don't change
#define LCR_DATA_ITEM_CHUNK (LCR_MENU_MAX_ITEMS - 1)
#define LCR_MENU_TABS 4

#if LCR_SETTING_GHOST_MAX_SAMPLES == 0
  #undef LCR_MENU_TABS
  #define LCR_MENU_TABS 3 // no ghosts => remove the tab for ghosts
#endif

#define LCR_MENU_STRING_SIZE 16

#define LCR_DATA_FILE_SEPARATOR '#'
#define LCR_DATA_FILE_SEPARATOR2 ';'

#define LCR_FREE_CAMERA_STEP \
  ((LCR_SETTING_FREE_CAMERA_SPEED * LCR_GAME_UNIT / 8) \
  / LCR_SETTING_FPS)

#if LCR_FREE_CAMERA_STEP == 0
  #define LCR_FREE_CAMERA_STEP 1
#endif

#define LCR_FREE_CAMERA_TURN_STEP \
  ((LCR_SETTING_FREE_CAMERA_TURN_SPEED * LCR_GAME_UNIT) \
  / (360 * LCR_SETTING_FPS))

#if LCR_FREE_CAMERA_TURN_STEP == 0
  #define LCR_FREE_CAMERA_TURN_STEP 1
#endif

#define LCR_POPUP_STR_SIZE 16

#define LCR_GUI_GAP \
  (1 + (LCR_EFFECTIVE_RESOLUTION_X + LCR_EFFECTIVE_RESOLUTION_Y) / 128)

struct
{
  uint8_t state;
  uint32_t statePrev;
  uint32_t stateStartTime;
  uint32_t time;                     ///< Current frame's time.
  uint32_t frame;                    ///< Current frame number.
  uint32_t nextRenderFrameTime;      ///< At which frame to render next frame.
  uint32_t nextRacingTickTime;       ///< When to simulate next physics tick.
  uint8_t cameraMode;
  uint8_t musicOn;
  uint8_t keyStates[LCR_KEYS_TOTAL]; /**< Assures unchanging key states
                                          during a single frame, hold number of
                                          frames for which the key has been
                                          continuously held. */ 
  uint32_t runTime;                  ///< Current time of the run, in ticks.
  uint8_t mapBeaten;

  char popupStr[LCR_POPUP_STR_SIZE];
  uint8_t popupCountdown;

  struct
  {
    uint8_t selectedTab;
    uint8_t selectedItem;
    uint8_t itemCount;
    char itemNames[LCR_MENU_MAX_ITEMS][LCR_MENU_STRING_SIZE];
    const char *itemNamePtrs[LCR_MENU_MAX_ITEMS]; ///< Helper array.
  } menu;

  struct
  {
    int32_t state;        ///< -1 if reading external data f., else pos.

    // Indices and counts are among the data of the same type.
    unsigned int firstItemIndex;
    unsigned int itemsTotal;
  } dataFile;

#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
#define LCR_GHOST_SAMPLE_SIZE 5
  struct
  {
    uint8_t active;
    uint8_t samples[LCR_SETTING_GHOST_MAX_SAMPLES * LCR_GHOST_SAMPLE_SIZE];
                      /**< Samples, each 5 bytes: 9 bits for X and Z, 10 for Y,
                           4 for each rotation component. */
    uint8_t stretch;  /**< Stretch of the base sample step, as a bit shift (i.e.
                           1 means the step will be 2x as long etc.). This is to
                           allow ghosts for even long replays. */
    int16_t offset[3];     ///< Small correcting position offset.
  } ghost;
#endif

#ifdef LCR_FPS_GET_MS
  uint16_t renderFrameMS;
  uint16_t physicsFrameMS;
  uint8_t  renderFramesMeasured;
  uint8_t  physicsFramesMeasured;
#endif
} LCR_game;

uint8_t LCR_gameMusicOn(void)
{
#if LCR_SETTING_MUSIC
  return LCR_game.musicOn;
#else
  return 0;
#endif
}

void LCR_gameDrawPixel(unsigned long index, uint16_t color)
{
#if LCR_SETTING_RESOLUTION_SUBDIVIDE <= 1
  LCR_drawPixel(index,color);
#else

  index = ((index / LCR_EFFECTIVE_RESOLUTION_X) * LCR_SETTING_RESOLUTION_X +
    (index % LCR_EFFECTIVE_RESOLUTION_X)) * LCR_SETTING_RESOLUTION_SUBDIVIDE;

  for (int y = 0; y < LCR_SETTING_RESOLUTION_SUBDIVIDE; ++y)
  {
    for (int x = 0; x < LCR_SETTING_RESOLUTION_SUBDIVIDE; ++x)
    {
      LCR_drawPixel(index,color);
      index++;
    }

    index += LCR_SETTING_RESOLUTION_X - LCR_SETTING_RESOLUTION_SUBDIVIDE;
  }
#endif
}

void LCR_gamePopupMessage(const char *str)
{
  if (*str == 0)
  {
    LCR_game.popupCountdown = 0;
    return;
  }

  for (uint8_t i = 0; i < LCR_POPUP_STR_SIZE - 1; ++i)
  {
    LCR_game.popupStr[i] = str[i];

    if (str[i] == 0)
      break; 
  }

  LCR_game.popupCountdown =
    (LCR_SETTING_POPUP_DURATION * LCR_SETTING_FPS) / 1000;
}

void LCR_gamePopupNumber(uint8_t num)
{
  LCR_game.popupStr[0] = '0' + num;
  LCR_game.popupStr[1] = 0;
  LCR_gamePopupMessage(LCR_game.popupStr); 
}

void LCR_gameDrawPixelXYUnsafe(unsigned int x, unsigned int y,
  uint16_t color)
{
  LCR_gameDrawPixel(y * LCR_EFFECTIVE_RESOLUTION_X + x,color);
}

void _LCR_physicdDebugDrawPixel(uint16_t x, uint16_t y, uint8_t color)
{
  if (x > 1 && x < LCR_EFFECTIVE_RESOLUTION_X - 2 &&
      y > 1 && y < LCR_EFFECTIVE_RESOLUTION_Y - 2)
  {
    uint16_t c = LCR_CONVERT_COLOR(0x8101) |
      (LCR_CONVERT_COLOR(0x8f1f) << (2 * color));

    for (int j = -1; j <= 2; ++j)
      for (int i = -1; i <= 2; ++i)
        LCR_gameDrawPixelXYUnsafe(x + i,y + j,c);
  }
}

static inline void LCR_gameDrawPixelXYSafe(unsigned int x, unsigned int y,
  uint_fast16_t color)
{
  if (x < LCR_EFFECTIVE_RESOLUTION_X && y < LCR_EFFECTIVE_RESOLUTION_Y)
    LCR_gameDrawPixelXYUnsafe(x,y,color);
}

void LCR_gameSetState(uint8_t state)
{
  LCR_LOG1("changing state to:");
  LCR_LOG1_NUM(state);
  LCR_game.statePrev = LCR_game.state;
  LCR_game.state = state;
  LCR_game.stateStartTime = LCR_game.time;
}

void LCR_gameResetRun(uint8_t replay, uint8_t ghost)
{
  LCR_GameUnit carTransform[6];

  LCR_LOG0("resetting run");
  LCR_mapReset();

  LCR_racingRestart(replay);
  LCR_rendererUnmarkCPs();
  LCR_racingGetCarTransform(carTransform,carTransform + 3,0);
  LCR_rendererSetCarTransform(carTransform,carTransform + 3);

  if (LCR_game.cameraMode != LCR_CAMERA_MODE_FREE)
  {
    LCR_rendererCameraReset();
    LCR_rendererLoadMapChunks();
  }

#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
  LCR_game.ghost.active = ghost;
#endif

  LCR_gameSetState(LCR_GAME_STATE_RUN_STARTING);
  LCR_game.runTime = 0;
}

void LCR_gameGetNthGhostSample(unsigned int n,
  LCR_GameUnit position[3], LCR_GameUnit rotation[3])
{
#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
  n *= LCR_GHOST_SAMPLE_SIZE;

  position[0] = LCR_game.ghost.samples[n];
  n++;

  position[0] |= ((LCR_GameUnit) (LCR_game.ghost.samples[n] & 0x01)) << 8;
  position[1] = LCR_game.ghost.samples[n] >> 1;

  n++;
  position[1] |= ((LCR_GameUnit) (LCR_game.ghost.samples[n] & 0x07)) << 7;
  position[2] = LCR_game.ghost.samples[n] >> 3;

  n++;
  position[2] |= ((LCR_GameUnit) (LCR_game.ghost.samples[n] & 0x0f)) << 5;
  rotation[0] = LCR_game.ghost.samples[n] >> 4;

  n++;
  rotation[1] = LCR_game.ghost.samples[n] & 0x0f;
  rotation[2] = LCR_game.ghost.samples[n] >> 4;

  for (int i = 0; i < 3; ++i)
  {
    if (i != 1)
      position[i] <<= 1;

    position[i] = (position[i] * LCR_GAME_UNIT) / 16;

    position[i] -= (LCR_MAP_SIZE_BLOCKS / 2) *
      (i == 1 ? LCR_GAME_UNIT / 2 : LCR_GAME_UNIT);

    rotation[i] = (rotation[i] * LCR_GAME_UNIT) / 16;
  }
#else
  for (int i = 0; i < 3; ++i)
  {
    position[i] = 0;
    rotation[i] = 0;
  }

#endif
}

void LCR_gameGhostGetTransform(uint32_t frame,
  LCR_GameUnit position[3], LCR_GameUnit rotation[3])
{
#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
  int n = ((frame >> LCR_game.ghost.stretch) / LCR_SETTING_GHOST_STEP); 

  LCR_gameGetNthGhostSample(n,position,rotation);

  if (n < LCR_SETTING_GHOST_MAX_SAMPLES - 1)
  {
    LCR_GameUnit carTransform[6];

    // interpolate:
    LCR_gameGetNthGhostSample(n + 1,carTransform,carTransform + 3);

    n = (frame >> LCR_game.ghost.stretch) % LCR_SETTING_GHOST_STEP;

    for (int i = 0; i < 3; ++i)
    {
      position[i] = position[i] + ((carTransform[i] - position[i]) * n) /
        LCR_SETTING_GHOST_STEP + LCR_game.ghost.offset[i];

      // rotations are a bit harder to interpolate (e.g. 1 -> 359 deg.)
      carTransform[3 + i] -= rotation[i];

      if ((carTransform[3 + i] > LCR_GAME_UNIT / 2) ||
        (carTransform[3 + i] < -1 * LCR_GAME_UNIT / 2))
        carTransform[3 + i] = -1 *(
          carTransform[3 + i] > 0 ?
            LCR_GAME_UNIT - carTransform[3 + i] :
            (-1 * LCR_GAME_UNIT - carTransform[3 + i]));

      rotation[i] = (LCR_GAME_UNIT + (rotation[i] + (n * carTransform[3 + i])
        / LCR_SETTING_GHOST_STEP)) % LCR_GAME_UNIT;
    }
  }
#else
  for (int i = 0; i < 3; ++i)
  {
    position[i] = 0;
    rotation[i] = 0;
  }
#endif
}

/**
  Prepares ghost, computes position/rotation samples. When calling this, the
  correct replay and map have to be already loaded.
*/
void _LCR_gamePrepareGhost(void)
{
#if LCR_SETTING_GHOST_MAX_SAMPLES != 0 && LCR_SETTING_REPLAY_MAX_SIZE != 0
  LCR_GameUnit carTransform[6];
  LCR_LOG1("preparing ghost");
 
  LCR_gameResetRun(1,0);

  LCR_replayInitPlaying();

  uint8_t *currentSample = LCR_game.ghost.samples;

  LCR_game.ghost.stretch = 0;

  while (((int) LCR_racing.replay.achievedTime) >
    (LCR_SETTING_GHOST_STEP << LCR_game.ghost.stretch) *
    LCR_SETTING_GHOST_MAX_SAMPLES)
  {
    LCR_LOG1("stretching replay step");
    LCR_game.ghost.stretch++;
  }
  
  while (1)
  {
    if ((LCR_racing.tick % (LCR_SETTING_GHOST_STEP << LCR_game.ghost.stretch)
      == 0) || LCR_replayHasFinished())
    {
      LCR_racingGetCarTransform(carTransform,carTransform + 3,
        LCR_GAME_UNIT / 2);

      for (int i = 0; i < 3; ++i)
      {
        carTransform[i] += (LCR_MAP_SIZE_BLOCKS / 2) * (i == 1 ? LCR_GAME_UNIT
          / 2 : LCR_GAME_UNIT); // make non-negative

        // convert to 10 bit value:
        carTransform[i] = (carTransform[i] * 16) / LCR_GAME_UNIT;

        // conv. rotations to 4 bits, we rely on them being non-negative!
        carTransform[3 + i] = (carTransform[3 + i] * 16) / LCR_GAME_UNIT;
      }

      // format: XXXXXXXX YYYYYYYX ZZZZZYYY AAAAZZZZ CCCCBBBB

      currentSample[0] = carTransform[0] >> 1;
      currentSample[1] =
        ((carTransform[0] >> 9) & 0x01) | (carTransform[1] << 1);
      currentSample[2] =
        ((carTransform[1] >> 7) & 0x07) | ((carTransform[2] << 2) & 0xf8);
      currentSample[3] =
        ((carTransform[2] >> 6) & 0x0f) | (carTransform[3] << 4);
      currentSample[4] =
        (carTransform[4] & 0x0f) | (carTransform[5] << 4);

      currentSample += LCR_GHOST_SAMPLE_SIZE;

      if (LCR_replayHasFinished())
        break;
    }

    LCR_racingStep(0);
  }

  while ( // fill the rest with the last sample
    currentSample >= LCR_game.ghost.samples + LCR_GHOST_SAMPLE_SIZE &&
    currentSample < LCR_game.ghost.samples +
      LCR_SETTING_GHOST_MAX_SAMPLES * LCR_GHOST_SAMPLE_SIZE)
  {
    *currentSample = *(currentSample - LCR_GHOST_SAMPLE_SIZE);
    currentSample++;
  }

  LCR_gameGetNthGhostSample(0,carTransform,carTransform + 3);

  for (int i = 0; i < 3; ++i)
    LCR_game.ghost.offset[i] = (((((int) LCR_currentMap.startPos[i]) -
      LCR_MAP_SIZE_BLOCKS / 2)  * LCR_GAME_UNIT + LCR_GAME_UNIT / 2)
      / (i == 1 ? 2 : 1)) - carTransform[i];
#endif
}

LCR_GameUnit LCR_carSpeedKMH(void)
{
  return // we use 28/8 as an approximation of 3.6 to convers MPS to KMH
    (28 * LCR_SETTING_CMS_PER_BLOCK * LCR_racingGetCarSpeedUnsigned() *
    LCR_RACING_FPS) / (800 * LCR_GAME_UNIT);
}

/**
  Rewinds the global data file reading head to the beginning.
*/
void LCR_gameRewindDataFile(void)
{
  LCR_appendDataStr("");
  LCR_game.dataFile.state = 0;
}

/**
  Reads the next global data file character (merged internal data file
  with the optional user file). First the internal file will be read,
  immediately followed by the user file, then zero char will be returned and
  then reading starts over.
*/
char LCR_gameGetNextDataFileChar(void)
{
#if LCR_SETTING_ENABLE_DATA_FILE
  char c;

  if (LCR_game.dataFile.state < 0) // external file?
  {
    c = LCR_getNextDataFileChar();

    if (c == 0)
      LCR_game.dataFile.state = 0; // move to internal file next
  }
  else // internal file
  {
    c = LCR_internalDataFile[LCR_game.dataFile.state];
    LCR_game.dataFile.state++;

    if (c == 0)
    {
      LCR_appendDataStr("");
      c = LCR_getNextDataFileChar();
      LCR_game.dataFile.state = c ? -1 : 0; // trust this
    }
  }

  return c;
#else
  if (LCR_internalDataFile[LCR_game.dataFile.state] == 0)
  {
    LCR_game.dataFile.state = 0;
    return 0;
  }

  return LCR_internalDataFile[LCR_game.dataFile.state++];
#endif
}

/**
  Similar to LCR_gameGetNextDataFileChar, but returns 0 instead of the data
  string separator character. This function is meant to be used by functions
  that load something from a string while expecting a zero terminated string.
*/
char LCR_gameGetNextDataStrChar(void)
{
  char c = LCR_gameGetNextDataFileChar();
  return c != LCR_DATA_FILE_SEPARATOR ? c : 0;
}

unsigned int LCR_countData(char magicNumber)
{
  unsigned int result = 0;

  LCR_gameRewindDataFile();
  
  while (1)
  {
    char c = LCR_gameGetNextDataFileChar();

    if (c == magicNumber)
      result++;

    while (c != 0 && c != LCR_DATA_FILE_SEPARATOR)
      c = LCR_gameGetNextDataFileChar();

    if (c == 0)
      break;
  }

  return result;
}

/**
  Seeks to the Nth data string in the global data file, after the magic number,
  so that the name is now available for reading.
*/
void LCR_seekDataByIndex(unsigned int index, char magicNumber)
{
  char c;

  LCR_LOG0("seeking data string");

  LCR_gameRewindDataFile();

  do
  {
    c = LCR_gameGetNextDataFileChar();

    if (c == magicNumber)
    {
      if (index)
        index--;
      else
        return;
    }
    
    while (c != 0 && c != LCR_DATA_FILE_SEPARATOR)
      c = LCR_gameGetNextDataFileChar();

  } while (c);
}

uint8_t LCR_mapIsBeaten(const char *name)
{
  LCR_gameRewindDataFile();

  while (1)
  {
    char c = LCR_gameGetNextDataFileChar();

    if (c == 'B')
    {
      uint8_t i = 0;

      while (1)
      {
        c = LCR_gameGetNextDataFileChar();

        if (name[i] == 0)
        {
          if (c < ' ' || c == LCR_DATA_FILE_SEPARATOR ||
            c == LCR_DATA_FILE_SEPARATOR2 || c == 0)
            return 1;
          else
            break;
        }
        else if (c != name[i])
          break;

        i++;
      }
    }

    if (c == 0)
      break;
  }

  return 0;
}

uint8_t LCR_gameLoadMap(unsigned int mapIndex)
{
  uint8_t result;
  char name[LCR_MAP_NAME_MAX_LEN + 1];
  name[0] = 0;

  LCR_seekDataByIndex(mapIndex,'M');

  for (int i = 0; i < LCR_MAP_NAME_MAX_LEN; ++i)
  {
    char c = LCR_gameGetNextDataFileChar();

    if (c == LCR_DATA_FILE_SEPARATOR2 ||
      c == LCR_DATA_FILE_SEPARATOR || c == 0)
      break;

    name[i] = c;
    name[i + 1] = 0;
  }

  result = LCR_mapLoadFromStr(LCR_gameGetNextDataStrChar,name);
  LCR_game.mapBeaten = LCR_mapIsBeaten(LCR_currentMap.name);

  return result;
}

/**
  Loads replay by its index, returns index of a map for the replay (and the map
  will be loaded as with LCR_mapLoadFromStr) or -1 if the map wasn't found or -2
  if the replay couldn't be loaded or -3 if the replay is invalid. This function
  potentially reloads current map!
*/
int LCR_gameLoadReplay(unsigned int replayIndex)
{
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  uint32_t mapHash;
  uint16_t nameHash;
  char c;

  LCR_LOG1("loading replay and map");

  LCR_seekDataByIndex(replayIndex,'R');

  do // skip name
  {
    c = LCR_gameGetNextDataFileChar();
  }
  while (c && c != LCR_DATA_FILE_SEPARATOR2 &&
    c != LCR_DATA_FILE_SEPARATOR);

  if (!LCR_replayLoadFromStr(LCR_gameGetNextDataStrChar,
    &mapHash,&nameHash))
    return -2;

  // now try to find the map with given nameHash
  LCR_gameRewindDataFile();

  unsigned int skipTo = 0;

  while (1)
  {
    unsigned int mapIndex = 0;

    while (1) // find first skipToth map
    {
      c = LCR_gameGetNextDataFileChar();

      if (c == 0)
        return -1;
      else if (c == 'M')
      {
        if (mapIndex >= skipTo &&
          nameHash == _LCR_simpleStrHash(LCR_gameGetNextDataStrChar,';'))
        {
          LCR_LOG2("map name hash matches");
          
          if (LCR_gameLoadMap(mapIndex) && mapHash == LCR_currentMap.hash)
            return LCR_replayValidate() ? (int) mapIndex : -3;
          else
          {
            LCR_LOG2("bad map");
            // map hash doesn't match
            skipTo = mapIndex + 1;
            break;
          }
        }

        mapIndex++;
      }

      while (c != LCR_DATA_FILE_SEPARATOR)
      {
        if (c == 0)
          return -1;

        c = LCR_gameGetNextDataFileChar();
      }
    }
  }

  return 0;
#else
  return -2;
#endif
}

void LCR_gameEraseMenuItemNames(void)
{
  for (int i = 0; i < LCR_MENU_MAX_ITEMS; ++i)
    for (int j = 0; j < LCR_MENU_STRING_SIZE; ++j)
      LCR_game.menu.itemNames[i][j] = 0;

  LCR_game.menu.itemCount = 0;
}

void LCR_gameLoadMainMenuItems(void)
{
  for (int j = 0; j < 5; ++j)
    for (int i = 0; i < LCR_MENU_STRING_SIZE - 1; ++i)
    {
      LCR_game.menu.itemNames[j][i] = LCR_texts[LCR_TEXTS_MAIN_MENU + j][i];
      LCR_game.menu.itemNames[j][i + 1] = 0; 
    }

  LCR_game.menu.itemCount = 5;
}

#define LCR_GAME_DATA_FILE_BUFFER_SIZE 32

char _LCR_gameDataFileBuffer[LCR_GAME_DATA_FILE_BUFFER_SIZE];

/**
  Appends a single character to the data file WITH buffering, i.e. actual write
  is only performed when the buffer is filled OR when the character is a
  newline (which can be used to force a flush).
*/
void _LCR_gameDataCharWrite(char c)
{
  uint8_t i = 0;

  while (_LCR_gameDataFileBuffer[i])
  {
    if (i >= LCR_GAME_DATA_FILE_BUFFER_SIZE - 3)
    {
      _LCR_gameDataFileBuffer[i + 1] = c;
      LCR_appendDataStr(_LCR_gameDataFileBuffer);
      _LCR_gameDataFileBuffer[0] = 0;
      return;
    }

    i++;
  }

  _LCR_gameDataFileBuffer[i] = c;
  _LCR_gameDataFileBuffer[i + 1] = 0;

  if (c == 0 || c == '\n')
  {
    LCR_appendDataStr(_LCR_gameDataFileBuffer);
    _LCR_gameDataFileBuffer[0] = 0;
  }
}

/**
  Loads up to LCR_DATA_ITEM_CHUNK items of given type, starting at given
  index (among items of the same type). This will also load the menu item names.
*/
void LCR_gameLoadDataFileChunk(unsigned int startIndex, char magicNumber)
{
  char c;
  unsigned char state = 0; // 0: read magic num., >= 1: read name, 255: skip

  LCR_gameEraseMenuItemNames();

  LCR_game.dataFile.firstItemIndex = startIndex;
  LCR_game.dataFile.itemsTotal = 0;

  LCR_gameRewindDataFile();

  /* 3 iterations: in first we seek to the start index, in second we load the
    names, in third we just read the rest to get the total count. */
  for (int i = 0; i < 3; ++i)
  {
    while (1)
    {
      if (i == 0 && !startIndex)
        break;

      c = LCR_gameGetNextDataFileChar();

      if (c == 0)
        return;

      if (state == 0) // magic number
      {
        state = 255;

        if (c == magicNumber)
        {
          LCR_game.dataFile.itemsTotal++;

          if (i == 0)
            startIndex--;
          else if (i == 1)
            state = 1;
        }
      }
      else if (i == 1 && state != 255)
      {
        if (c == LCR_DATA_FILE_SEPARATOR || c == LCR_DATA_FILE_SEPARATOR2 ||
          (state >= 1 + LCR_MENU_STRING_SIZE - 1))
        {
          state = 255;

          LCR_game.menu.itemCount++;

          if (LCR_game.menu.itemCount >= LCR_DATA_ITEM_CHUNK)
            break;
        }
        else
          LCR_game.menu.itemNames[LCR_game.menu.itemCount][state - 1] = c;

        state++;
      }

      if (c == LCR_DATA_FILE_SEPARATOR)
        state = 0;
    }
  }
}

void LCR_gameInit(int argc, const char **argv)
{
  LCR_LOG0("initializing");

  for (int i = 0; i < LCR_KEYS_TOTAL; ++i)
    LCR_game.keyStates[i] = 0;

  LCR_rendererInit();
  LCR_racingInit();
  LCR_audioInit();

  LCR_game.dataFile.state = 0;

  for (int i = 0; i < LCR_MENU_MAX_ITEMS; ++i)
    LCR_game.menu.itemNamePtrs[i] = LCR_game.menu.itemNames[i];

  LCR_game.menu.selectedTab = 0;
  LCR_game.menu.selectedItem = 0;

  LCR_game.popupCountdown = 0;
  LCR_game.popupStr[LCR_POPUP_STR_SIZE - 1] = 0;

  LCR_game.frame = 0;
  LCR_game.musicOn = LCR_SETTING_MUSIC;
  LCR_game.nextRenderFrameTime = 0;
  LCR_game.nextRacingTickTime = 0;
  LCR_game.cameraMode = LCR_CAMERA_MODE_DRIVE;
  LCR_currentMap.blockCount = 0; // means no map loaded

#ifdef LCR_FPS_GET_MS
  LCR_game.renderFrameMS = 0;
  LCR_game.physicsFrameMS = 0;
#endif

  LCR_LOG2("parsing arguments");

  uint8_t quickLoad = 0;

  while (argc) // parse arguments
  {
    argc--;

    if (argv[argc][0] == '-')
      switch (argv[argc][1])
      {
        case 'c': LCR_game.cameraMode = (argv[argc][2] - '0') % 4; break;
        case 'm': LCR_game.musicOn = argv[argc][2] != '0'; break;
        case 's': LCR_audio.on = argv[argc][2] != '0'; break;
        case 'M': quickLoad = 1; break;
        case 'R': quickLoad = 2; break;
        case 'P': quickLoad = 3; break;

        default:
          LCR_LOG1("unknown argument"); 
          break;
      }
  }
    
  if (quickLoad == 1)
  {
    if (LCR_gameLoadMap(LCR_countData('M') - 1))
    {
      LCR_gameSetState(LCR_GAME_STATE_LOADING);
      LCR_game.menu.selectedTab = 1;
      LCR_gameLoadDataFileChunk(0,'M');
    }
    else
    {
      LCR_LOG0("couldn't load map");
      LCR_gameSetState(LCR_GAME_STATE_END);
    }
  }
  else if (quickLoad == 2 || quickLoad == 3)
  {
    LCR_gameLoadReplay(LCR_countData('R') - 1);
    LCR_gameSetState(LCR_GAME_STATE_LOADING);
    LCR_game.menu.selectedTab = quickLoad;
    LCR_gameLoadDataFileChunk(0,'R');
  }
  else
  {
    LCR_gameLoadMainMenuItems();
    LCR_gameSetState(LCR_GAME_STATE_MENU);
  }
}

/**
  Assumes maps are loaded in menu items, checks (in the resource file) which
  ones have been marked as beaten and marks corresponding menu items as such.
*/
void LCR_checkBeatenMaps(void)
{
  LCR_LOG2("checking beaten maps");

  char name[LCR_MAP_NAME_MAX_LEN + 1];
  LCR_gameRewindDataFile();

  while (1)
  {
    char c = LCR_gameGetNextDataFileChar();

    if (c == 'B')
    {
      uint8_t i = 0;

      while (i < LCR_MAP_NAME_MAX_LEN + 1)
      {
        c = LCR_gameGetNextDataFileChar();

        if (c < ' ' || c == LCR_DATA_FILE_SEPARATOR ||
          c == LCR_DATA_FILE_SEPARATOR2)
          break;

        name[i] = c;

        i++;
      }

      name[i] = 0;

      for (uint8_t j = 0; j < LCR_game.menu.itemCount; ++j)
        if (_LCR_strCmp(name,LCR_game.menu.itemNamePtrs[j]))
        {
          for (uint8_t k = 0; k < LCR_MENU_STRING_SIZE; ++k)
            if (LCR_game.menu.itemNames[j][k] == 0)
            {
              LCR_game.menu.itemNames[j]
                [k - (k == LCR_MENU_STRING_SIZE - 1)] = '#';
              LCR_game.menu.itemNames[j]
                [k + (k != LCR_MENU_STRING_SIZE - 1)] = 0;
              break;
            }
        }
    }
    else
      while (c != 0 && c != LCR_DATA_FILE_SEPARATOR)
        c = LCR_gameGetNextDataFileChar();

    if (c == 0)
      break;
  }
}

void LCR_gameEnd(void)
{
  LCR_LOG0("ending");
}

void LCR_gameTimeToStr(uint32_t timeMS, char *str)
{
  str[9] = 0;
  str[8] = '0' + timeMS % 10; // milliseconds
  timeMS /= 10;
  str[7] = '0' + timeMS % 10;
  timeMS /= 10;
  str[6] = '0' + timeMS % 10;
  timeMS /= 10;
  str[5] = '\'';
  str[4] = '0' + timeMS % 10; // seconds
  timeMS /= 10;
  str[3] = '0' + timeMS % 6;
  str[2] = '\'';
  timeMS /= 6;
  str[1] = '0' + timeMS % 10; // minutes
  timeMS /= 10;
  str[0] = '0' + timeMS % 10;
}
 
void LCR_gameDrawPopupMessage(void)
{
#define _TEXT_SIZE 1 + 4 * (LCR_EFFECTIVE_RESOLUTION_Y > 96)
#define _OFFSET_V (LCR_EFFECTIVE_RESOLUTION_Y / 16)

  int textH = LCR_rendererComputeTextHeight(_TEXT_SIZE);
  int textW = LCR_rendererComputeTextWidth(LCR_game.popupStr,_TEXT_SIZE);

  LCR_rendererDrawRect((LCR_EFFECTIVE_RESOLUTION_X - textW - 2 * LCR_GUI_GAP)
    / 2,_OFFSET_V,textW + 2 * LCR_GUI_GAP,textH + 2 * LCR_GUI_GAP,
    LCR_CONVERT_COLOR(0xce59),0);

  LCR_rendererDrawText(LCR_game.popupStr,(LCR_EFFECTIVE_RESOLUTION_X - textW)
    / 2,_OFFSET_V + LCR_GUI_GAP,LCR_CONVERT_COLOR(0x0300),_TEXT_SIZE);

#undef _OFFSET_V
#undef _TEXT_SIZE
}

void LCR_gameDraw3DView(void)
{
  LCR_GameUnit carTransform[6];

  LCR_GameUnit physicsInterpolationParam =
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
    !(LCR_racing.replay.on && LCR_replayHasFinished())
#else
    1
#endif
    ? LCR_GAME_UNIT -
        (((int) (LCR_game.nextRacingTickTime - LCR_game.time)) * LCR_GAME_UNIT)
        / LCR_RACING_TICK_MS_RT // 32: magic constant
      : _LCR_min(LCR_GAME_UNIT,32 * ((int) (LCR_game.time -
        LCR_game.stateStartTime)));

  LCR_racingGetCarTransform(carTransform,carTransform + 3,
    physicsInterpolationParam);

  LCR_rendererSetCarTransform(carTransform,carTransform + 3);

#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
  if (LCR_game.ghost.active && LCR_game.state != LCR_GAME_STATE_RUN_STARTING)
  {
    LCR_GameUnit carTransform2[3];

    LCR_rendererSetGhostVisibility(1);

    LCR_gameGhostGetTransform(LCR_racing.tick + 1,carTransform2,
      carTransform + 3);
    LCR_gameGhostGetTransform(LCR_racing.tick,carTransform,carTransform + 3);

    for (int i = 0; i < 3; ++i)
      carTransform[i] += ((carTransform2[i] - carTransform[i]) *
        physicsInterpolationParam) / LCR_GAME_UNIT;

    LCR_rendererSetGhostTransform(carTransform,carTransform + 3);
  }
  else
#endif
    LCR_rendererSetGhostVisibility(0);

  if (LCR_game.cameraMode != LCR_CAMERA_MODE_FREE &&
    LCR_game.state != LCR_GAME_STATE_RUN_FINISHED)
    LCR_rendererCameraFollow(
      (LCR_game.cameraMode != LCR_CAMERA_MODE_INSIDE) +
      (LCR_game.cameraMode == LCR_CAMERA_MODE_DRIVE2));

#if LCR_ANIMATE_CAR
  LCR_rendererSetWheelState(LCR_racingGetWheelRotation(),
    LCR_racingGetWheelSteer() * 2);
#endif

  LCR_rendererDraw3D();

#if LCR_SETTING_DEBUG_PHYSICS_DRAW
  LCR_GameUnit camTr[7];
  LCR_rendererGetCameraTransform(camTr,camTr + 3,camTr + 6);
  LCR_physicsDebugDraw(camTr,camTr + 3,camTr[6],_LCR_physicdDebugDrawPixel);
#endif

#if LCR_SETTING_DISPLAY_HUD
  // GUI/HUD:

  char str[10];
  int val = LCR_carSpeedKMH();

  if (val < 5) // don't show tiny oscillations when still
    val = 0;

  str[0] = val >= 100 ? '0' + (val / 100) % 10  : ' ';
  str[1] = val >= 10 ? '0' + (val / 10) % 10  : ' ';
  str[2] = '0' + val % 10;
  str[3] = 0;

  #define _FONT_SIZE (1 + (LCR_EFFECTIVE_RESOLUTION_Y > 96))

  LCR_rendererDrawText(str,LCR_EFFECTIVE_RESOLUTION_X - // speed (bot., right)
    LCR_rendererComputeTextWidth(str,_FONT_SIZE) - LCR_GUI_GAP,
    LCR_EFFECTIVE_RESOLUTION_Y - LCR_rendererComputeTextHeight(_FONT_SIZE) -
    LCR_GUI_GAP,0,_FONT_SIZE); 

  str[0] = (LCR_racing.currentInputs & LCR_RACING_INPUT_LEFT)  ? 'L' : '.';
  str[1] = (LCR_racing.currentInputs & LCR_RACING_INPUT_BACK)  ? 'D' : '.';
  str[2] = (LCR_racing.currentInputs & LCR_RACING_INPUT_FORW)  ? 'U' : '.';
  str[3] = (LCR_racing.currentInputs & LCR_RACING_INPUT_RIGHT) ? 'R' : '.';
  str[4] = 0;
  
  LCR_rendererDrawText(str,LCR_EFFECTIVE_RESOLUTION_X -
    LCR_rendererComputeTextWidth(str,_FONT_SIZE) - LCR_GUI_GAP,
    LCR_EFFECTIVE_RESOLUTION_Y - 2 *
    (LCR_rendererComputeTextHeight(_FONT_SIZE) + LCR_GUI_GAP),0,_FONT_SIZE);

  LCR_gameTimeToStr(LCR_game.runTime * LCR_RACING_TICK_MS,str);

  if (LCR_game.state != LCR_GAME_STATE_RUN_FINISHED)
    LCR_rendererDrawText(str,LCR_GUI_GAP,LCR_EFFECTIVE_RESOLUTION_Y -
    2 * (LCR_rendererComputeTextHeight(_FONT_SIZE) + LCR_GUI_GAP),0,_FONT_SIZE);
  else
    LCR_rendererDrawText(str,(LCR_EFFECTIVE_RESOLUTION_X -
      LCR_rendererComputeTextWidth(str,2 * _FONT_SIZE)) / 2,
      LCR_EFFECTIVE_RESOLUTION_Y / 2,
      LCR_game.runTime <= LCR_currentMap.targetTime ?
        LCR_CONVERT_COLOR(0x0700) : LCR_CONVERT_COLOR(0x4208),2 * _FONT_SIZE);

  LCR_gameTimeToStr(LCR_currentMap.targetTime * LCR_RACING_TICK_MS,str);

  LCR_rendererDrawText(str,LCR_GUI_GAP,LCR_EFFECTIVE_RESOLUTION_Y -
    LCR_rendererComputeTextHeight(_FONT_SIZE) - LCR_GUI_GAP,
    LCR_CONVERT_COLOR(0x4208),_FONT_SIZE);
#undef _FONT_SIZE
#endif
}

void LCR_gameSaveReplay(void)
{
  char str[10];

  LCR_LOG0("saving replay");

#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  _LCR_gameDataCharWrite(LCR_DATA_FILE_SEPARATOR);
  _LCR_gameDataCharWrite('R');

  for (int i = 0; i < LCR_MAP_NAME_MAX_LEN; ++i)
    if (LCR_currentMap.name[i])
      _LCR_gameDataCharWrite(LCR_currentMap.name[i]);
    else
      break;

  _LCR_gameDataCharWrite(' ');

  LCR_gameTimeToStr(LCR_timeTicksToMS(LCR_game.runTime),str);

  for (int i = (str[0] == '0' && str[1] == '0' ? 3 : 0);
    i < LCR_MAP_NAME_MAX_LEN; ++i)
    if (str[i])
      _LCR_gameDataCharWrite(str[i]);
    else
      break;
 
  _LCR_gameDataCharWrite(';');

  LCR_replayOutputStr(_LCR_gameDataCharWrite);
  LCR_gamePopupMessage(LCR_texts[LCR_TEXTS_SAVED]);
#endif
}

/**
  Checks if given key is either immediately pressed or repeated after being
  held for some time.
*/
uint8_t LCR_gameKeyActive(uint8_t key)
{
  return LCR_game.keyStates[key] == 1 ||
    (LCR_game.keyStates[key] >= (1200 / LCR_SETTING_FPS)
      && ((LCR_game.frame & 0x03) == 0));
}

/**
  Helper subroutine, handles user input during main loop frame, EXCEPT for the
  driving input (that is handled in the loop itself).
*/
void LCR_gameHandleInput(void)
{
  int tabSwitchedTo = -1;
  int scrolled = 0;

  if (LCR_game.state != LCR_GAME_STATE_MENU)
  {
    if (LCR_game.state == LCR_GAME_STATE_RUN_FINISHED)
    {
      if (LCR_game.keyStates[LCR_KEY_A] == 1)
      {
        if (LCR_game.runTime <= LCR_currentMap.targetTime
#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
          && !LCR_game.ghost.active
#endif
          )
        {
          LCR_LOG1("setting new target time");
          LCR_currentMap.targetTime = LCR_game.runTime;
        }

        LCR_gameResetRun(
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
          LCR_racing.replay.on,
#else
          0,
#endif

#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
          LCR_game.ghost.active
#else
          0
#endif
        );
      }
    }
    else if (LCR_game.state == LCR_GAME_STATE_RUN_STARTING)
    {
      if (LCR_game.time - LCR_game.stateStartTime
        >= LCR_SETTING_COUNTDOWN_MS)
      {
        LCR_gameSetState(LCR_GAME_STATE_RUN);      
        LCR_gamePopupMessage("");
      }
      else
        LCR_gamePopupNumber(1 + (LCR_SETTING_COUNTDOWN_MS -
          (LCR_game.time - LCR_game.stateStartTime)) / 1000);
    }

    if (LCR_game.keyStates[LCR_KEY_B] == 1)
    {
      LCR_LOG1("menu open");
      LCR_gamePopupMessage("");
      LCR_gameSetState(LCR_GAME_STATE_MENU);
      LCR_game.menu.selectedItem = 0;
      LCR_game.menu.selectedTab = 0;
      LCR_gameLoadMainMenuItems();
    }
    else if (LCR_game.cameraMode == LCR_CAMERA_MODE_FREE)
    {
      LCR_GameUnit offsets[5];

      for (int i = 0; i < 5; ++i)
        offsets[i] = 0;

      if (LCR_game.keyStates[LCR_KEY_A])
      {
        if (LCR_game.keyStates[LCR_KEY_UP])
          offsets[4] = LCR_FREE_CAMERA_TURN_STEP;
        else if (LCR_game.keyStates[LCR_KEY_DOWN])
          offsets[4] -= LCR_FREE_CAMERA_TURN_STEP;

        if (LCR_game.keyStates[LCR_KEY_RIGHT])
          offsets[3] -= LCR_FREE_CAMERA_TURN_STEP;
        else if (LCR_game.keyStates[LCR_KEY_LEFT])
          offsets[3] = LCR_FREE_CAMERA_TURN_STEP;
      }
      else
      {
        if (LCR_game.keyStates[LCR_KEY_UP])
          offsets[0] = LCR_FREE_CAMERA_STEP;
        else if (LCR_game.keyStates[LCR_KEY_DOWN])
          offsets[0] -= LCR_FREE_CAMERA_STEP;

        if (LCR_game.keyStates[LCR_KEY_RIGHT])
          offsets[1] = LCR_FREE_CAMERA_STEP;
        else if (LCR_game.keyStates[LCR_KEY_LEFT])
          offsets[1] -= LCR_FREE_CAMERA_STEP;
      }

      LCR_rendererMoveCamera(offsets,offsets + 3);
    }
    else if (LCR_game.keyStates[LCR_KEY_A] == 1)
      LCR_gameResetRun(
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
        LCR_racing.replay.on,
#else
        0,
#endif

#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
        LCR_game.ghost.active
#else
        0
#endif
        );
  }
  else // LCR_GAME_STATE_MENU
  {
    if (LCR_game.keyStates[LCR_KEY_RIGHT] == 1)
    {
      LCR_LOG1("menu tab right");
      LCR_game.menu.selectedTab =
        (LCR_game.menu.selectedTab + 1) % LCR_MENU_TABS;
      tabSwitchedTo = LCR_game.menu.selectedTab;
      LCR_game.menu.selectedItem = 0;
      LCR_audioPlaySound(LCR_SOUND_CLICK);
    }
    else if (LCR_game.keyStates[LCR_KEY_LEFT] == 1)
    {
      LCR_LOG1("menu tab left");
      LCR_game.menu.selectedTab =
        (LCR_game.menu.selectedTab + LCR_MENU_TABS - 1) % LCR_MENU_TABS;
      tabSwitchedTo = LCR_game.menu.selectedTab;
      LCR_game.menu.selectedItem = 0;
      LCR_audioPlaySound(LCR_SOUND_CLICK);
    }
    else if (LCR_gameKeyActive(LCR_KEY_UP))
    {
      LCR_LOG1("menu item up");

      if (LCR_game.menu.selectedItem != 0)
      {
        LCR_game.menu.selectedItem--;
        LCR_audioPlaySound(LCR_SOUND_CLICK);
      }
      else if (LCR_game.menu.selectedTab != 0 &&
        LCR_game.dataFile.firstItemIndex != 0)
      {
        LCR_game.menu.selectedItem = LCR_DATA_ITEM_CHUNK - 1;
        LCR_audioPlaySound(LCR_SOUND_CLICK);
        scrolled = -1;
      }
    }
    else if (LCR_gameKeyActive(LCR_KEY_DOWN))
    {
      LCR_LOG1("menu item down");

      if (LCR_game.menu.selectedTab == 0)
      {
        if (LCR_game.menu.selectedItem < 4)
        {
          LCR_game.menu.selectedItem++;
          LCR_audioPlaySound(LCR_SOUND_CLICK);
        }
      }
      else if (LCR_game.menu.selectedItem < LCR_game.menu.itemCount - 1)
      {
        LCR_game.menu.selectedItem++;
        LCR_audioPlaySound(LCR_SOUND_CLICK);
      }
      else if (LCR_game.dataFile.firstItemIndex +
        LCR_DATA_ITEM_CHUNK < LCR_game.dataFile.itemsTotal)
      {
        LCR_game.menu.selectedItem = 0;
        LCR_audioPlaySound(LCR_SOUND_CLICK);
        scrolled = 1;
      }
    }
    else if (LCR_game.keyStates[LCR_KEY_B] == 1 && LCR_currentMap.blockCount)
    {
      LCR_LOG1("menu closed");
      LCR_rendererLoadMapChunks();
      LCR_gameSetState(LCR_game.statePrev);
    }
    else if (LCR_game.keyStates[LCR_KEY_A] == 1)
    {
      LCR_LOG1("menu confirm");
      LCR_audioPlaySound(LCR_SOUND_CLICK);

      switch (LCR_game.menu.selectedTab)
      {
        case 0:
          switch (LCR_game.menu.selectedItem)
          {
            case 0:
              LCR_game.cameraMode = (LCR_game.cameraMode + 1) % 4;
              LCR_rendererSetCarVisibility(
                LCR_game.cameraMode != LCR_CAMERA_MODE_INSIDE);
              LCR_rendererCameraReset();
              LCR_gamePopupNumber(LCR_game.cameraMode);
              break;

#if LCR_SETTING_MUSIC
            case 1:
              LCR_game.musicOn = !LCR_game.musicOn;
              LCR_gamePopupNumber(LCR_game.musicOn);
              break;
#endif
            case 2:
              LCR_audio.on = !LCR_audio.on;
              LCR_gamePopupNumber(LCR_audio.on);
              break;

            case 3:
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
              if (LCR_game.statePrev == LCR_GAME_STATE_RUN_FINISHED &&
                !LCR_racing.replay.on)
                LCR_gameSaveReplay();
              else
#endif
                LCR_gamePopupMessage(LCR_texts[LCR_TEXTS_FAIL]);

              break;

            case 4:
              LCR_gameSetState(LCR_GAME_STATE_END);
              break;

            default: break;
          }

          LCR_gameLoadMainMenuItems();
          break;

        case 1: // maps
          if (LCR_gameLoadMap(LCR_game.dataFile.firstItemIndex +
            LCR_game.menu.selectedItem) && LCR_currentMap.blockCount != 0)
            LCR_gameSetState(LCR_GAME_STATE_LOADING);
          else
          {
            LCR_LOG0("couldn't load map");
            LCR_gamePopupMessage(LCR_texts[LCR_TEXTS_FAIL]);
          }

          break;

        case 2: // view replay
        case 3: // play against replay
        {
          int mapIndex = LCR_gameLoadReplay(LCR_game.dataFile.firstItemIndex +
            LCR_game.menu.selectedItem);

          if (mapIndex < -1)
          {
            LCR_LOG1("couldn't load replay");
            LCR_gamePopupMessage(LCR_texts[LCR_TEXTS_FAIL]);
            LCR_currentMap.blockCount = 0;
            LCR_gameSetState(LCR_GAME_STATE_MENU);
          }
          else if (mapIndex == -1)
          {
            LCR_LOG1("couldn't load replay map");
            LCR_gamePopupMessage(LCR_texts[LCR_TEXTS_FAIL]);
            LCR_currentMap.blockCount = 0;
            LCR_gameSetState(LCR_GAME_STATE_MENU);
          }
          else
            LCR_gameSetState(LCR_GAME_STATE_LOADING);

          break;
        }

        default: break;
      }
    }
  }

  if (tabSwitchedTo == 0)
    LCR_gameLoadMainMenuItems();
  else if (tabSwitchedTo > 0 || scrolled != 0)
  {
    LCR_gameLoadDataFileChunk(
      (tabSwitchedTo > 0) ? 0 : (LCR_game.dataFile.firstItemIndex +
        scrolled * LCR_DATA_ITEM_CHUNK),
      LCR_game.menu.selectedTab == 1 ? 'M' : 'R');

    if (LCR_game.menu.selectedTab == 1)
      LCR_checkBeatenMaps();
  }
}

uint8_t LCR_gameStep(uint32_t time)
{
  LCR_LOG2("game step (start)");

#ifdef LCR_FPS_GET_MS
  uint32_t frameTime;
#endif

  uint32_t sleep = 0;

  LCR_game.time = time;

  if (LCR_game.state == LCR_GAME_STATE_END)
    return 0;

  for (int i = 0; i < LCR_KEYS_TOTAL; ++i)
    LCR_game.keyStates[i] = LCR_keyPressed(i) ?
      (LCR_game.keyStates[i] < 255 ? LCR_game.keyStates[i] + 1 : 255) : 0;
 
  if (LCR_game.state == LCR_GAME_STATE_LOADING)
  {
    LCR_rendererLoadMap();

    if (LCR_game.menu.selectedTab == 3)
    {
      _LCR_gamePrepareGhost();
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
      LCR_currentMap.targetTime = LCR_racing.replay.achievedTime;
#endif
    }

    LCR_gameSetState(LCR_GAME_STATE_LOADED);
  }
  else if (LCR_game.state == LCR_GAME_STATE_LOADED)
    LCR_gameResetRun( // start countdown now, loading the map lost many frames
      LCR_game.menu.selectedTab == 2,
      LCR_game.menu.selectedTab == 3);
  else
  {
    LCR_gameHandleInput();
 
    int paused = 
      LCR_game.state != LCR_GAME_STATE_RUN &&
      LCR_game.state != LCR_GAME_STATE_RUN_FINISHED;

    // handle simulation:
    while (time >= LCR_game.nextRacingTickTime)
    {
      LCR_LOG2("gonna step racing engine");
      unsigned int input = 
        (LCR_game.cameraMode == LCR_CAMERA_MODE_FREE ||
         LCR_game.state == LCR_GAME_STATE_RUN_FINISHED) ? 0 :
        ((LCR_game.keyStates[LCR_KEY_UP]    ? LCR_RACING_INPUT_FORW  : 0) |
         (LCR_game.keyStates[LCR_KEY_RIGHT] ? LCR_RACING_INPUT_RIGHT : 0) |
         (LCR_game.keyStates[LCR_KEY_DOWN]  ? LCR_RACING_INPUT_BACK  : 0) |
         (LCR_game.keyStates[LCR_KEY_LEFT]  ? LCR_RACING_INPUT_LEFT  : 0));

#ifdef LCR_FPS_GET_MS
      frameTime = LCR_FPS_GET_MS;
#endif

      uint32_t events = paused ? 0 : LCR_racingStep(input);

#if LCR_SETTING_PARTICLES
      LCR_rendererSetParticles(0);

      if (LCR_racingGetCarSpeedUnsigned() > 60)
      {
        if (LCR_racingCurrentGroundMaterial() == LCR_BLOCK_MATERIAL_GRASS)
          LCR_rendererSetParticles(
            LCR_CONVERT_COLOR(LCR_SETTING_PARTICLE_COLOR_GRASS));
        else if (LCR_racingCurrentGroundMaterial() == LCR_BLOCK_MATERIAL_DIRT)
          LCR_rendererSetParticles(
            LCR_CONVERT_COLOR(LCR_SETTING_PARTICLE_COLOR_DIRT));
        else if (LCR_racingCarIsDrifting())
          LCR_rendererSetParticles(
            LCR_CONVERT_COLOR(LCR_SETTING_PARTICLE_COLOR_DRIFT));
      }
#endif

#ifdef LCR_FPS_GET_MS
      LCR_game.physicsFrameMS += LCR_FPS_GET_MS - frameTime;
      LCR_game.physicsFramesMeasured++;
#endif

      if (events & LCR_RACING_EVENT_CP_TAKEN)
      {
        int carBlock[3];
        char str[10];

        LCR_LOG1("CP taken");
        LCR_racingGetCarBlockCoords(carBlock);
        LCR_rendererMarkTakenCP(carBlock[0],carBlock[1],carBlock[2]);
        LCR_audioPlaySound(LCR_SOUND_CLICK);
        LCR_gameTimeToStr(LCR_timeTicksToMS(LCR_game.runTime),str);
        LCR_gamePopupMessage(str);
      }
      else if (events & LCR_RACING_EVENT_FINISHED &&
        LCR_game.state != LCR_GAME_STATE_RUN_FINISHED)
      {
        LCR_LOG1("finished, time:");
        LCR_LOG1_NUM(LCR_game.runTime);

        if (LCR_game.runTime <= LCR_currentMap.targetTime
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
          && !LCR_racing.replay.on
#endif
          )
        {
          LCR_gameSaveReplay();

          if (!LCR_game.mapBeaten &&
#if LCR_SETTING_GHOST_MAX_SAMPLES != 0
            !LCR_game.ghost.active &&
#endif

#if LCR_SETTING_REPLAY_MAX_SIZE != 0
            !LCR_racing.replay.on
#else
            1
#endif
            )
          {
            LCR_LOG1("map beaten");
            LCR_game.mapBeaten = 1;

            // record that the map is beaten in the data file:
            _LCR_gameDataCharWrite(LCR_DATA_FILE_SEPARATOR);
            _LCR_gameDataCharWrite('B');

            for (int i = 0; i < LCR_MAP_NAME_MAX_LEN; ++i)
              if (LCR_currentMap.name[i])
                _LCR_gameDataCharWrite(LCR_currentMap.name[i]);
              else
                break;

            _LCR_gameDataCharWrite(';');
            _LCR_gameDataCharWrite('\n');
          }
        }

        LCR_audioPlaySound(LCR_SOUND_CLICK);
        LCR_gameSetState(LCR_GAME_STATE_RUN_FINISHED);
      }

      if (events & LCR_RACING_EVENT_CRASH_SMALL)
      {
        LCR_audioPlaySound(LCR_SOUND_CRASH_SMALL);
        LCR_LOG2("crash (small)");
      }
      else if (events & LCR_RACING_EVENT_CRASH_BIG)
      {
        LCR_audioPlaySound(LCR_SOUND_CRASH_BIG);
        LCR_LOG2("crash (big)");
      }
      else if (events & LCR_RACING_EVENT_ACCELERATOR)
        LCR_audioPlaySoundIfFree(LCR_SOUND_ACCELERATOR);    
      else if (events & LCR_RACING_EVENT_FAN)
        LCR_audioPlaySoundIfFree(LCR_SOUND_FAN);

      int engineIntensity = LCR_carSpeedKMH() * 2;

      if (LCR_game.state == LCR_GAME_STATE_RUN_FINISHED)
      {
        engineIntensity -= LCR_game.time - LCR_game.stateStartTime;

        if (engineIntensity < 0)
          engineIntensity = 0;
      }

      LCR_audioSetEngineIntensity(paused ? 0 :
        (engineIntensity < 256 ? engineIntensity : 255));

      if (LCR_game.state == LCR_GAME_STATE_RUN ||                                     
        LCR_game.state == LCR_GAME_STATE_RUN_STARTING)                                
        LCR_game.runTime = LCR_racing.tick;

      LCR_game.nextRacingTickTime += LCR_RACING_TICK_MS_RT; 
    }

    // handle rendering:
    if (time >= LCR_game.nextRenderFrameTime ||
      LCR_game.state == LCR_GAME_STATE_LOADING || 1 /*pvmk*/)
    {
      LCR_LOG2("rendering next frame");

      while (time >= LCR_game.nextRenderFrameTime)
        LCR_game.nextRenderFrameTime += 1000 / LCR_SETTING_FPS;

#ifdef LCR_FPS_GET_MS
      frameTime = LCR_FPS_GET_MS;
#endif

      if ((LCR_game.state == LCR_GAME_STATE_MENU) || 
        LCR_game.state == LCR_GAME_STATE_LOADING)
        LCR_rendererDrawMenu(LCR_texts[LCR_TEXTS_TABS
          + LCR_game.menu.selectedTab],LCR_game.menu.itemNamePtrs,
          LCR_game.menu.itemCount,LCR_game.menu.selectedItem,
          (LCR_game.menu.selectedTab > 0) &&
          LCR_game.dataFile.firstItemIndex + LCR_DATA_ITEM_CHUNK
          < LCR_game.dataFile.itemsTotal);
      else
        LCR_gameDraw3DView(); 

      if (LCR_game.popupCountdown)
      {
        LCR_gameDrawPopupMessage();
        LCR_game.popupCountdown--;
      }

#ifdef LCR_FPS_GET_MS
      LCR_game.renderFrameMS += LCR_FPS_GET_MS - frameTime;
      LCR_game.renderFramesMeasured++;
#endif
    }

    if (LCR_game.state == LCR_GAME_STATE_LOADING)
    {
      // show the "loading" screen

      LCR_gamePopupMessage(LCR_texts[LCR_TEXTS_LOADING]);
      LCR_game.popupCountdown = 2;
      LCR_gameDrawPopupMessage();
    }

    if (LCR_game.nextRacingTickTime > time)
      sleep = LCR_game.nextRenderFrameTime - time;

    if (LCR_game.nextRenderFrameTime > time &&
      LCR_game.nextRenderFrameTime - time < sleep)
      sleep = LCR_game.nextRenderFrameTime - time;

    sleep = (sleep * 3) / 4;

    if (sleep)
      LCR_sleep(sleep);
    else
    {
      LCR_LOG2("can't sleep");
    }

    LCR_game.frame++;
    LCR_LOG2("game step (end)");

#ifdef LCR_FPS_GET_MS
    if (LCR_game.renderFramesMeasured >= 64)
    {
      LCR_LOG0("us/frame (render):");
      LCR_LOG0_NUM((LCR_game.renderFrameMS * 1000) / 64);
      LCR_game.renderFramesMeasured = 0;
      LCR_game.renderFrameMS = 0;
    }

    if (LCR_game.physicsFramesMeasured >= 64)
    {
      LCR_LOG0("us/frame (physics):");
      LCR_LOG0_NUM((LCR_game.physicsFrameMS * 1000) / 64);
      LCR_game.physicsFramesMeasured = 0;
      LCR_game.physicsFrameMS = 0;
    }
#endif
  }

  return 1;
}

uint8_t LCR_gameGetNextAudioSample(void)
{
  return LCR_audioGetNextSample();
}
  
#undef LCR_MODULE_NAME
#endif // guard
