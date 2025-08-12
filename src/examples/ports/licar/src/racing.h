#ifndef _LCR_RACING_H
#define _LCR_RACING_H

/** @file racing.h

  Licar: racing module
  
  This file implements the racing physics and logic as well as replays and other
  stuff related to racing itself. It's possible to use this module alone if one
  wants to implement a program that doesn't need graphics, I/O etc.

  Some comments:

  - This module uses tinyphysicsengine (TPE), a small and simple physics engine
    that uses "balls and springs" to model bodies (here the car) and kind of
    "signed distance functions" to model the static environment (the map). The
    car is strictly a soft body, but it's very "stiff" so that it behaves almost
    like a rigid body. The car body consists of 5 joints (4 wheels and the
    body), the top down views looks like this:

         front
       (2)---(3) r
     l  |\   /|  i
     e  | (4) |  g
     f  |/   \|  h
     t (0)---(1) t
         rear

  - Replays are internally stored as follows: the replay consists of 16 bit
    words representing changes in input at specific frame. In lowest 4 bits the
    new input state is recorded, the remaining 12 bits record physics frame
    offset against the previous input change. If the 12 bits don't suffice
    because the offset is too big (input didn't change for more than 2^12
    ticks), there must simply be inserted an extra word that just copies the
    current input state.
  - Replay text format: first there are two characters saying the physics
    engine version, then immediately the name of the map terminated by ';', then
    hexadecimal hash of the map follows (exactly 8 characters), then blank
    character follows, then achieved time as a series of decimal digits
    expressing the physics tick at which the run finished, then the replay data,
    i.e. the series of 16 bit words in hexadecimal, each preceded by ':'. The
    events (but nothing else) may otherwise be preceded or followed by other
    characters (possible comments). All hexadecimal letters must be lowercase.
    The word 00000000 may optionally be used to terminate the replay, the rest
    of the string will be ignored.
  - Physics engine version: LCR_RACING_VERSION1 and LCR_RACING_VERSION2 define
    a two-character version string of this module that determines compatibility
    of replays. Whenever a change is made to this module that changes the
    behavior of physics, the version string must be changed because replays will
    stop being compatible. It's still possible to make other changes to this
    module (such as optimizations or comments) without having to change physics
    version.
*/

#include <stdint.h>

typedef int32_t LCR_GameUnit;        ///< Abstract game unit.

#define LCR_RACING_FPS 30            /**< Physics FPS, i.e. the number of
                                          physics ticks per second. */

#define LCR_RACING_TICK_MS (1000 / LCR_RACING_FPS)

#define LCR_RACING_TICK_MS_RT \
  (100000 / (LCR_RACING_FPS * LCR_SETTING_TIME_MULTIPLIER))

#define LCR_RACING_VERSION1 '0'      ///< First part of physics eng. version.
#define LCR_RACING_VERSION2 '0'      ///< Second part of physics eng. version.

#define LCR_GAME_UNIT 2048           ///< Length of map square in LCR_GameUnits.

#define LCR_RACING_INPUT_FORW        0x01
#define LCR_RACING_INPUT_RIGHT       0x02
#define LCR_RACING_INPUT_BACK        0x04
#define LCR_RACING_INPUT_LEFT        0x08

#define LCR_RACING_EVENT_CP_TAKEN    0x0001
#define LCR_RACING_EVENT_FINISHED    0x0002
#define LCR_RACING_EVENT_CRASH_SMALL 0x0004
#define LCR_RACING_EVENT_CRASH_BIG   0x0008
#define LCR_RACING_EVENT_ACCELERATOR 0x0010
#define LCR_RACING_EVENT_FAN         0x0020

#define LCR_PHYSICS_UNIT             4096 ///< Len. of square for phys. engine.

/*
  The combination of values TPE_RESHAPE_TENSION_LIMIT and TPE_RESHAPE_ITERATIONS
  has crucial effect on the bugginess and feel of car physics, especially when
  driving onto ramps in high speed etc., the values were chosen empirically by
  testing, these are the ones that showed to subjectively feel the best.
*/

#define TPE_RESHAPE_TENSION_LIMIT    7
#define TPE_RESHAPE_ITERATIONS       18

#include "general.h"
#include "map.h"
#include "tinyphysicsengine.h"

#ifdef LCR_MODULE_NAME
  #undef LCR_MODULE_NAME
#endif

#define LCR_MODULE_NAME "race"

/*
  What follows are carefully chosen and tuned physics constants, don't change
  them unless with a very good reason, as replay compatibility will be broken
  and bugs may start to appear. If physics changes, LCR_RACING_VERSION1 and
  LCR_RACING_VERSION2 must be changed to indicate the change.
*/
#define LCR_GRAVITY (LCR_PHYSICS_UNIT / 160)
#define LCR_FAN_FORCE 4
#define LCR_FAN_FORCE_DECREASE 6

#define LCR_CAR_FORWARD_FRICTION (TPE_F / 180)
#define LCR_CAR_AIR_FRICTION 32
#define LCR_CAR_STAND_FRICTION_MULTIPLIER 32
#define LCR_CAR_STEER_FRICTION ((3 * TPE_F) / 4)
#define LCR_CAR_ELASTICITY (TPE_F / 64)
#define LCR_CAR_ACCELERATION (LCR_PHYSICS_UNIT / 9)

#define LCR_CAR_STEER_MAX (LCR_GAME_UNIT / 2)
#define LCR_CAR_STEER_SPEED 50      ///< 0 to 64, lower is faster.
#define LCR_CAR_STEER_SLOWDOWN 64   ///< Slows down steering at higher speeds.

#define LCR_CAR_ACCELERATOR_FACTOR 2
#define LCR_CAR_WHEEL_AIR_ROTATION_SPEED (LCR_GAME_UNIT / 32)
#define LCR_CAR_WHEEL_GROUND_SPEED_DIV 8

#define LCR_CAR_CRASH_SPEED_SMALL 400
#define LCR_CAR_CRASH_SPEED_BIG 800

// Multipliers (in 8ths) of friction and acceleration on concrete:
#define LCR_CAR_GRASS_FACTOR 5
#define LCR_CAR_DIRT_FACTOR  3
#define LCR_CAR_ICE_FACTOR   1
#define LCR_CAR_DRIFT_FACTOR 2      ///< Only affects steering friction.

#define LCR_CAR_DRIFT_THRESHOLD_1 (LCR_GAME_UNIT / 10)
#define LCR_CAR_DRIFT_THRESHOLD_0 (LCR_GAME_UNIT / 227)

#define LCR_CAR_JOINTS 5
#define LCR_CAR_CONNECTIONS 10

#define LCR_REPLAY_EVENT_END 0xff   ///< Special event fed to replay at the end.

struct
{
  TPE_World physicsWorld;
  TPE_Body carBody;
  TPE_Joint carJoints[LCR_CAR_JOINTS];
  TPE_Connection carConnections[LCR_CAR_CONNECTIONS];

  uint32_t tick;             ///< Physics tick (frame) number.
  uint8_t wheelCollisions;   /**< In individual bits records for each car wheel
                                  whether it's currently touching the ground.
                                  Lower bits record current collisions, higher
                                  bits the previous state (for averaging). */

  TPE_Vec3 carPositions[2];  ///< Current and previous position in game units.
  TPE_Vec3 carRotations[2];  ///< Current and previous rotation in game units.
  uint8_t carDrifting;       ///< Whether or not the car is currently in drift.

  TPE_Unit fanForce;         ///< Upwards acceleration caused by a fan.
  LCR_GameUnit wheelAngle;   ///< Current wheel angle, 0 to LCR_GAME_UNIT.
  LCR_GameUnit wheelSteer;   ///< Left/right steer, LCR_CAR_STEER_MAX bounds.

  LCR_GameUnit carSpeeds[2]; /**< Signed speed in game units per tick (negative
                                  if backwards) and its previous value. */
  uint8_t currentInputs;     ///< Current input state (from player or replay).

  uint8_t groundMaterial;    ///< Material currently under car wheels.

  uint16_t crashState;

#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  struct
  {
    uint8_t on;              ///< Currently playing replay?
    uint16_t eventCount;
    uint16_t events[LCR_SETTING_REPLAY_MAX_SIZE]; 

    // for playing
    uint16_t currentEvent;
    uint16_t currentFrame;
    uint32_t achievedTime;   ///< Time achieved in physics frames.
  } replay;
#endif
} LCR_racing;

uint32_t LCR_timeTicksToMS(uint32_t ticks)
{
  return ticks * LCR_RACING_TICK_MS;
}

TPE_Vec3 _LCR_TPE_vec3DividePlain(TPE_Vec3 v, TPE_Unit d)
{
  v.x /= d; v.y /= d; v.z /= d;
  return v;
}

static inline int LCR_racingCarIsDrifting(void)
{
  return LCR_racing.carDrifting;
}

static inline uint8_t LCR_racingCurrentGroundMaterial(void)
{
  return LCR_racing.groundMaterial;
}

/**
  Initializes replay for recording.
*/
void LCR_replayInitRecording(void)
{
  LCR_LOG1("initializing replay recording");
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  LCR_racing.replay.eventCount = 0;
  LCR_racing.replay.achievedTime = 0;
#endif
}

void LCR_replayInitPlaying(void)
{
  LCR_LOG1("initializing replay playing");
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  LCR_racing.replay.currentEvent = 0;
  LCR_racing.replay.currentFrame = 0;
#endif
}

/**
  Outputs current replay using provided character printing function. The string
  will be zero terminated.
*/
void LCR_replayOutputStr(void (*printChar)(char))
{
  LCR_LOG1("outputting replay");

#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  const char *s = LCR_currentMap.name;

  printChar(LCR_RACING_VERSION1);
  printChar(LCR_RACING_VERSION2);

  while (*s)
  {
    printChar(*s);
    s++;
  }

  printChar(';');

  uint32_t hash = LCR_currentMap.hash;

  for (int i = 0; i < 8; ++i)
  {
    printChar(_LCR_hexDigit((hash >> 28) % 16));
    hash <<= 4;
  }

  printChar(' ');

  // 7 decimal digits are enough to record 24 hours

#define PUTD(order) \
  printChar('0' + (LCR_racing.replay.achievedTime / order) % 10);
  PUTD(1000000) PUTD(100000) PUTD(10000)
  PUTD(1000) PUTD(100) PUTD(10) PUTD(1)
#undef PUTD

  for (int i = 0; i < LCR_racing.replay.eventCount; ++i)
  {
    uint16_t e = LCR_racing.replay.events[i];
    printChar(':');

    for (int j = 0; j < 4; ++j)
    {
      printChar(_LCR_hexDigit((e >> 12) & 0x0f));
      e <<= 4;
    }
  }

  printChar('\n');
#endif
}

/**
  Reads replay from string using provided function that returns next character
  in the string. The mapHash and nameHash pointers are optional: if non-zero,
  the memory they point to will be filled with the map hash and name hash.
  Returns 1 on success, else 0.
*/
int LCR_replayLoadFromStr(char (*nextChar)(void), uint32_t *mapHash,
  uint16_t *nameHash)
{
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  char c;

  LCR_racing.replay.eventCount = 0;
  LCR_racing.replay.achievedTime = 0;

  // Has to be like this to force correct evaluation order:
  c = nextChar() == LCR_RACING_VERSION1;
  c |= (nextChar() == LCR_RACING_VERSION2) << 1;

  if (c != 3)
  {
    LCR_LOG1("wrong physics version");
    return 0;
  }

  c = ' ';

  if (nameHash)
    *nameHash = _LCR_simpleStrHash(nextChar,';');
  else
    _LCR_simpleStrHash(nextChar,';');

  if (mapHash)
    *mapHash = 0;

  for (int i = 0; i < 8; ++i) // hash
  {
    c = nextChar();

    if (_LCR_hexDigitVal(c) < 0)
    {
      LCR_LOG1("wrong hash");
      return 0;
    }

    if (mapHash)
      *mapHash = ((*mapHash) << 4) | _LCR_hexDigitVal(c);
  }

  nextChar();

  while (1) // time
  {
    c = nextChar();

    if (c < '0' || c > '9')
      break;

    LCR_racing.replay.achievedTime = 
      LCR_racing.replay.achievedTime * 10 + c - '0';
  }

  while (c != 0) // events
  {
    if (c == ':')
    {
      uint16_t e = 0;

      for (int i = 0; i < 4; ++i)
        e = (e << 4) | _LCR_hexDigitVal(nextChar());

      if (e == 0)
        break;
      
      if (LCR_racing.replay.eventCount >= LCR_SETTING_REPLAY_MAX_SIZE)
      {
        LCR_LOG1("replay too big");
        return 0;
      }

      LCR_racing.replay.events[LCR_racing.replay.eventCount] = e;
      LCR_racing.replay.eventCount++;
    }

    c = nextChar();
  }

  for (int i = LCR_racing.replay.eventCount; // must be here
    i < LCR_SETTING_REPLAY_MAX_SIZE; ++i)
    LCR_racing.replay.events[i] = 0;

  return 1;
#else
  return 0;
#endif
}

int LCR_replayHasFinished(void)
{
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  if (LCR_racing.replay.currentEvent == LCR_racing.replay.eventCount)
  {
    uint32_t totalTime = LCR_racing.replay.currentFrame;

    for (int i = 0; i < LCR_racing.replay.eventCount; ++i)
      totalTime += LCR_racing.replay.events[i] >> 4;

    return totalTime > LCR_racing.replay.achievedTime;
  }

  return LCR_racing.replay.currentEvent > LCR_racing.replay.eventCount;
#else
  return 1;
#endif
}

/**
  When playing back a replay this function returns the next recorded input and
  shifts to the next frame.
*/
uint8_t LCR_replayGetNextInput(void)
{
#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  if (LCR_replayHasFinished())
  {
    LCR_racing.replay.currentFrame++; // has to be here
    return 0;
  }

  if (LCR_racing.replay.currentFrame ==
    (LCR_racing.replay.events[LCR_racing.replay.currentEvent] >> 4))
  {
    LCR_racing.replay.currentEvent++;
    LCR_racing.replay.currentFrame = 0;
  }
 
  LCR_racing.replay.currentFrame++;

  return LCR_racing.replay.currentEvent ?
    (LCR_racing.replay.events[LCR_racing.replay.currentEvent - 1] & 0x0f) : 0;
#else
  return 0;
#endif
}

/**
  Records another input event into a replay. Returns 1 on success, or 0 if the
  event couldn't be recorded. The event is only added if necessary, i.e. this
  function can, and MUST, be called every frame without worrying about inflating
  its size. When the run ends, the LCR_REPLAY_EVENT_END has to be fed!
*/
int LCR_replayRecordEvent(uint32_t frame, uint8_t input)
{
  LCR_LOG2("recording replay event");

#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  if (LCR_racing.replay.achievedTime)
    return 1; // already finished

  if (input == LCR_REPLAY_EVENT_END)
  {
    LCR_racing.replay.achievedTime = frame;
    LCR_LOG1("replay recording finished");
    return 1;
  }

  uint8_t previousInput = 0;
  uint32_t previousFrame = 0;

  for (int i = 0; i < LCR_racing.replay.eventCount; ++i)
  {
    previousInput = LCR_racing.replay.events[i] & 0x0f;
    previousFrame += LCR_racing.replay.events[i] >> 4;
  }

  if (input == previousInput)
    return 1;

  if (frame < previousFrame)
    return 0;

  frame -= previousFrame; // convert to offset

  while (frame > 4095 && LCR_racing.replay.eventCount <
    LCR_SETTING_REPLAY_MAX_SIZE)
  {
    // add intermediate events
    frame -= 4095;
    previousFrame += 4095;

    LCR_racing.replay.events[LCR_racing.replay.eventCount] =
      (previousFrame << 4) | previousInput;
    LCR_racing.replay.eventCount++;
  }

  if (LCR_racing.replay.eventCount >= LCR_SETTING_REPLAY_MAX_SIZE)
    return 0;

  LCR_racing.replay.events[LCR_racing.replay.eventCount] = (frame << 4) | (input & 0x0f);
  LCR_racing.replay.eventCount++;
#endif

  return 1;
}

/**
  Helper function for _LCR_racingEnvironmentFunction, for given arbitrary point
  returns the closest point on map block (of given type) placed at coordinate
  origin (and in default orientation). This function defines shapes of map
  blocks.
*/
TPE_Vec3 _LCR_racingBlockEnvFunc(TPE_Vec3 point, const uint8_t *block)
{
  TPE_Vec3 v, vBest = TPE_vec3(TPE_INFINITY,TPE_INFINITY,TPE_INFINITY);
  TPE_Unit d, dBest = TPE_INFINITY;

  uint8_t bx, by, bz;
  LCR_mapBlockGetCoords(block,&bx,&by,&bz);
 
  TPE_Vec3 blockOffset = TPE_vec3(
    (((int) bx) - LCR_MAP_SIZE_BLOCKS / 2) * LCR_PHYSICS_UNIT,
    (((int) by) - LCR_MAP_SIZE_BLOCKS / 2) * (LCR_PHYSICS_UNIT / 2),
    (((int) bz) - LCR_MAP_SIZE_BLOCKS / 2) * LCR_PHYSICS_UNIT);

  point = TPE_vec3Minus(point,blockOffset); // shift to origin

  uint8_t transform = 
    LCR_mapBlockOppositeTransform(LCR_mapBlockGetTransform(block));

  LCR_TRANSFORM_COORDS(transform,point.x,point.y,point.z,LCR_PHYSICS_UNIT,
    (LCR_PHYSICS_UNIT / 2))

  point = TPE_vec3Minus(point,
    TPE_vec3(LCR_PHYSICS_UNIT / 2,LCR_PHYSICS_UNIT / 4,LCR_PHYSICS_UNIT / 2));

  switch (block[0])
  {
    case LCR_BLOCK_FULL:
    case LCR_BLOCK_BOTTOM:
    case LCR_BLOCK_BOTTOM_ACCEL:
    case LCR_BLOCK_LEFT:
    case LCR_BLOCK_BOTTOM_LEFT:
    case LCR_BLOCK_BOTTOM_LEFT_FRONT:
    case LCR_BLOCK_LEFT_FRONT:
    case LCR_BLOCK_FULL_ACCEL:
    case LCR_BLOCK_FULL_FAN:
    {
      TPE_Vec3
        offset = TPE_vec3(0,0,0),
        size = TPE_vec3(LCR_PHYSICS_UNIT / 2,LCR_PHYSICS_UNIT / 4,
          LCR_PHYSICS_UNIT / 2);

      if (block[0] == LCR_BLOCK_BOTTOM ||
          block[0] == LCR_BLOCK_BOTTOM_ACCEL ||
          block[0] == LCR_BLOCK_BOTTOM_LEFT ||
          block[0] == LCR_BLOCK_BOTTOM_LEFT_FRONT)
      {
        offset.y -= LCR_PHYSICS_UNIT / 8;
        size.y = LCR_PHYSICS_UNIT / 8;
      }

      if (block[0] == LCR_BLOCK_LEFT ||
          block[0] == LCR_BLOCK_BOTTOM_LEFT ||
          block[0] == LCR_BLOCK_BOTTOM_LEFT_FRONT ||
          block[0] == LCR_BLOCK_LEFT_FRONT)
      {
        offset.x -= LCR_PHYSICS_UNIT / 4;
        size.x = LCR_PHYSICS_UNIT / 4;
      }

      if (block[0] == LCR_BLOCK_BOTTOM_LEFT_FRONT ||
          block[0] == LCR_BLOCK_LEFT_FRONT)
      {
        offset.z -= LCR_PHYSICS_UNIT / 4;
        size.z = LCR_PHYSICS_UNIT / 4;
      }

      point = TPE_envAABox(point,offset,size);
      break;
    }

#define _CHECK_NEXT(check)\
  v = check;\
  d = TPE_dist(point,v);\
  if (d < dBest) {\
    vBest = v;\
    dBest = d;}\
  if (dBest == 0) {\
    point = vBest;\
    break;}

    case LCR_BLOCK_RAMP_CURVED_WALL:
      _CHECK_NEXT(TPE_envAABox(point,TPE_vec3(5 * LCR_PHYSICS_UNIT / 12,0,0),
        TPE_vec3(LCR_PHYSICS_UNIT / 12,LCR_PHYSICS_UNIT / 4,LCR_PHYSICS_UNIT
        / 2)));
      // fall through

    case LCR_BLOCK_RAMP_CURVED_PLAT:
      _CHECK_NEXT(TPE_envAABox(point,TPE_vec3(0,0,5 * LCR_PHYSICS_UNIT / 12),
      TPE_vec3(LCR_PHYSICS_UNIT / 2,LCR_PHYSICS_UNIT / 4,LCR_PHYSICS_UNIT / 12
      )));
      // fall through

    case LCR_BLOCK_RAMP_CURVED:
    {
      TPE_Unit sides[6];
      TPE_Unit rampShift = block[0] != LCR_BLOCK_RAMP_CURVED ?
        LCR_PHYSICS_UNIT / 6 : 0;

      sides[0] = -1 * LCR_PHYSICS_UNIT / 8 - rampShift;
      sides[1] = -1 * LCR_PHYSICS_UNIT / 4;
      sides[2] = LCR_PHYSICS_UNIT / 2 - rampShift;
      sides[3] = -1 * LCR_PHYSICS_UNIT / 4;
      sides[4] = LCR_PHYSICS_UNIT / 2 - rampShift;
      sides[5] = LCR_PHYSICS_UNIT / 4;

      _CHECK_NEXT(TPE_envAATriPrism(point,
        TPE_vec3(0,0,0),sides,LCR_PHYSICS_UNIT,2))

      sides[0] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[1] = -1 * LCR_PHYSICS_UNIT / 4;
      sides[2] = LCR_PHYSICS_UNIT / 2 - rampShift;
      sides[3] = -1 * LCR_PHYSICS_UNIT / 4;
      sides[4] = LCR_PHYSICS_UNIT / 2 - rampShift;
      sides[5] = LCR_PHYSICS_UNIT / 8;

      _CHECK_NEXT(TPE_envAATriPrism(point,TPE_vec3(0,0,0),sides,LCR_PHYSICS_UNIT
        ,2));

      point = vBest;

      break;
    }

    case LCR_BLOCK_CORNER_CONVEX:
    case LCR_BLOCK_CORNER_CONCAVE:
    {
      TPE_Unit sides[6];
      sides[0] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[1] = LCR_PHYSICS_UNIT / 2;
      sides[2] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[3] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[4] = LCR_PHYSICS_UNIT / 5;
      sides[5] = -1 * LCR_PHYSICS_UNIT / 5;

      if (block[0] == LCR_BLOCK_CORNER_CONCAVE)
      {
        sides[4] *= -1;
        sides[5] *= -1; 
      }

      _CHECK_NEXT(TPE_envAATriPrism(point,TPE_vec3(0,0,0),sides,
        LCR_PHYSICS_UNIT / 2,1));

      sides[2] = sides[4];
      sides[3] = sides[5];

      sides[4] = LCR_PHYSICS_UNIT / 2;
      sides[5] = LCR_PHYSICS_UNIT / 2;

      _CHECK_NEXT(TPE_envAATriPrism(point,TPE_vec3(0,0,0),sides,
        LCR_PHYSICS_UNIT / 2,1));

      point = vBest;

      break;
    }

    case LCR_BLOCK_RAMP_12_UP:
    {
      TPE_Unit sides[6];
      sides[0] = LCR_PHYSICS_UNIT / 2;
      sides[1] = LCR_PHYSICS_UNIT / 4;
      sides[2] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[3] = 0;
      sides[4] = LCR_PHYSICS_UNIT / 2;
      sides[5] = -1 * LCR_PHYSICS_UNIT / 4;

      _CHECK_NEXT(TPE_envAATriPrism(point,TPE_vec3(0,0,0),sides,
        LCR_PHYSICS_UNIT,2));

      _CHECK_NEXT(TPE_envAABox(
        point,TPE_vec3(0,-1 * LCR_PHYSICS_UNIT / 8,0),TPE_vec3(
        LCR_PHYSICS_UNIT / 2,LCR_PHYSICS_UNIT / 8,LCR_PHYSICS_UNIT / 2)));

      point = vBest;

      break;
    }

#undef _CHECK_NEXT

    case LCR_BLOCK_RAMP_CORNER:
    {
      TPE_Unit sides[6];

      sides[0] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[1] = -1 * LCR_PHYSICS_UNIT / 4;
      sides[2] = LCR_PHYSICS_UNIT / 2;
      sides[3] = -1 * LCR_PHYSICS_UNIT / 4;
      sides[4] = LCR_PHYSICS_UNIT / 2;
      sides[5] = LCR_PHYSICS_UNIT / 4;

      if (point.x > LCR_PHYSICS_UNIT / 2)
      {
        sides[4] *= -1;

        point = TPE_envAATriPrism(point,TPE_vec3(0,0,0),
          sides,LCR_PHYSICS_UNIT,2);
      }
      else if (point.z < -1 * LCR_PHYSICS_UNIT / 2)
        point = TPE_envAATriPrism(point,TPE_vec3(0,0,0),
          sides,LCR_PHYSICS_UNIT,0);
      else if (point.y < -1 * LCR_PHYSICS_UNIT / 4)
      {
        sides[1] *= 2;
        sides[3] *= 2;
        sides[5] *= 2;

        point = TPE_envAATriPrism(point,
          TPE_vec3(0,0,0),sides,LCR_PHYSICS_UNIT / 2,1);
      }
      else
      {
        point = TPE_envHalfPlane(point,TPE_vec3(LCR_PHYSICS_UNIT / 2,
          LCR_PHYSICS_UNIT / 4,-1 * LCR_PHYSICS_UNIT / 2),
          TPE_vec3(-1 * LCR_PHYSICS_UNIT,2 * LCR_PHYSICS_UNIT,LCR_PHYSICS_UNIT));

#define LINESNAP(a,b,c,d,e,f)\
  point = TPE_envLineSegment(point,\
    TPE_vec3(a * LCR_PHYSICS_UNIT / 2,b * LCR_PHYSICS_UNIT / 4,\
    c * LCR_PHYSICS_UNIT / 2),TPE_vec3(d * LCR_PHYSICS_UNIT / 2,\
    e * LCR_PHYSICS_UNIT / 4,f * LCR_PHYSICS_UNIT / 2));

        if (point.y < -1 * LCR_PHYSICS_UNIT / 4)
          LINESNAP(-1,-1,-1,1,-1,1)
        else if (point.x > LCR_PHYSICS_UNIT / 2)
          LINESNAP(1,1,-1,1,-1,1)
        else if (point.z < -1 * LCR_PHYSICS_UNIT / 2)
          LINESNAP(-1,-1,-1,1,1,-1)
#undef LINESNAP
      }

      break;
    }

    case LCR_BLOCK_RAMP:
    case LCR_BLOCK_RAMP_34:
    case LCR_BLOCK_RAMP_12:
    case LCR_BLOCK_RAMP_14:
    case LCR_BLOCK_RAMP_STEEP:
    case LCR_BLOCK_RAMP_ACCEL:
    case LCR_BLOCK_RAMP_FAN:
    {
      uint8_t front, top;
      LCR_rampGetDimensions(block[0],&top,&front);
      front = 6 - front;

      TPE_Unit sides[6];
      sides[0] = 
        -1 * LCR_PHYSICS_UNIT / 2 + (LCR_PHYSICS_UNIT / 6) * ((int) front);

      sides[1] = -1 * LCR_PHYSICS_UNIT / 4;

      sides[2] = LCR_PHYSICS_UNIT / 2;
      sides[3] = -1 * LCR_PHYSICS_UNIT / 4;

      sides[4] = LCR_PHYSICS_UNIT / 2;
      sides[5] = -1 * LCR_PHYSICS_UNIT / 4 +
        ((int) top) * (LCR_PHYSICS_UNIT / 8);

      point = TPE_envAATriPrism(point,TPE_vec3(0,0,0),sides,LCR_PHYSICS_UNIT,2);
      break;
    }

    case LCR_BLOCK_HILL:
    {
      point = (point.y > -1 * LCR_PHYSICS_UNIT / 4 && point.z
        < LCR_PHYSICS_UNIT / 4) ?
          TPE_envCylinder(point,
            TPE_vec3(0,-1 * LCR_PHYSICS_UNIT / 2,LCR_PHYSICS_UNIT / 4),
            TPE_vec3(LCR_PHYSICS_UNIT / 2,0,0),
            LCR_PHYSICS_UNIT / 2 + LCR_PHYSICS_UNIT / 4) :
          TPE_envAABox(point,TPE_vec3(0,0,0),
            TPE_vec3(LCR_PHYSICS_UNIT / 2,LCR_PHYSICS_UNIT / 4,
            LCR_PHYSICS_UNIT / 2));

      if (point.y < -1 * LCR_PHYSICS_UNIT / 4) // for some reason happens somet.
        point.y = -1 * LCR_PHYSICS_UNIT / 4;

      break;
    }

    case LCR_BLOCK_BUMP:
      point = TPE_envCone(point,TPE_vec3(0,-1 * LCR_PHYSICS_UNIT / 4 ,0),
        TPE_vec3(0,LCR_PHYSICS_UNIT / 6,0),(5 * LCR_PHYSICS_UNIT) / 12);
      break;

    case LCR_BLOCK_CORNER:
    case LCR_BLOCK_CORNER_12:
    {
      TPE_Unit sides[6];
      sides[0] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[1] = LCR_PHYSICS_UNIT / 2;
      sides[2] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[3] = -1 * LCR_PHYSICS_UNIT / 2;
      sides[4] = block[0] == LCR_BLOCK_CORNER ? LCR_PHYSICS_UNIT / 2 : 0;
      sides[5] = LCR_PHYSICS_UNIT / 2;

      point = TPE_envAATriPrism(point,TPE_vec3(0,0,0),sides,
        LCR_PHYSICS_UNIT / 2,1);
      
      break;
    }

    default:
      point = TPE_vec3(0,0,LCR_MAP_SIZE_BLOCKS * LCR_PHYSICS_UNIT);
      break;
  }

  point = TPE_vec3Plus(point,
    TPE_vec3(LCR_PHYSICS_UNIT / 2,LCR_PHYSICS_UNIT / 4,LCR_PHYSICS_UNIT / 2));

  transform = LCR_mapBlockOppositeTransform(transform);

  LCR_TRANSFORM_COORDS(transform,point.x,point.y,point.z,LCR_PHYSICS_UNIT,
    (LCR_PHYSICS_UNIT / 2))

  point = TPE_vec3Plus(point,blockOffset); // shift back

  return point;
}

/**
  For tinyphysicsengine, function that defines the shape of the static physics
  world. For any given point in space returns the environment's closest point.
*/
TPE_Vec3 _LCR_racingEnvironmentFunction(TPE_Vec3 point, TPE_Unit maxDist)
{
  // start with the map outside walls:
  TPE_ENV_START(TPE_envAABoxInside(point,TPE_vec3(0,0,0),TPE_vec3(
    LCR_PHYSICS_UNIT * LCR_MAP_SIZE_BLOCKS,
    (LCR_PHYSICS_UNIT * LCR_MAP_SIZE_BLOCKS) / 2,
    LCR_PHYSICS_UNIT * LCR_MAP_SIZE_BLOCKS)),point)

  // without this check we might try to get block outside the map 
  if (_pBest.x == point.x && _pBest.y == point.y && _pBest.z == point.z)
    return _pBest;

  if (maxDist <= LCR_PHYSICS_UNIT / 4) // considering half of square height
  {
    /* Here we only check the 8 closest blocks => relatively fast. */

    TPE_Vec3 pointShifted = TPE_vec3Plus(point,TPE_vec3(
        (LCR_MAP_SIZE_BLOCKS / 2) * LCR_PHYSICS_UNIT,
        (LCR_MAP_SIZE_BLOCKS / 4) * LCR_PHYSICS_UNIT,
        (LCR_MAP_SIZE_BLOCKS / 2) * LCR_PHYSICS_UNIT));

    uint8_t coords[6]; // x_low, x_high, y_low, y_high, z_low, z_high

    coords[0] = (pointShifted.x / LCR_PHYSICS_UNIT);
    coords[1] = (pointShifted.x % LCR_PHYSICS_UNIT < LCR_PHYSICS_UNIT / 2);

    coords[2] = (pointShifted.y / (LCR_PHYSICS_UNIT / 2));
    coords[3] =
      (pointShifted.y % (LCR_PHYSICS_UNIT / 2) < LCR_PHYSICS_UNIT / 4);

    coords[4] = (pointShifted.z / LCR_PHYSICS_UNIT);
    coords[5] = (pointShifted.z % LCR_PHYSICS_UNIT < LCR_PHYSICS_UNIT / 2);

    for (int i = 0; i < 6; i += 2)
      if (coords[i + 1])
      {
        coords[i + 1] = coords[i];
        coords[i] = coords[i] > 0 ? coords[i] - 1 : 0;
      }
      else
        coords[i + 1] = coords[i] < 63 ? coords[i] + 1 : 63;

    int start = 0, end = LCR_currentMap.blockCount - 1;

    for (uint8_t i = 0; i < 8; ++i)
    {
      /* Black magic: here we make it so that we check the lowest coord numbers
        (0,0,0), then the highest (1,1,1), then second lowest (1,0,0), then
        second highest (0,1,1) etc. This way we are narrowing the range (start,
        end) for the binary search. */

      int blockNum = LCR_mapGetBlockAtFast(
        coords[0] + ((i ^ (i >> 1)) & 0x01),
        coords[2] + ((i ^ (i >> 2)) & 0x01),
        coords[4] + (i & 0x01),start,end);

      if (blockNum >= 0) // is there a block at the coords?
      {
        TPE_ENV_NEXT(_LCR_racingBlockEnvFunc(point, // check it
          LCR_currentMap.blocks + blockNum * LCR_BLOCK_SIZE),point)

        // Narrow the search range:
        if (i % 2 == 0 && blockNum > start)
          start = blockNum;

        if (i % 2 && blockNum < end)
          end = blockNum;
      }
    }
  }
  else
  {
    LCR_LOG1("collision checking all blocks (shouldn't happen often!)");

    const uint8_t *block = LCR_currentMap.blocks;

    // Full check of all map blocks, slow, shouldn't happen often!
    for (int j = 0; j < LCR_currentMap.blockCount; ++j)
    {
      TPE_ENV_NEXT(_LCR_racingBlockEnvFunc(point,block),point)
      block += LCR_BLOCK_SIZE;
    }
  }

  TPE_ENV_END
}

LCR_GameUnit LCR_racingGetCarSpeedUnsigned(void)
{
  return LCR_racing.carSpeeds[0] >= 0 ? LCR_racing.carSpeeds[0] :
    (-1 * LCR_racing.carSpeeds[0]);
}

LCR_GameUnit LCR_racingGetCarSpeedSigned(void)
{
  return LCR_racing.carSpeeds[0];
}

uint8_t _LCR_racingCollisionHandler(uint16_t b1, uint16_t j1, uint16_t b2,
  uint16_t j2, TPE_Vec3 p)
{
#if LCR_SETTING_CRASH_SOUNDS
  // Detect crashes:

  TPE_Unit speed = TPE_vec3Len(
    TPE_vec3Project(TPE_vec3(
      LCR_racing.carBody.joints[j1].velocity[0],
        LCR_racing.carBody.joints[j1].velocity[1],
        LCR_racing.carBody.joints[j1].velocity[2]),
      TPE_vec3Minus(p,LCR_racing.carBody.joints[j1].position)));

  LCR_racing.crashState |= ((speed >= LCR_CAR_CRASH_SPEED_BIG) << 1) |
    (speed >= LCR_CAR_CRASH_SPEED_SMALL);
#endif

  // Check which wheels are touching the ground:

  if (j1 < 4) // wheel joint?
    LCR_racing.wheelCollisions |= 0x01 << j1;

  return 1;
}

LCR_GameUnit _LCR_racingInterpolateRot(LCR_GameUnit angleNew,
  LCR_GameUnit angleOld, LCR_GameUnit interpolationParam)
{
  angleNew %= LCR_GAME_UNIT;

  if (angleNew < 0)
    angleNew += LCR_GAME_UNIT;

  angleOld %= LCR_GAME_UNIT;

  if (angleOld < 0)
    angleOld += LCR_GAME_UNIT;

  LCR_GameUnit diff = angleNew - angleOld;

  LCR_GameUnit diff2 = (diff > 0) ?
    -1 * (LCR_GAME_UNIT - diff) :
    (LCR_GAME_UNIT + diff);

  return angleOld + ((TPE_abs(diff) < TPE_abs(diff2) ?
    diff : diff2) * interpolationParam) / LCR_GAME_UNIT;
}

TPE_Vec3 _LCR_racingGetWheelCenterPoint(void)
{
  return _LCR_TPE_vec3DividePlain(
    TPE_vec3Plus(
      TPE_vec3Plus(
        LCR_racing.carBody.joints[0].position,
        LCR_racing.carBody.joints[1].position),
      TPE_vec3Plus(
        LCR_racing.carBody.joints[2].position,
        LCR_racing.carBody.joints[3].position)),4);
}

void _LCR_racingUpdateCarPosRot(void)
{
  TPE_Vec3 tmpVec = LCR_racing.carPositions[0];

  TPE_Unit tccFix = tmpVec.x; /* Fix for TCC: for whatever reason the following
                                 line nukes the x coord in tmpVec under TCC, so
                                 here we back it up and then restore it. */

  LCR_racing.carPositions[0] = _LCR_TPE_vec3DividePlain(
    TPE_vec3TimesPlain(_LCR_racingGetWheelCenterPoint(),LCR_GAME_UNIT),
      LCR_PHYSICS_UNIT);

  tmpVec.x = tccFix;

  LCR_racing.carPositions[0] = // smooth the position
    TPE_vec3KeepWithinBox(LCR_racing.carPositions[1],LCR_racing.carPositions[0],
        TPE_vec3(
          LCR_PHYSICS_UNIT / 128, // hardcoded consts
          LCR_PHYSICS_UNIT / 128,
          LCR_PHYSICS_UNIT / 128));

  LCR_racing.carPositions[1] = tmpVec;

  tmpVec = _LCR_TPE_vec3DividePlain(TPE_vec3TimesPlain(TPE_bodyGetRotation(
    &(LCR_racing.carBody),0,2,1),LCR_GAME_UNIT),TPE_F);

  LCR_racing.carRotations[1] = LCR_racing.carRotations[0];
  LCR_racing.carRotations[0] = tmpVec;
}

/**
  Initializes a new run.
*/
void LCR_racingRestart(uint8_t replay)
{
  LCR_LOG0("restarting race");
  LCR_mapReset();

  LCR_racing.tick = 0;
  LCR_racing.fanForce = 0;

#if LCR_SETTING_REPLAY_MAX_SIZE > 0
  LCR_racing.replay.on = replay;
#endif

  TPE_bodyActivate(&(LCR_racing.carBody));
  LCR_racing.wheelCollisions = 0;

  LCR_racing.wheelAngle = 0;
  LCR_racing.wheelSteer = 0;
  LCR_racing.carSpeeds[0] = 0;
  LCR_racing.carSpeeds[1] = 0;
  LCR_racing.carDrifting = 0;
  LCR_racing.crashState = 0;
  LCR_racing.currentInputs = 0;

  // make the car body:
  TPE_makeCenterRectFull(LCR_racing.carJoints,
    LCR_racing.carConnections,
    LCR_PHYSICS_UNIT / 2,
    (LCR_PHYSICS_UNIT * 3) / 4,
    LCR_PHYSICS_UNIT / 8);

  LCR_racing.carJoints[4].position.y += LCR_PHYSICS_UNIT / 6;
  LCR_racing.carJoints[4].sizeDivided *= 3;
  LCR_racing.carJoints[4].sizeDivided /= 2;

  TPE_bodyInit(&(LCR_racing.carBody),LCR_racing.carJoints,LCR_CAR_JOINTS,
    LCR_racing.carConnections,LCR_CAR_CONNECTIONS,TPE_F);

  LCR_racing.carBody.friction = LCR_CAR_FORWARD_FRICTION;
  LCR_racing.carBody.elasticity = LCR_CAR_ELASTICITY;
  LCR_racing.carBody.flags |= TPE_BODY_FLAG_ALWAYS_ACTIVE;

  /* We disable bounding sphere checks because that would lead to calling env.
    function with large min. distance which would lead to slow iteration over
    all map blocks. */
  LCR_racing.carBody.flags |= TPE_BODY_FLAG_NO_BSPHERE;

  TPE_bodyMoveTo(&(LCR_racing.carBody),TPE_vec3(
    (((TPE_Unit) LCR_currentMap.startPos[0]) - LCR_MAP_SIZE_BLOCKS / 2)
      * LCR_PHYSICS_UNIT + LCR_PHYSICS_UNIT / 2,
    (((TPE_Unit) LCR_currentMap.startPos[1]) - LCR_MAP_SIZE_BLOCKS / 2)
      * LCR_PHYSICS_UNIT / 2 + LCR_PHYSICS_UNIT / 4,
    (((TPE_Unit) LCR_currentMap.startPos[2]) - LCR_MAP_SIZE_BLOCKS / 2)
      * LCR_PHYSICS_UNIT + LCR_PHYSICS_UNIT / 2));

  if (LCR_currentMap.startPos[3])
  {
    uint8_t trans = LCR_currentMap.startPos[3];

    if (trans & LCR_BLOCK_TRANSFORM_FLIP_V)
      TPE_bodyRotateByAxis(&(LCR_racing.carBody),
        TPE_vec3(TPE_FRACTIONS_PER_UNIT / 2,0,0));

    trans &=
      LCR_BLOCK_TRANSFORM_ROT_90 |
      LCR_BLOCK_TRANSFORM_ROT_180 |
      LCR_BLOCK_TRANSFORM_ROT_270;

    TPE_bodyRotateByAxis(&(LCR_racing.carBody),
      TPE_vec3(0,(trans == LCR_BLOCK_TRANSFORM_ROT_90) ?
        3 * TPE_F / 4 : (trans == LCR_BLOCK_TRANSFORM_ROT_180) ?
          TPE_F / 2 : (TPE_F / 4),0));
  }

  LCR_racing.carPositions[0] = TPE_vec3(0,0,0);
  LCR_racing.carPositions[1] = LCR_racing.carPositions[0];
  LCR_racing.carRotations[0] = TPE_vec3(0,0,0);
  LCR_racing.carRotations[1] = LCR_racing.carRotations[0];

  _LCR_racingUpdateCarPosRot();

  LCR_racing.carPositions[1] = LCR_racing.carPositions[0];
  LCR_racing.carRotations[1] = LCR_racing.carRotations[0];
}

/**
  Initializes the racing module, only call once.
*/
void LCR_racingInit(void)
{
  LCR_LOG0("initializing racing engine");

  TPE_worldInit(&(LCR_racing.physicsWorld),
  &(LCR_racing.carBody),1,_LCR_racingEnvironmentFunction);

#if LCR_SETTING_REPLAY_MAX_SIZE > 0
  LCR_racing.replay.on = 0;
#endif
  LCR_racing.physicsWorld.collisionCallback = _LCR_racingCollisionHandler;
}

/**
  Gets current car transformation intended for rendering, i.e. potentially with
  smoothing and interpolation (by LCR_GameUnits) applied to the underlying
  internal state in the physics engine.
*/
void LCR_racingGetCarTransform(LCR_GameUnit position[3],
  LCR_GameUnit rotation[3], LCR_GameUnit interpolationParam)
{
  LCR_LOG2("getting car transform");  

  TPE_Vec3 v;

#if LCR_SETTING_SMOOTH_ANIMATIONS
  v = TPE_vec3Plus(LCR_racing.carPositions[1], // LERP previous and current pos
    _LCR_TPE_vec3DividePlain(
      TPE_vec3TimesPlain(TPE_vec3Minus(
        LCR_racing.carPositions[0],LCR_racing.carPositions[1]),
          interpolationParam),LCR_GAME_UNIT));

  position[0] = v.x;
  position[1] = v.y;
  position[2] = v.z;

  rotation[0] = _LCR_racingInterpolateRot(LCR_racing.carRotations[0].x,
    LCR_racing.carRotations[1].x,interpolationParam);
  rotation[1] = _LCR_racingInterpolateRot(LCR_racing.carRotations[0].y,
    LCR_racing.carRotations[1].y,interpolationParam);
  rotation[2] = _LCR_racingInterpolateRot(LCR_racing.carRotations[0].z,
    LCR_racing.carRotations[1].z,interpolationParam);
#else
  position[0] = LCR_racing.carPositions[0].x;
  position[1] = LCR_racing.carPositions[0].y;
  position[2] = LCR_racing.carPositions[0].z;
  rotation[0] = LCR_racing.carRotations[0].x;
  rotation[1] = LCR_racing.carRotations[0].y;
  rotation[2] = LCR_racing.carRotations[0].z;
#endif
}

/**
  Gets the current block coordinates of the car, without interpolation etc.,
  intended for checking accurately whether CP has been taken etc.
*/
void LCR_racingGetCarBlockCoords(int coords[3])
{
  coords[0] = (LCR_racing.carPositions[0].x +
    (LCR_GAME_UNIT * LCR_MAP_SIZE_BLOCKS) / 2) / LCR_GAME_UNIT;

  coords[1] = (LCR_racing.carPositions[0].y +
    (LCR_GAME_UNIT * LCR_MAP_SIZE_BLOCKS) / 4) / (LCR_GAME_UNIT / 2);

  coords[2] = (LCR_racing.carPositions[0].z +
    (LCR_GAME_UNIT * LCR_MAP_SIZE_BLOCKS) / 2) / LCR_GAME_UNIT;
}

LCR_GameUnit LCR_racingGetWheelRotation(void)
{
  return LCR_racing.wheelAngle;
}

LCR_GameUnit LCR_racingGetWheelSteer(void)
{
  return LCR_racing.wheelSteer;
}

TPE_Unit _LCR_applyMaterialFactor(TPE_Unit value, uint8_t mat)
{
  switch (mat)
  {
    case LCR_BLOCK_MATERIAL_GRASS: value *= LCR_CAR_GRASS_FACTOR; break;
    case LCR_BLOCK_MATERIAL_DIRT:  value *= LCR_CAR_DIRT_FACTOR;  break;
    case LCR_BLOCK_MATERIAL_ICE:   value *= LCR_CAR_ICE_FACTOR;   break;
    default: value *= 8; break;
  }

  return value / 8;
}

void _LCR_racingWheelAccelerate(unsigned int wheel, TPE_Vec3 dir,
  uint8_t material, uint8_t accelerator)
{
  TPE_Unit acc = _LCR_applyMaterialFactor(LCR_CAR_ACCELERATION,material);

  acc = acc / (1 + (LCR_racingGetCarSpeedUnsigned() / LCR_CAR_AIR_FRICTION));

  if (accelerator)
    acc *= LCR_CAR_ACCELERATOR_FACTOR;

  LCR_racing.carBody.joints[wheel].velocity[0] += (dir.x * acc) / TPE_F;
  LCR_racing.carBody.joints[wheel].velocity[1] += (dir.y * acc) / TPE_F;
  LCR_racing.carBody.joints[wheel].velocity[2] += (dir.z * acc) / TPE_F;
}

int _LCR_racingCarShapeOK(void)
{
  TPE_Unit bodyTension = 0;
  TPE_Connection *c = LCR_racing.carConnections;

  if (TPE_vec3Dot( // checks if roof is on the correct side (can be pushed down)
        TPE_vec3Minus(
          LCR_racing.carBody.joints[4].position,
          LCR_racing.carBody.joints[0].position),
        TPE_vec3Cross(
          TPE_vec3Minus(
            LCR_racing.carBody.joints[0].position,
            LCR_racing.carBody.joints[2].position),
          TPE_vec3Minus(
            LCR_racing.carBody.joints[0].position,
            LCR_racing.carBody.joints[1].position))) <= 0)
  {
    LCR_LOG1("roof flipped over");
    return 0;
  }

  for (int i = 0; i < LCR_CAR_CONNECTIONS; ++i) // joint tension
  {
    bodyTension += TPE_abs(
      TPE_connectionTension(
        TPE_LENGTH(
          TPE_vec3Minus(
            LCR_racing.carJoints[c->joint1].position,
            LCR_racing.carJoints[c->joint2].position)),
         c->length));

    c++;
  }

  return (bodyTension / LCR_CAR_CONNECTIONS) <= TPE_RESHAPE_TENSION_LIMIT;
}

/**
  Updates the racing physics world, call every LCR_RACING_TICK_MS_RT
  milliseconds. Returns a set of events (logically ORed) that occured during
  this step.
*/
uint32_t LCR_racingStep(unsigned int input)
{
  LCR_LOG2("racing step (start)");

  uint32_t result = 0;
  TPE_Vec3 carForw, carRight, carUp, carVel;
  uint8_t onAccel = 0; // standing on accelerator?
  int groundBlockIndex = -1;
  TPE_Unit driftFriction = 0; // average wheel friction (absolute value)
  
  LCR_racing.groundMaterial = LCR_BLOCK_MATERIAL_CONCRETE;

#if LCR_SETTING_REPLAY_MAX_SIZE > 0
  if (LCR_racing.replay.on)
  {
    if (LCR_racing.tick == 0)
      LCR_replayInitPlaying();

    if (LCR_replayHasFinished())
      return LCR_RACING_EVENT_FINISHED;

    input = LCR_replayGetNextInput();
  }
  else
#endif
  {
    if (LCR_racing.tick == 0)
      LCR_replayInitRecording();

    LCR_replayRecordEvent(LCR_racing.tick,input);
  }

  LCR_racing.currentInputs = input;

  carForw = TPE_vec3Normalized(TPE_vec3Plus(
    TPE_vec3Minus(LCR_racing.carBody.joints[0].position,
                  LCR_racing.carBody.joints[2].position),
    TPE_vec3Minus(LCR_racing.carBody.joints[1].position,
                  LCR_racing.carBody.joints[3].position)));

  carRight = TPE_vec3Normalized(TPE_vec3Plus(
    TPE_vec3Minus(LCR_racing.carBody.joints[0].position,
                  LCR_racing.carBody.joints[1].position),
    TPE_vec3Minus(LCR_racing.carBody.joints[2].position,
                  LCR_racing.carBody.joints[3].position)));

  carUp = TPE_vec3Cross(carForw,carRight);

  carVel = TPE_vec3(
    LCR_racing.carBody.joints[4].velocity[0],
    LCR_racing.carBody.joints[4].velocity[1],
    LCR_racing.carBody.joints[4].velocity[2]);

  /* Apply gravity like this: if all wheels are on ground, we don't apply
    gravity to roof. This helps prevent sliding when standing still. */

  for (int i = 0; i < 5; ++i)
    if (i < 4 || (((LCR_racing.wheelCollisions |
      (LCR_racing.wheelCollisions >> 4)) & 0x0f) != 0x0f))
      LCR_racing.carBody.joints[i].velocity[1] -= LCR_GRAVITY;

  if (LCR_racing.wheelCollisions) // at least one wheel on ground?
  {
    TPE_Unit upDot = TPE_vec3Dot(carUp,TPE_vec3(0,TPE_F,0));

    /* if car is "somewhat" horizontal (TPE_F / 8 is a hardcoded constant)
      (else we assume we're riding on wall): */
    if (upDot > TPE_F / 8 || upDot < -1 * TPE_F / 8)
    {
      uint8_t
        gx = (LCR_racing.carPositions[0].x + (LCR_MAP_SIZE_BLOCKS / 2) *
          LCR_GAME_UNIT) / LCR_GAME_UNIT,
        gy = (LCR_racing.carPositions[0].y + (LCR_MAP_SIZE_BLOCKS / 2) *
          (LCR_GAME_UNIT / 2)) / (LCR_GAME_UNIT / 2),
        gz = (LCR_racing.carPositions[0].z + (LCR_MAP_SIZE_BLOCKS / 2) *
          LCR_GAME_UNIT) / LCR_GAME_UNIT;

      TPE_Unit yMod = (LCR_racing.carPositions[0].y + LCR_MAP_SIZE_BLOCKS *
        LCR_GAME_UNIT / 2) % (LCR_GAME_UNIT / 2);

      if (yMod < LCR_GAME_UNIT / 5) // if kinda at the bottom of the block
        groundBlockIndex = LCR_mapGetBlockAt(gx,gy - 1,gz);

      if (groundBlockIndex == -1)
        groundBlockIndex = LCR_mapGetBlockAt(gx,gy,gz);

      if (groundBlockIndex == -1) // still nothing? try bottom again
        groundBlockIndex = LCR_mapGetBlockAt(gx,gy - 1,gz);
    }

    if (groundBlockIndex != -1)
    {
      uint8_t b = LCR_currentMap.blocks[groundBlockIndex * LCR_BLOCK_SIZE];

      onAccel = LCR_mapBlockIsAccelerator(b);

      if (LCR_mapBlockIsFan(b))
      {
        LCR_racing.fanForce = LCR_GRAVITY * LCR_FAN_FORCE;
        result |= LCR_RACING_EVENT_FAN;
      }

      LCR_racing.groundMaterial = LCR_mapBlockGetMaterial(
        LCR_currentMap.blocks + groundBlockIndex * LCR_BLOCK_SIZE);
    }
  }

  LCR_racing.carBody.friction =
    _LCR_applyMaterialFactor(LCR_CAR_FORWARD_FRICTION,LCR_racing.groundMaterial);

  if (onAccel)
  {
    input |= LCR_RACING_INPUT_FORW; // accelerator enforces this
    result |= LCR_RACING_EVENT_ACCELERATOR;
  }

  if(!(input & (LCR_RACING_INPUT_FORW | LCR_RACING_INPUT_BACK)))
    LCR_racing.carBody.friction *= LCR_CAR_STAND_FRICTION_MULTIPLIER;
  else if (
    ((input & LCR_RACING_INPUT_FORW) && (LCR_racing.carSpeeds[0] < 0)) ||
    ((input & LCR_RACING_INPUT_BACK) && (LCR_racing.carSpeeds[0] > 0)))
    LCR_racing.carBody.friction *= 2 * LCR_CAR_STAND_FRICTION_MULTIPLIER;

  if (input)
  {
    unsigned char steering = 0;
  
    if ((input & LCR_RACING_INPUT_BACK) && LCR_racing.wheelCollisions == 0x00)
    {
      LCR_LOG2("air brake");

      LCR_racing.carBody.jointCount--; // exclude roof (may be unsqueezing)
      TPE_Vec3 linVel = TPE_bodyGetLinearVelocity(&LCR_racing.carBody);
      LCR_racing.carBody.jointCount++;
       
      for (int i = 0; i < LCR_CAR_JOINTS - 1; ++i) // exclude roof again
      {
        LCR_racing.carBody.joints[i].velocity[0] = linVel.x; 
        LCR_racing.carBody.joints[i].velocity[1] = linVel.y; 
        LCR_racing.carBody.joints[i].velocity[2] = linVel.z; 
      }

      // gradual slowing down mid-air:

      LCR_racing.carBody.joints[4].velocity[0] = 
        15 * (LCR_racing.carBody.joints[LCR_CAR_JOINTS - 1].velocity[0] / 16);

      LCR_racing.carBody.joints[4].velocity[2] = 
        15 * (LCR_racing.carBody.joints[LCR_CAR_JOINTS - 1].velocity[2] / 16);
    }

    if (input & (LCR_RACING_INPUT_FORW | LCR_RACING_INPUT_BACK))
    {
      LCR_GameUnit rotateBy =
        LCR_racing.wheelCollisions ? // on ground slow down wheel rot.
        (LCR_racingGetCarSpeedUnsigned() / LCR_CAR_WHEEL_GROUND_SPEED_DIV) :
        LCR_CAR_WHEEL_AIR_ROTATION_SPEED;

      if (!(input & LCR_RACING_INPUT_BACK))
        rotateBy *= -1;

      LCR_racing.wheelAngle =
        (LCR_racing.wheelAngle + rotateBy) % LCR_GAME_UNIT;

      if (LCR_racing.wheelAngle < 0)
        LCR_racing.wheelAngle += LCR_GAME_UNIT;
    }

    if (input & LCR_RACING_INPUT_RIGHT)
    {
      steering = 2;

      // We slow down steering by 1 for each LCR_CAR_STEER_SLOWDOWN speed.
      LCR_racing.wheelSteer = LCR_CAR_STEER_MAX -
        (TPE_min(LCR_CAR_STEER_SPEED + LCR_racingGetCarSpeedUnsigned() /
         LCR_CAR_STEER_SLOWDOWN,64) * (LCR_CAR_STEER_MAX -
         LCR_racing.wheelSteer)) / 64;
    }
    else if (input & LCR_RACING_INPUT_LEFT)
    {
      steering = 1;

      LCR_racing.wheelSteer = -1 * LCR_CAR_STEER_MAX +
        (TPE_min(LCR_CAR_STEER_SPEED + LCR_racingGetCarSpeedUnsigned() /
        LCR_CAR_STEER_SLOWDOWN,64) * (LCR_racing.wheelSteer +
        LCR_CAR_STEER_MAX)) / 64;
    }

    if ((LCR_racing.wheelCollisions & 0xcc)) // back wheel on ground?
    {
      if (input & LCR_RACING_INPUT_FORW)
      { 
        _LCR_racingWheelAccelerate(0,carForw,LCR_racing.groundMaterial,onAccel);
        _LCR_racingWheelAccelerate(1,carForw,LCR_racing.groundMaterial,onAccel);
      }
      else if (input & LCR_RACING_INPUT_BACK)
      {
        _LCR_racingWheelAccelerate(0,TPE_vec3TimesPlain(carForw,-1),
          LCR_racing.groundMaterial,onAccel);
        _LCR_racingWheelAccelerate(1,TPE_vec3TimesPlain(carForw,-1),
          LCR_racing.groundMaterial,onAccel);
      }
    }

    for (int i = 0; i < 4; ++i)
      if (LCR_racing.wheelCollisions & (0x11 << i)) // wheel on ground?
      {
        TPE_Vec3 jv = TPE_vec3( // joint velocity 
          LCR_racing.carBody.joints[i].velocity[0],   
          LCR_racing.carBody.joints[i].velocity[1],   
          LCR_racing.carBody.joints[i].velocity[2]);   

        TPE_Vec3 ja = carRight; // wheel axis of rotation

        if (i >= 2 && steering)
        {
          // for front wheels with turning we tilt the wheel axis 45 degrees

          TPE_Unit steer =
            (LCR_racing.wheelSteer * TPE_F) / LCR_GAME_UNIT;
 
          ja = TPE_vec3Normalized(
            TPE_vec3Plus(TPE_vec3Times(carForw,steer),carRight));
        }

        /* friction is in the direction if the axis and its magnitude is
          determined by the dot product (angle) of the axis and velocity */

        TPE_Vec3 fric = TPE_vec3Times(ja,(TPE_vec3Dot(ja,jv) *
          _LCR_applyMaterialFactor(
            LCR_racing.carDrifting ?
              (LCR_CAR_STEER_FRICTION * LCR_CAR_DRIFT_FACTOR) / 8 :
              LCR_CAR_STEER_FRICTION,LCR_racing.groundMaterial)) / TPE_F);

        driftFriction += TPE_vec3Len(fric);

        jv = TPE_vec3Minus(jv,fric); // subtract the friction

        LCR_racing.carBody.joints[i].velocity[0] = jv.x;
        LCR_racing.carBody.joints[i].velocity[1] = jv.y;
        LCR_racing.carBody.joints[i].velocity[2] = jv.z;
      }

    driftFriction /= 4; // divide by 4 wheels

    if (steering &&
      (input & (LCR_RACING_INPUT_FORW | LCR_RACING_INPUT_BACK)) &&
      (LCR_racing.wheelCollisions & 0x33))
    {
      /* When steering, also slightly spin the car. This helps fix the car
        getting "stuck" by its side to a wall when too close, becoming unable
        to unstick itself. */

      TPE_bodySpin(&LCR_racing.carBody,_LCR_TPE_vec3DividePlain(carUp,
        ((steering == (((input & LCR_RACING_INPUT_LEFT) != 0))) ==
        ((input & (LCR_RACING_INPUT_FORW)) != 0)) ? 100 : -100));
    }
  }

  if ((!LCR_racing.carDrifting) &&
    driftFriction > 
     (LCR_CAR_DRIFT_THRESHOLD_1 >> (( // back key initiates drift easily
      ((input & (LCR_RACING_INPUT_FORW | LCR_RACING_INPUT_BACK))
      == (LCR_RACING_INPUT_FORW | LCR_RACING_INPUT_BACK))) << 2)))
  {
    LCR_LOG1("drift start");
    LCR_racing.carDrifting = 1;
  }
  else if (LCR_racing.carDrifting &&
    (driftFriction < LCR_CAR_DRIFT_THRESHOLD_0 ||
    LCR_racingGetCarSpeedUnsigned() < 5))
  {
    LCR_LOG1("drift end");
    LCR_racing.carDrifting = 0;
  }

  if ((!(input & LCR_RACING_INPUT_LEFT)) &&
    (!(input & LCR_RACING_INPUT_RIGHT)))
    LCR_racing.wheelSteer /= 2;

  LCR_racing.wheelCollisions <<= 4;

  if (LCR_racing.fanForce)
  {
    TPE_bodyAccelerate(&(LCR_racing.carBody),TPE_vec3(0,LCR_racing.fanForce,0));

    LCR_racing.fanForce -= LCR_GRAVITY / LCR_FAN_FORCE_DECREASE;

    if (LCR_racing.fanForce < 0)
      LCR_racing.fanForce = 0;
  }

#if LCR_SETTING_CRASH_SOUNDS
  LCR_racing.crashState <<= 2;
#endif

  LCR_LOG2("stepping physics (start)");

  {
    /* To prevent many physics glitches we simulate the physics steps twice:
       once with normal physics and once with non-rotating (completely stiff)
       body. If the normal step works out wrong (car shape ends up wrong etc.),
       we fall back to the simple non-rotating step. */

    TPE_Joint joints[LCR_CAR_JOINTS];

    TPE_Vec3 linearVel = TPE_bodyGetLinearVelocity(&LCR_racing.carBody);

    /* Now back up the joints and also set all current joint velocities to the
       average linear velocity (since non-rotating bodies only consider the
       first joint's velocity, it must be the average of the whole body, else
       the car is super fast): */
    for (int i = 0; i < LCR_CAR_JOINTS; ++i)
    {
      joints[i] = LCR_racing.carBody.joints[i];
      LCR_racing.carBody.joints[i].velocity[0] = linearVel.x;
      LCR_racing.carBody.joints[i].velocity[1] = linearVel.y;
      LCR_racing.carBody.joints[i].velocity[2] = linearVel.z;
    }

    LCR_racing.carBody.flags |= TPE_BODY_FLAG_NONROTATING;

    TPE_worldStep(&(LCR_racing.physicsWorld)); // non-rotating step

    for (int i = 0; i < LCR_CAR_JOINTS; ++i) // remember the position and reset
    {
      TPE_Joint tmpJoint = LCR_racing.carBody.joints[i];
      LCR_racing.carBody.joints[i] = joints[i];

      if (LCR_racing.carBody.flags & TPE_BODY_FLAG_UNRESOLVED)
      {
        // if unsuccessful, keep the original pre-step pos. and decrease speed
        joints[i].velocity[0] /= 2;
        joints[i].velocity[1] /= 2;
        joints[i].velocity[2] /= 2;
      }
      else
      {
        // in normal case back up this step and revert the body total lin. vel.
        joints[i] = tmpJoint;
        joints[i].velocity[0] /= LCR_CAR_JOINTS;
        joints[i].velocity[1] /= LCR_CAR_JOINTS;
        joints[i].velocity[2] /= LCR_CAR_JOINTS;
      }
    }

    LCR_racing.carBody.flags &= ~TPE_BODY_FLAG_NONROTATING;
 
    TPE_worldStep(&(LCR_racing.physicsWorld)); // normal step

    uint8_t useSimplified = (LCR_racing.carBody.flags &
      TPE_BODY_FLAG_UNRESOLVED) != 0;

    if (!useSimplified && !_LCR_racingCarShapeOK())
    {
      useSimplified = 1;

      // we only do this with wheels on ground, because squeezed roof bugs here
      if ((LCR_racing.wheelCollisions & 0x0f) == 0x0f)
        for (int i = 0; i < 4 * TPE_RESHAPE_ITERATIONS; ++i)
        {
          LCR_LOG2("trying harder to resolve physics");

          TPE_bodyReshape(&LCR_racing.carBody,_LCR_racingEnvironmentFunction);

          if ((i % 4 == 0) && _LCR_racingCarShapeOK())
          {
            LCR_LOG2("succeeded in fixing physics");
            useSimplified = 0;
            break;
          }
        }
    }

    if (useSimplified)
    {
      LCR_LOG2("using simplified physics");

      for (int i = 0; i < LCR_CAR_JOINTS; ++i) // use the first step result
        LCR_racing.carBody.joints[i] = joints[i];
    }
  }

  LCR_LOG2("stepping physics (end)");

  LCR_racing.carSpeeds[1] = LCR_racing.carSpeeds[0];

  LCR_racing.carSpeeds[0] = (TPE_vec3Len(carVel) * LCR_GAME_UNIT)
    / LCR_PHYSICS_UNIT;

  if (TPE_vec3Dot(carVel,carForw) < 0)
    LCR_racing.carSpeeds[0] *= -1;

#if LCR_SETTING_CRASH_SOUNDS
  if (LCR_racing.crashState == 1)
    result |= LCR_RACING_EVENT_CRASH_SMALL;
  else if (LCR_racing.crashState == 3)
    result |= LCR_RACING_EVENT_CRASH_BIG;
#endif

  _LCR_racingUpdateCarPosRot();

  int carBlock[3];

  LCR_racingGetCarBlockCoords(carBlock);

  carBlock[0] = LCR_mapGetBlockAt(carBlock[0],carBlock[1],carBlock[2]);

  if (carBlock[0] >= 0)
  {
    if (LCR_currentMap.blocks[carBlock[0] * LCR_BLOCK_SIZE] ==
      LCR_BLOCK_CHECKPOINT_0)
    {
      LCR_currentMap.blocks[carBlock[0] * LCR_BLOCK_SIZE] =
        LCR_BLOCK_CHECKPOINT_1;

      result |= LCR_RACING_EVENT_CP_TAKEN;
    }
    else if (LCR_currentMap.blocks[carBlock[0] * LCR_BLOCK_SIZE] ==
      LCR_BLOCK_FINISH)
    {
      int valid = 1;

      for (int i = 0; i < LCR_currentMap.blockCount; ++i)
        if (LCR_currentMap.blocks[i * LCR_BLOCK_SIZE] == LCR_BLOCK_CHECKPOINT_0)
        {
          valid = 0;
          break;
        }

      if (valid)
      {
        result |= LCR_RACING_EVENT_FINISHED;

#if LCR_SETTING_REPLAY_MAX_SIZE > 0
        if (!LCR_racing.replay.on)
          LCR_replayRecordEvent(LCR_racing.tick,LCR_REPLAY_EVENT_END);
#endif
      }
    }
  }

  LCR_racing.tick += LCR_racing.tick < 0xffffffff; // disallow overflow

  LCR_LOG2("racing step (end)");

  return result;
}

/**
  Draws a simple 3D debug overlap of the physics world.
*/
void LCR_physicsDebugDraw(LCR_GameUnit camPos[3], LCR_GameUnit camRot[2],
  LCR_GameUnit camFov, void (*drawPixel)(uint16_t, uint16_t, uint8_t))
{
#if LCR_SETTING_DEBUG_PHYSICS_DRAW
  LCR_LOG2("drawing physics debug");

  TPE_Vec3 cPos, cRot, cView;

  cPos.x = (camPos[0] * LCR_PHYSICS_UNIT) / LCR_GAME_UNIT;
  cPos.y = (camPos[1] * LCR_PHYSICS_UNIT) / LCR_GAME_UNIT;
  cPos.z = (camPos[2] * LCR_PHYSICS_UNIT) / LCR_GAME_UNIT;

  cRot.x = (camRot[0] * TPE_F) / LCR_GAME_UNIT;
  cRot.y = (camRot[1] * TPE_F) / LCR_GAME_UNIT;
  cRot.z = 0; 

  cView.x = LCR_EFFECTIVE_RESOLUTION_X;
  cView.y = LCR_EFFECTIVE_RESOLUTION_Y;
  cView.z = (camFov * TPE_F) / LCR_GAME_UNIT;

  TPE_worldDebugDraw(&(LCR_racing.physicsWorld),drawPixel,
    cPos,cRot,cView,16,LCR_PHYSICS_UNIT / 4,LCR_racing.tick * 4);
#endif
}

/**
  Validates replay, i.e. checks if it finishes and whether its achieved time
  agrees with the stored time. The function can only be called when both the
  replay and the correct map are loaded. The loaded replay's playing position
  will be reset after this function returns.
*/
int LCR_replayValidate(void)
{
  LCR_LOG1("validating replay");

#if LCR_SETTING_REPLAY_MAX_SIZE != 0
  int result = 0;

  LCR_racingRestart(1);

  while (LCR_racing.tick < 0x00f00000 // no infinite loops
    && LCR_racing.tick <= LCR_racing.replay.achievedTime)
  {
    int coords[3];

    LCR_racingStep(0);
    LCR_racingGetCarBlockCoords(coords);

    coords[0] = LCR_mapGetBlockAt(coords[0],coords[1],coords[2]);

    if (coords[0] && LCR_currentMap.blocks[coords[0] * LCR_BLOCK_SIZE] ==
      LCR_BLOCK_FINISH)
    {
      uint8_t allCPsTaken = 1; // reuse as variable

      for (int i = 0; i < LCR_currentMap.blockCount; ++i)
        if (LCR_currentMap.blocks[i * LCR_BLOCK_SIZE] == LCR_BLOCK_CHECKPOINT_0)
        {
          allCPsTaken = 0;
          break;
        }

      if (allCPsTaken)
      {
        result = LCR_racing.tick == (LCR_racing.replay.achievedTime + 1);
        break;
      }
    }
  }

  if (result)
  {
    LCR_LOG1("replay valid");
  }
  else
  {
    LCR_LOG1("replay invalid!");
  }

  LCR_racingRestart(1);

  return result;
#else
  return 0;
#endif
}

#endif // guard
