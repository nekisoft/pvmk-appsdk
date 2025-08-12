#ifndef _LCR_SETTINGS_H
#define _LCR_SETTINGS_H

/** @file settings.h

  Licar: settings

  Compile time settings file for all modules, values here may be changed by the
  user or overriden by frontend before compilation.
*/

#ifndef LCR_SETTING_RESOLUTION_X
  /** Horizontal rendering resolution in pixels. */
  #define LCR_SETTING_RESOLUTION_X 1024
#endif

#ifndef LCR_SETTING_RESOLUTION_Y
  /** Vertical rendering resolution in pixels. */
  #define LCR_SETTING_RESOLUTION_Y 768
#endif

#ifndef LCR_SETTING_RESOLUTION_SUBDIVIDE
  /** Subdivides the whole game's effective resolution by this value, by making
    each pixel this number of times bigger. This can drastically improve
    performance. */
  #define LCR_SETTING_RESOLUTION_SUBDIVIDE 1
#endif

#ifndef LCR_SETTING_FPS
  /** Rendering frames per second. Note this only applies to graphics, NOT
    physics. */
  #define LCR_SETTING_FPS 30
#endif

#ifndef LCR_SETTING_CAMERA_FOCAL_LENGTH
  /** Adjusts camera's focal length, which affects field of view (FOV). The
    value must be in range from 1 to 32. */
  #define LCR_SETTING_CAMERA_FOCAL_LENGTH 17
#endif

#ifndef LCR_SETTING_FREE_CAMERA_SPEED
  /** Move speed of free camera, in 1/8ths of block length. */
  #define LCR_SETTING_FREE_CAMERA_SPEED 50
#endif

#ifndef LCR_SETTING_FREE_CAMERA_TURN_SPEED
  /** Turn speed of free camera, in degrees per second. */
  #define LCR_SETTING_FREE_CAMERA_TURN_SPEED 180
#endif

#ifndef LCR_SETTING_MAX_MAP_VERTICES
  /** Maximum number of vertices for 3D rendering. Lower number will decrease
  RAM usage but also prevent larger maps from being loaded. */
  #define LCR_SETTING_MAX_MAP_VERTICES 4096
#endif

#ifndef LCR_SETTING_MAX_MAP_TRIANGLES
  /** Like LCR_SETTING_MAX_MAP_VERTICES but for the number of triangles. */
  #define LCR_SETTING_MAX_MAP_TRIANGLES 8192
#endif

#ifndef LCR_SETTING_SKY_SIZE
  /** Size of sky texture pixel, 0 turns off sky rendering. */
  #define LCR_SETTING_SKY_SIZE 2
#endif

#ifndef LCR_SETTING_MAP_MAX_BLOCKS
  /** Maximum number of blocks a map can consist of, decreasing will save RAM
    but also rule out loading bigger maps. */
  #define LCR_SETTING_MAP_MAX_BLOCKS 4096
#endif

#ifndef LCR_SETTING_CULLING_PERIOD
  /** This says how often (after how many blocks added) during the map loading
    the map model triangles and vertices will be culled. This value may affect
    how quickly maps load. */
  #define LCR_SETTING_CULLING_PERIOD 256
#endif

#ifndef LCR_SETTING_TEXTURE_SUBSAMPLE
  /** Sets texture subsampling: 0 means no subsampling, higher value N means a
    texture will be sampled once per N rasterized pixels. Higher value can
    increase performance. */
  #define LCR_SETTING_TEXTURE_SUBSAMPLE 4
#endif

#ifndef LCR_SETTING_LOD_DISTANCE
  /** Distance in game squares from which LOD will be drawn. Value 64 or higher
    turns off LOD completely. Note that this doesn't affect rendering distance
    of 3D models. */
  #define LCR_SETTING_LOD_DISTANCE 20
#endif

#ifndef LCR_SETTING_LOD_COLOR
  /** RGB565 color of LOD blocks in the distance. */
  #define LCR_SETTING_LOD_COLOR 0x4229
#endif

#ifndef LCR_SETTING_DISPLAY_HUD
  /** Whether to display game HUD. */
  #define LCR_SETTING_DISPLAY_HUD 1
#endif

#ifndef LCR_SETTING_CAR_ANIMATION_SUBDIVIDE
  /** How many frames will be used to complete whole animation of the car model.
    0 turns off car animation completely (may be faster and smaller), 1 turns on
    highest quality animation, higher values lower animation quality and may
    increase performance. */
  #define LCR_SETTING_CAR_ANIMATION_SUBDIVIDE 2
#endif

#ifndef LCR_SETTING_CAMERA_HEIGHT
  /** Base height of the car follow camera, in 4ths of map block height. */
  #define LCR_SETTING_CAMERA_HEIGHT 5
#endif

#ifndef LCR_SETTING_CAMERA_HEIGHT_BAND
  /** Size of height band of the follow camera, in same units as base height. */
  #define LCR_SETTING_CAMERA_HEIGHT_BAND 1
#endif

#ifndef LCR_SETTING_CAMERA_DISTANCE
  /** Base horizontal distance of the car follow camera, in 4ths of map block
    width. */
  #define LCR_SETTING_CAMERA_DISTANCE 4
#endif

#ifndef LCR_SETTING_CAMERA_DISTANCE_BAND
  /** Band for distance of the car follow camera, in same units as base dist. */
  #define LCR_SETTING_CAMERA_DISTANCE_BAND 2
#endif

#ifndef LCR_SETTING_GHOST_COLOR
  /** Color of the ghost car (in RGB565). */
  #define LCR_SETTING_GHOST_COLOR 0xf79e
#endif

#ifndef LCR_SETTING_CHECKPOINT_0_COLOR
  /** Color of untaken checkpoint (in RGB565). */
  #define LCR_SETTING_CHECKPOINT_0_COLOR 0x37e0
#endif

#ifndef LCR_SETTING_CHECKPOINT_1_COLOR
  /** Color of taken checkpoint (in RGB565). */
  #define LCR_SETTING_CHECKPOINT_1_COLOR 0xdefb
#endif

#ifndef LCR_SETTING_FINISH_COLOR
  /** Color of finish block (in RGB565). */
  #define LCR_SETTING_FINISH_COLOR 0xf900
#endif

#ifndef LCR_SETTING_SMOOTH_ANIMATIONS
  /** Whether to smooth out animations (car physics, camera movement etc.). */
  #define LCR_SETTING_SMOOTH_ANIMATIONS 1
#endif

#ifndef LCR_SETTING_LOG_LEVEL
  /** How detailed the console logs should be. 0 turns off logging, 1 means
  normal, 2 more detailed etc. Setting high log level may result in spam and
  slower game, but is useful for debugging. */
  #define LCR_SETTING_LOG_LEVEL 1
#endif

#ifndef LCR_SETTING_CMS_PER_BLOCK
  /** How many centimeters one game block is considered to measure
    (horizontally). This is for calculating speed etc. */
  #define LCR_SETTING_CMS_PER_BLOCK 400
#endif

#ifndef LCR_SETTING_DEBUG_PHYSICS_DRAW
  /** If on, physics world will be drawn. */
  #define LCR_SETTING_DEBUG_PHYSICS_DRAW 0
#endif

#ifndef LCR_SETTING_POTATO_GRAPHICS
  /** Setting this will turn on very simple graphics without textures and etc.,
    can be good for very weak devices. */
  #define LCR_SETTING_POTATO_GRAPHICS 0
#endif

#ifndef LCR_SETTING_332_COLOR
  /** If turned on, pixel colors will be converted to RGB332 format from the
    default RGB565 (so that only the lower byte of a color is significant). */
  #define LCR_SETTING_332_COLOR 0
#endif

#ifndef LCR_SETTING_MUSIC
  /** Whether to enable in game music. */
  #define LCR_SETTING_MUSIC 1
#endif

#ifndef LCR_SETTING_ENABLE_DATA_FILE
  /** May be used to disable use of the user data file. */
  #define LCR_SETTING_ENABLE_DATA_FILE 1
#endif

#ifndef LCR_SETTING_COUNTDOWN_MS
  /** Run start countdown length in milliseconds. */
  #define LCR_SETTING_COUNTDOWN_MS 2200
#endif

#ifndef LCR_SETTING_MAP_CHUNK_RELOAD_INTERVAL
  /** Interval in rendering frames of reloading map chunks, should ideally be
    kept a power of two, can't be 0. */
  #define LCR_SETTING_MAP_CHUNK_RELOAD_INTERVAL 16
#endif

#ifndef LCR_SETTING_REPLAY_MAX_SIZE
  /** Says the maximum size of a replay (in replay events). The value 0 will
    turn replays off completely. */
  #define LCR_SETTING_REPLAY_MAX_SIZE 256
#endif

#ifndef LCR_SETTING_HORIZON_SHIFT
  /** Vertical offset of the background sky image in percents. */
  #define LCR_SETTING_HORIZON_SHIFT 200
#endif

#ifndef LCR_SETTING_TIME_MULTIPLIER
  /** Multiplies speed of time by this percentage, doesn't affect physics. */
  #define LCR_SETTING_TIME_MULTIPLIER 100
#endif

#ifndef LCR_SETTING_GHOST_STEP
  /** Step (in physics engine ticks) by which the samples for ghost car will be
  spaced (positions inbetween will be interpolated). Lower step along with more
  ghost samples should result in more accurate ghost animation, but will eat up
  more memory. This should ideally be kept a power of two. */
  #define LCR_SETTING_GHOST_STEP 16
#endif

#ifndef LCR_SETTING_GHOST_MAX_SAMPLES
  /** Maximum number of samples the ghost car will be able to use. Higher value
  should generally result in more accurate ghost animation, but will eat up more
  memory. Value 0 disables ghosts. */
  #define LCR_SETTING_GHOST_MAX_SAMPLES 128
#endif

#ifndef LCR_SETTING_CAR_RENDER_DISTANCE
  /** Distance in blocks at which player and ghost car will be seen. */
  #define LCR_SETTING_CAR_RENDER_DISTANCE 30
#endif

#ifndef LCR_SETTING_CRASH_SOUNDS
  /** Whether or not to detect crashes and play their sounds. */
  #define LCR_SETTING_CRASH_SOUNDS 1
#endif

#ifndef LCR_SETTING_POPUP_DURATION
  /** Duration of popup messages in milliseconds. */
  #define LCR_SETTING_POPUP_DURATION 2500
#endif

#ifndef LCR_SETTING_CAR_TINT
  /** Tints the car's texture, in lowest 3 bits says which of the RGB
    components should be retained (1) and which diminished (0). Value 0x07
    leaves the car untinted (car have slightly better performance). */
  #define LCR_SETTING_CAR_TINT 0x07
#endif

#ifndef LCR_SETTING_PARTICLES
  /** Can turn particle effects on/off. */
  #define LCR_SETTING_PARTICLES 1
#endif

#ifndef LCR_SETTING_PARTICLE_COLOR_DRIFT
  /** Color of drifting particle effect. */
  #define LCR_SETTING_PARTICLE_COLOR_DRIFT 0x4208
#endif

#ifndef LCR_SETTING_PARTICLE_COLOR_GRASS
  /** Color of grass particle effect. */
  #define LCR_SETTING_PARTICLE_COLOR_GRASS 0x5467
#endif

#ifndef LCR_SETTING_PARTICLE_COLOR_DIRT
  /** Color of dirt particle effect. */
  #define LCR_SETTING_PARTICLE_COLOR_DIRT 0x8b84
#endif

#ifndef LCR_SETTING_CAR_SHADOW
  /** Turns on/off a simple car shadow. */
  #define LCR_SETTING_CAR_SHADOW 1
#endif

#ifndef LCR_SETTING_FOG
  /** Turns distance fog on/off. Turning this off may increase performance. */
  #define LCR_SETTING_FOG 0
#endif

#ifndef LCR_SETTING_ONLY_SMALL_MAPS
  /** Turning this on will only include the small maps in the internal data
    file. This option exists for weak devices that couldn't handle big maps
    and/or have to reduce the size of the internal data file. */
  #define LCR_SETTING_ONLY_SMALL_MAPS 0
#endif

#endif // guard
