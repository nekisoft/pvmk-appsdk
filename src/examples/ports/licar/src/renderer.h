#ifndef _LCR_RENDERER_H
#define _LCR_RENDERER_H

/** @file renderer.h

  Licar: renderer module

  This implements 3D and 2D rendering. It should be possible to replace this
  module with another one to get let's say a GPU accelerated OpenGL renderer.

  Some comments:

  - The module uses small3dlib, a tiny software rasterization library. This
    module knows nothing about I/O (windows, canvases, ...), it just says where
    to draw pixels and what colors they should be.
  - The map 3D model is divided into chunks 4 times in each dimension, i.e.
    there are 64 chunks in total, out of which only 8 are loaded at any time,
    depending on where the camera is located and where it is looking. This is to
    save CPU time, we don't draw the far away chunks or those behind the camera.
  - Extremely simple LOD of far away chunks is implemented: we keep an 8x8x8
    bit array of where there is empty space and where there is "something", then
    for far away areas with "something" we just draw some 2D rectangles. This
    mostly helps orientation in areas where there are no 3D models nearby.
  - RENDERING IS THE BOTTLENECK OF PERFORMANCE, it takes even much more time
    than physics simulation, i.e. care should be taken to make code here very
    optimized, namely the _LCR_pixelFunc3D function, as it is called for every
    rasterized pixel.
*/

#define S3L_RESOLUTION_X LCR_EFFECTIVE_RESOLUTION_X
#define S3L_RESOLUTION_Y LCR_EFFECTIVE_RESOLUTION_Y
#define S3L_PIXEL_FUNCTION _LCR_pixelFunc3D

#ifndef S3L_PERSPECTIVE_CORRECTION
  #define S3L_PERSPECTIVE_CORRECTION 2
#endif

#ifndef S3L_NEAR_CROSS_STRATEGY
  #define S3L_NEAR_CROSS_STRATEGY 1
#endif

#define LCR_FONT_PIXEL_SIZE (1 + LCR_EFFECTIVE_RESOLUTION_X / 512)
#define LCR_ANIMATE_CAR (LCR_SETTING_CAR_ANIMATION_SUBDIVIDE != 0)
#define LCR_ANT_RESOLUTION (LCR_EFFECTIVE_RESOLUTION_Y <= 110)

#if LCR_SETTING_POTATO_GRAPHICS
  #undef  S3L_PERSPECTIVE_CORRECTION
  #define S3L_PERSPECTIVE_CORRECTION 0
  #define S3L_NEAR_CROSS_STRATEGY 1
  #define S3L_FLAT 1

  #ifndef S3L_Z_BUFFER
    #define S3L_Z_BUFFER 2 // simplified
  #endif

  #ifndef S3L_MAX_TRIANGES_DRAWN
    #define S3L_MAX_TRIANGES_DRAWN 64 // lower, in case sorting was turned on
  #endif
#else
  #define S3L_Z_BUFFER 1
#endif
 
#define S3L_NEAR (10 * (S3L_F / 16)) // too low value will cause overflows
#include "small3dlib.h"

#ifdef LCR_MODULE_NAME
  #undef LCR_MODULE_NAME
#endif

#define LCR_MODULE_NAME "rend"

/// Renderer specific unit, length of one map square.
#define LCR_RENDERER_UNIT (S3L_F)

#define LCR_RENDERER_CHUNK_RESOLUTION 4  // do not change
#define LCR_RENDERER_LOD_BLOCKS       64 // do not change

#define LCR_RENDERER_CHUNK_SIZE_HORIZONTAL \
  ((LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT) / LCR_RENDERER_CHUNK_RESOLUTION)

#define LCR_RENDERER_CHUNKS_TOTAL (LCR_RENDERER_CHUNK_RESOLUTION * \
  LCR_RENDERER_CHUNK_RESOLUTION * LCR_RENDERER_CHUNK_RESOLUTION)

#define LCR_RENDERER_MODEL_COUNT 11

#define LCR_RENDERER_CAR_SCALE (LCR_RENDERER_UNIT / 4)

#define LCR_RENDERER_FONT_SEGMENT_SIZE (2 + LCR_EFFECTIVE_RESOLUTION_Y / 320)

/** For some reason the map model is a bit misaligned with physics world, this
  kinda hotfixes it -- later try to discover source of this bug. TODO */
#define _LCR_MAP_MODEL_SCALE 1034

#define LCR_RENDERER_MAT_CP0 0x0f ///< material for untaken checkpoint
#define LCR_RENDERER_MAT_CP1 0x0e
#define LCR_RENDERER_MAT_FIN 0x0d

struct
{
  S3L_Scene scene;          ///< Whole 3D scene.
  S3L_Model3D mapModel;     ///< Whole map model.
  S3L_Model3D *carModel;    ///< Shortcut ptr to the car model in the scene.
  S3L_Model3D *ghostModel;  ///< Shortcut ptr to the ghost model in the scene.
  S3L_Model3D *shadowModel;

  /**
    The scene model array.
    0, 1, 2, 3, 4, 5, 6, 7: nearest map chunk models
    8:                      car model
    9:                      ghost model
    10:                     shadow model
  */
  S3L_Model3D models[LCR_RENDERER_MODEL_COUNT];

  uint_fast16_t pixelColor; /**< Holds pixel color for _LCR_pixelFunc3D. This
                                 is needed when texture subsampling is on. */

  uint32_t frame;
  uint8_t loadedChunks[8]; ///< Numbers of loaded map chunks.

  S3L_Unit mapVerts[LCR_SETTING_MAX_MAP_VERTICES * 3];
  S3L_Index mapTris[LCR_SETTING_MAX_MAP_TRIANGLES * 3];

  S3L_Index chunkStarts[LCR_RENDERER_CHUNKS_TOTAL];

  /**
    Additional data for triangles. 4 higher bits hold direction (for lighting):
    0 is floor, 1 is wall, 2 is wall (90 degrees). 4 lower bits hold the
    texture index.
  */
  uint8_t mapTriangleData[LCR_SETTING_MAX_MAP_TRIANGLES];

  /**
    8x8x8 3D grid of bits, each bit says (for each corresponding part of map)
    whether there is an LOD block or not.
  */
  uint8_t gridOfLODs[LCR_RENDERER_LOD_BLOCKS];

#if LCR_ANIMATE_CAR
  S3L_Unit wheelRotation;
  S3L_Unit wheelSteer;
  S3L_Unit wheelRotationCenters[4]; /**< back and front wheel XY centers */
  S3L_Unit animatedCarVerts[LCR_CAR_VERTEX_COUNT * 3];
#endif

  // pixel function precomputed values:
  uint32_t previousTriID;
  int triUVs[6];
  uint_fast16_t texSubsampleCount;
  uint_fast16_t flatAndTransparent; /**< If non-zero, transparent (dithered)
                                         polygons will be drawn without texture,
                                         with color stored in this variable. */
#if LCR_SETTING_PARTICLES
  uint_fast16_t particleColor;      ///< 0x0000 means no particles active.
#endif
} LCR_renderer;

/**
  Sets particle effects drawn around wheels. The parameter says particle color,
  0x0000 turns particles off.
*/
void LCR_rendererSetParticles(uint16_t color)
{
#if LCR_SETTING_PARTICLES
  LCR_renderer.particleColor = color;
#else
  return;
#endif
}

void _LCR_rendererSetModelTransform(S3L_Model3D *model,
  LCR_GameUnit position[3], LCR_GameUnit rotation[3])
{
  model->transform.translation.x =
    (position[0] * LCR_RENDERER_UNIT) / LCR_GAME_UNIT;
  model->transform.translation.y =
    (position[1] * LCR_RENDERER_UNIT) / LCR_GAME_UNIT;
  model->transform.translation.z =
    (position[2] * LCR_RENDERER_UNIT) / LCR_GAME_UNIT;

  model->transform.rotation.x = S3L_wrap((rotation[0] *
    S3L_F) / LCR_GAME_UNIT,S3L_F);
  model->transform.rotation.y = S3L_wrap((rotation[1] *
    S3L_F) / LCR_GAME_UNIT,S3L_F);
  model->transform.rotation.z = S3L_wrap((rotation[2] *
    S3L_F) / LCR_GAME_UNIT,S3L_F);
}

void LCR_rendererSetCarTransform(LCR_GameUnit position[3],
  LCR_GameUnit rotation[3])
{
  LCR_LOG2("setting car transform");
  _LCR_rendererSetModelTransform(LCR_renderer.carModel,position,rotation);
}

void LCR_rendererSetGhostTransform(LCR_GameUnit position[3],
  LCR_GameUnit rotation[3])
{
  LCR_LOG2("setting ghost transform");
  _LCR_rendererSetModelTransform(LCR_renderer.ghostModel,position,rotation);
}

void LCR_rendererSetCarVisibility(uint8_t visible)
{
  LCR_renderer.carModel->config.visible = visible;
}

void LCR_rendererSetGhostVisibility(uint8_t visible)
{
  LCR_renderer.ghostModel->config.visible = visible;
}

void _LCR_rendererDrawFontPixel(int x, int y, uint16_t color)
{
#if LCR_FONT_PIXEL_SIZE == 1
  LCR_gameDrawPixelXYSafe(x,y,color);
#else
  if ((x >= 0) && (y >= 0) &&
    (x < LCR_EFFECTIVE_RESOLUTION_X - LCR_FONT_PIXEL_SIZE) &&
    (y < LCR_EFFECTIVE_RESOLUTION_Y - LCR_FONT_PIXEL_SIZE))
    for (int i = x; i < x + LCR_FONT_PIXEL_SIZE; ++i)
      for (int j = y; j < y + LCR_FONT_PIXEL_SIZE; ++j)
        LCR_gameDrawPixelXYSafe(i,j,color);
#endif
}

int _LCR_rendererGimbalDanger(S3L_Unit angle)
{
  return S3L_min(
    S3L_abs(LCR_renderer.carModel->transform.rotation.x - S3L_F / 4),
    S3L_abs(LCR_renderer.carModel->transform.rotation.x - (S3L_F - S3L_F / 4)))
    < S3L_F / 100;
}

int LCR_rendererComputeTextWidth(const char *text, int size)
{
  int r = 0;
  size *= LCR_RENDERER_FONT_SEGMENT_SIZE;

  while (*text)
  {
    r += 2 * size;

    if (text[1])
      r += 3 * size / 4;

    text++;
  }

  return r + LCR_FONT_PIXEL_SIZE - 1;
}

int LCR_rendererComputeTextHeight(int size)
{
  return 2 * size * LCR_RENDERER_FONT_SEGMENT_SIZE + LCR_FONT_PIXEL_SIZE - 1;
}

void LCR_rendererDrawText(const char *text, int x, int y, uint16_t color,
  int size)
{
  size *= LCR_RENDERER_FONT_SEGMENT_SIZE;

  while (*text)
  {
    uint16_t c = LCR_getFontChar(*text);

    for (int b = 0; b < 3; ++b) // horizontal and vertical segments
      for (int a = 0; a < 2; ++a)
      {
        if (c & 0x01)
          for (int i = 0; i <= size; ++i)
            _LCR_rendererDrawFontPixel(x + a * size + i,y + b * size,color);

        if (c & 0x02)
          for (int i = 0; i <= size; ++i)
            _LCR_rendererDrawFontPixel(x + b * size,y + a * size + i,color);

        c >>= 2;
      }

    for (int b = 0; b < 2; ++b) // diagonal segments
      for (int a = 0; a < 2; ++a)
      {
        if (c & 0x01)
          for (int i = 0; i <= size; ++i)
            _LCR_rendererDrawFontPixel(x + a * size + i,y + b * size + i,color);

        c >>= 1;
      }

    x += 2 * size + 3 * size / 4;
    text++;
  }
}

/**
  Guesses (not 100% accurately) if a quad is convex or not.
*/
int _LCR_rendererQuadLooksConvex(S3L_Unit quad[8])
{
  S3L_Unit cx = (quad[0] + quad[2] + quad[4] + quad[6]) / 4;
  S3L_Unit cy = (quad[1] + quad[3] + quad[5] + quad[7]) / 4;
  S3L_Unit r = 0;

  for (int i = 0; i < 8; i += 2)
  {
    r += S3L_abs(cx - quad[i]);
    r += S3L_abs(cy - quad[i + 1]);
  }

  r = (3 * r) / 32;

  for (int i = 0; i < 8; i += 2)
    if ((S3L_abs(cx - quad[i]) <= r) && (S3L_abs(cy - quad[i + 1]) <= r))
      return 0;

  return 1;
}

/**
  Used as a fragment shader by small3dlib. This function will be called for
  every rasterized 3D pixel, we use it write actual pixels to the screen with
  shading, texturing etc.
*/
void _LCR_pixelFunc3D(S3L_PixelInfo *pixel)
{
#if LCR_SETTING_POTATO_GRAPHICS
  // simple shader for simplified graphics

  if (pixel->triangleID != LCR_renderer.previousTriID)
  {
    LCR_renderer.previousTriID = pixel->triangleID;
    LCR_renderer.flatAndTransparent = 0x630c; // base gray

    if (pixel->modelIndex < 8)
    {
      uint8_t tData = (LCR_renderer.mapTriangleData +
        LCR_renderer.chunkStarts[LCR_renderer.loadedChunks[
          pixel->modelIndex]])[pixel->triangleIndex];

      switch (tData & 0x0f)
      {
        case 3: LCR_renderer.flatAndTransparent &= ~(0x3008); break; // grass
        case 4: LCR_renderer.flatAndTransparent &= ~(0x0408); break; // mud
        case 5: LCR_renderer.flatAndTransparent |= 0x0010;    break; // ice
        case 6: LCR_renderer.flatAndTransparent |= 0x8080;    break; // acc
        case 7: LCR_renderer.flatAndTransparent &= ~(0x4208); break; // fan

        case LCR_RENDERER_MAT_CP0:
          LCR_renderer.flatAndTransparent = LCR_SETTING_CHECKPOINT_0_COLOR;
          break;

        case LCR_RENDERER_MAT_CP1:
          LCR_renderer.flatAndTransparent = LCR_SETTING_CHECKPOINT_1_COLOR;
          break;

        case LCR_RENDERER_MAT_FIN:
          LCR_renderer.flatAndTransparent = LCR_SETTING_FINISH_COLOR;
          break;

        default: break;
      }

      tData &= 0x30; // isolate type

      LCR_renderer.flatAndTransparent |= (((uint16_t) (tData)) >> 4) |
        (((uint16_t) (tData)) << 2) | (((uint16_t) (tData)) << 7);
    }
    else
      LCR_renderer.flatAndTransparent >>= 1; // car, darken

    // alter each triangle's color slightly:
    LCR_renderer.flatAndTransparent += pixel->triangleIndex % 4;

#if LCR_SETTING_332_COLOR
    LCR_renderer.flatAndTransparent =
      LCR_CONVERT_COLOR(LCR_renderer.flatAndTransparent);
#endif
  }

  LCR_gameDrawPixelXYUnsafe(pixel->x,pixel->y,LCR_renderer.flatAndTransparent);

#else // LCR_SETTING_POTATO_GRAPHICS
  // New triangle? Precompute stuff for it:
  if (pixel->triangleID != LCR_renderer.previousTriID)
  {
    LCR_renderer.previousTriID = pixel->triangleID;
    LCR_renderer.flatAndTransparent = 0;

#if LCR_SETTING_TEXTURE_SUBSAMPLE != 0
    LCR_renderer.texSubsampleCount = 0;
#endif

    if (pixel->modelIndex == 8)
    {
      // car model
      LCR_loadImage(LCR_IMAGE_CAR);

      for (int i = 0; i < 6; ++i)
        LCR_renderer.triUVs[i] = (LCR_carUvs[2 * LCR_carTriangleUvs[3 *
          pixel->triangleIndex + i / 2] + i % 2] * (LCR_IMAGE_SIZE + i % 2)) / 512;

#if (LCR_SETTING_CAR_TINT & 0x7) != 0x07
      for (int i = 0; i < 256; ++i)
      {
        LCR_currentImage.palette[i] &=
  #if LCR_SETTING_CAR_TINT & 0x01
          0x001f |
  #else
          0x0003 |
  #endif
  #if LCR_SETTING_CAR_TINT & 0x02
          0x07e0 |
  #else
          0x00e0 |
  #endif
  #if LCR_SETTING_CAR_TINT & 0x04
          0xf800
  #else
          0x1800
  #endif
          ;
#if LCR_SETTING_332_COLOR
        LCR_currentImage.palette[i] =
          LCR_CONVERT_COLOR(LCR_currentImage.palette[i]);
#endif
      }
#endif
    }
    else if (pixel->modelIndex == 9)
    {
      // car ghost model
      LCR_renderer.flatAndTransparent = LCR_SETTING_GHOST_COLOR;
    }
    else if (pixel->modelIndex == 10)
    {
      // shadow model
      LCR_renderer.flatAndTransparent = 0x0001;
    }
    else
    {
      // map model
      S3L_Unit *v[3];
      const S3L_Index *t = LCR_renderer.models[pixel->modelIndex].triangles +
        3 * pixel->triangleIndex;

      for (int i = 0; i < 3; ++i)
        v[i] = LCR_renderer.mapVerts + 3 * t[i];

      const uint8_t *triData = LCR_renderer.mapTriangleData +
        LCR_renderer.chunkStarts[LCR_renderer.loadedChunks[pixel->modelIndex]];

      uint8_t type = triData[pixel->triangleIndex] >> 4;
      uint8_t mat = triData[pixel->triangleIndex] & 0x0f;

#if LCR_SETTING_CAR_SHADOW
      if (type == 0 && (S3L_abs((2 * v[0][0] + v[1][0] + v[2][0]) / 4 -
          LCR_renderer.carModel->transform.translation.x) +
        S3L_abs((2 * v[0][2] + v[1][2] + v[2][2]) / 4 -
          LCR_renderer.carModel->transform.translation.z)) < LCR_RENDERER_UNIT)
      {
        S3L_Unit groundH = (2 * v[0][1] + v[1][1] + v[2][1]) / 4;

        if (groundH < LCR_renderer.carModel->transform.translation.y &&
          groundH > LCR_renderer.shadowModel->transform.translation.y)
        {
          LCR_renderer.shadowModel->transform.translation.y = S3L_min(groundH,
            LCR_renderer.carModel->transform.translation.y
            - LCR_RENDERER_UNIT / 8);

          LCR_renderer.shadowModel->transform.scale.x = S3L_max(0,
            LCR_RENDERER_UNIT - (LCR_renderer.carModel->transform.translation.y
            - groundH) / 4);

          LCR_renderer.shadowModel->transform.scale.y =
            LCR_renderer.shadowModel->transform.scale.x;

          LCR_renderer.shadowModel->transform.scale.z =
            LCR_renderer.shadowModel->transform.scale.x;
        }
      }
#endif

      switch (mat)
      {
#define CL (type ? 0x8210 : 0x0000)
        case LCR_RENDERER_MAT_CP0:
          LCR_renderer.flatAndTransparent = LCR_SETTING_CHECKPOINT_0_COLOR | CL;
          LCR_renderer.flatAndTransparent =
            LCR_CONVERT_COLOR(LCR_renderer.flatAndTransparent); 
          break;

        case LCR_RENDERER_MAT_CP1:
          LCR_renderer.flatAndTransparent = LCR_SETTING_CHECKPOINT_1_COLOR | CL;
          LCR_renderer.flatAndTransparent =
            LCR_CONVERT_COLOR(LCR_renderer.flatAndTransparent); 
          break;

        case LCR_RENDERER_MAT_FIN:
          LCR_renderer.flatAndTransparent = LCR_SETTING_FINISH_COLOR | CL;
          LCR_renderer.flatAndTransparent =
            LCR_CONVERT_COLOR(LCR_renderer.flatAndTransparent); 
          break;
#undef CL

        default:
          LCR_loadImage(mat);

          if (type == 0) // floor?
          {
            if (v[0][1] != v[1][1] || v[1][1] != v[2][1]) // angled floor?
              LCR_imageChangeBrightness(1);

            for (int i = 0; i < 6; ++i)
              LCR_renderer.triUVs[i] = ((
                (v[i / 2][(i % 2) * 2]) *
                  LCR_IMAGE_SIZE) / LCR_RENDERER_UNIT);
          }
          else
          {
            if (type == 1)
              LCR_imageChangeBrightness(0);

            for (int i = 0; i < 6; ++i)
            {
              LCR_renderer.triUVs[i] = ((
                (v[i / 2][(i % 2) ? 1 : (type == 1 ? 2 : 0)]) *
                  LCR_IMAGE_SIZE) / LCR_RENDERER_UNIT);

              if (i % 2)
                LCR_renderer.triUVs[i] = LCR_IMAGE_SIZE -
                  LCR_renderer.triUVs[i];
            }
          }

#if LCR_SETTING_FOG
          if (pixel->depth > (S3L_MAX_DEPTH >> 18))
            LCR_imageChangeBrightness(0);
#endif

          // shift the UVs to the origin (prevent high values of UV coords)
          for (int i = 0; i < 2; ++i)
          {
            uint8_t minCoord = LCR_renderer.triUVs[i] <
              LCR_renderer.triUVs[2 + i] ? (0 + i) : (2 + i);

            if (LCR_renderer.triUVs[4 + i] < LCR_renderer.triUVs[minCoord])
              minCoord = 4 + i;

            S3L_Unit shiftBy = LCR_renderer.triUVs[minCoord] % LCR_IMAGE_SIZE;

            if (shiftBy < 0)
              shiftBy += LCR_IMAGE_SIZE;

            shiftBy -= LCR_renderer.triUVs[minCoord];

            LCR_renderer.triUVs[i] += shiftBy;
            LCR_renderer.triUVs[2 + i] += shiftBy;
            LCR_renderer.triUVs[4 + i] += shiftBy;
          }

          break;
      }
    }
  }

  /* Bottleneck: code from here below will be ran for every rasterized pixel,
     optimizing it may significantly improve rendering performance. */

  if (LCR_renderer.flatAndTransparent)
  {
    if ((pixel->x % 2) == (pixel->y % 2))
      LCR_gameDrawPixelXYUnsafe(
        pixel->x,pixel->y,LCR_renderer.flatAndTransparent);
    else
      S3L_zBufferWrite(pixel->x,pixel->y,S3L_MAX_DEPTH);
      /* ^ Clear z-buffer if we didn't draw the pixel. Without this further
      geometry drawn later on won't be seen through transparent objects which
      looks bad. With this "fix" glitches may still appear (wrong draw order)
      but it generally looks better this way. */

    return;
  }

#if LCR_SETTING_TEXTURE_SUBSAMPLE != 0
  if (LCR_renderer.texSubsampleCount == 0)
  {
#endif
    int barycentric[3];

    barycentric[0] = pixel->barycentric[0] / 8;
    barycentric[1] = pixel->barycentric[1] / 8;
    barycentric[2] = pixel->barycentric[2] / 8;

    LCR_renderer.pixelColor = LCR_sampleImage(
      (barycentric[0] * LCR_renderer.triUVs[0] +
       barycentric[1] * LCR_renderer.triUVs[2] +
       barycentric[2] * LCR_renderer.triUVs[4])
       / (S3L_F / 8),
      (barycentric[0] * LCR_renderer.triUVs[1] +
       barycentric[1] * LCR_renderer.triUVs[3] +
       barycentric[2] * LCR_renderer.triUVs[5])
       / (S3L_F / 8));

#if LCR_SETTING_TEXTURE_SUBSAMPLE != 0
    LCR_renderer.texSubsampleCount = LCR_SETTING_TEXTURE_SUBSAMPLE;
  }

  LCR_renderer.texSubsampleCount--;
#endif

  LCR_gameDrawPixelXYUnsafe(pixel->x,pixel->y,
#if LCR_SETTING_FOG
    (((uint_fast16_t) 0) - (
    (pixel->depth < (S3L_MAX_DEPTH >> 20)) |
    ((pixel->depth < (S3L_MAX_DEPTH >> 19)) &
     ((pixel->x | pixel->y) % 2)) |
     ((pixel->x ^ pixel->y) % 2))) &
#endif
    LCR_renderer.pixelColor);

#endif // LCR_SETTING_POTATO_GRAPHICS
}

S3L_Index _LCR_rendererAddMapVert(S3L_Unit x, S3L_Unit y, S3L_Unit z)
{
  S3L_Index index = 0;
  S3L_Unit *verts = LCR_renderer.mapVerts;

  while (index < LCR_renderer.mapModel.vertexCount) // if exists, return index
  {
    if (verts[0] == x && verts[1] == y && verts[2] == z)
      return index;

    verts += 3;
    index++;
  }

  // if it doesn't exist, add it
  if (LCR_renderer.mapModel.vertexCount < LCR_SETTING_MAX_MAP_VERTICES)
  {
    *verts = x; verts++;
    *verts = y; verts++;
    *verts = z;
    LCR_renderer.mapModel.vertexCount++;
    return LCR_renderer.mapModel.vertexCount - 1;
  }

  LCR_LOG1("couldn't add vertex");
  return 0;
}

void _LCR_rendererAddMapTri(S3L_Index a, S3L_Index b, S3L_Index c, uint8_t mat)
{
  if (LCR_renderer.mapModel.triangleCount < LCR_SETTING_MAX_MAP_TRIANGLES)
  {
    S3L_Index *t =
      &(LCR_renderer.mapTris[LCR_renderer.mapModel.triangleCount * 3]);

    *t = a; t++;
    *t = b; t++;
    *t = c;

    LCR_renderer.mapTriangleData[LCR_renderer.mapModel.triangleCount] = mat;

    LCR_renderer.mapModel.triangleCount++;
  }
  else
  {
    LCR_LOG1("couldn't add triangle");
  }
}

void _LCR_rendererSwapMapTris(unsigned int index1, unsigned int index2)
{
  uint8_t tmpMat;
  S3L_Index tmpIndex,
    *t1 = LCR_renderer.mapTris + index1 * 3,
    *t2 = LCR_renderer.mapTris + index2 * 3;

  for (int i = 0; i < 3; ++i)
  {
    tmpIndex = t1[i];
    t1[i] = t2[i];
    t2[i] = tmpIndex;
  }

  tmpMat = LCR_renderer.mapTriangleData[index1];
  LCR_renderer.mapTriangleData[index1] = LCR_renderer.mapTriangleData[index2];
  LCR_renderer.mapTriangleData[index2] = tmpMat;
}

int _LCR_rendererQuadCoversTri(const S3L_Unit quad[8], const S3L_Unit tri[6])
{
  for (int i = 0; i < 3; ++i) // for each triangle point
  {
    int covered = 0;

    for (int j = 0; j < 3; ++j) // for each quad subtriangle
    {
      uint8_t winds = 0;

      for (int k = 0; k < 3; ++k) // for each subtriangle side
      {
        int w =
          _LCR_triangleWinding(
            quad[(2 * (j + (k + 1) % 3)) % 8],
            quad[(2 * (j + ((k + 1) % 3))) % 8 + 1],
            quad[(2 * (j + k)) % 8],
            quad[(2 * (j + k)) % 8 + 1],
            tri[2 * i],
            tri[2 * i + 1]);
        winds |= (w > 0) | ((w < 0) << 1);
      }

      if (winds != 3 && winds != 0) // no opposite winds and at least 1 non-0?
      {
        covered = 1;
        break;
      }
    }

    if (!covered)
      return 0;
  }

  return 1;
}

/**
  Checks whether two triangles (and potentially their neighbors) cover each
  other, in return values lowest bit means whether t1 is covered and the second
  lowest bit means whether t2 is covered.
*/
uint8_t _LCR_rendererCheckMapTriCover(const S3L_Index *t1, const S3L_Index *t2)
{
  S3L_Unit *vertices[6];

  // critical place, manually unrolled loop:

  vertices[0] = LCR_renderer.mapVerts + 3 * t1[0];
  vertices[3] = LCR_renderer.mapVerts + 3 * t2[0];

  if ( // quick chebyshev distance bailout condition
    S3L_abs(vertices[0][0] - vertices[3][0]) > LCR_RENDERER_UNIT ||
    S3L_abs(vertices[0][2] - vertices[3][2]) > LCR_RENDERER_UNIT ||
    S3L_abs(vertices[0][1] - vertices[3][1]) > (LCR_RENDERER_UNIT / 2))
    return 0;

  if ( // same vert indices?
    (((t1[0] == t2[0]) || (t1[0] == t2[1]) || (t1[0] == t2[2]))) &&
    (((t1[1] == t2[0]) || (t1[1] == t2[1]) || (t1[1] == t2[2]))) &&
    (((t1[2] == t2[0]) || (t1[2] == t2[1]) || (t1[2] == t2[2]))))
    return 0x03;

  vertices[1] = LCR_renderer.mapVerts + 3 * t1[1];
  vertices[4] = LCR_renderer.mapVerts + 3 * t2[1];  
  vertices[2] = LCR_renderer.mapVerts + 3 * t1[2];
  vertices[5] = LCR_renderer.mapVerts + 3 * t2[2];

  uint8_t result = 0;
  int plane = -1;

  for (int i = 0; i < 3; ++i)
    if (vertices[0][i] == vertices[1][i] && vertices[1][i] == vertices[2][i] &&
      vertices[2][i] == vertices[3][i] && vertices[3][i] == vertices[4][i] &&
      vertices[4][i] == vertices[5][i])
    {
      plane = i;
      break;
    }

  if (plane >= 0) // both triangles in the same plane => then do more checks
  {
    for (int j = 0; j < 2; ++j)
    {
      S3L_Unit points2D[14]; // tri1, quad (tri2 + 1 extra vert)

      int coordX = plane == 0 ? 1 : 0,
          coordY = plane == 2 ? 1 : 2;

      for (int i = 0; i < 3; ++i)
      {
        points2D[i * 2]         = vertices[i][coordX];
        points2D[i * 2 + 1]     = vertices[i][coordY];
        points2D[6 + i * 2]     = vertices[3 + i][coordX];
        points2D[6 + i * 2 + 1] = vertices[3 + i][coordY];
      }

      points2D[12] = (4 * points2D[6] + 3 * points2D[8] + points2D[10]) / 8;
      points2D[13] = (4 * points2D[7] + 3 * points2D[9] + points2D[11]) / 8;

      // first: does the triangle alone cover the other one?
      if (_LCR_rendererQuadLooksConvex(points2D + 6) &&
        _LCR_rendererQuadCoversTri(points2D + 6,points2D))
        result |= 1 << j;
      else
      {
        // now check if this triangle along with a neighbor cover the other one

        S3L_Index *t3 = LCR_renderer.mapTris;

        for (int i = 0; i < LCR_renderer.mapModel.triangleCount; ++i)
        {
          uint8_t sharedVerts =
            (t3[0] == t2[0] || t3[0] == t2[1] || t3[0] == t2[2]) |
            ((t3[1] == t2[0] || t3[1] == t2[1] || t3[1] == t2[2]) << 1) |
            ((t3[2] == t2[0] || t3[2] == t2[1] || t3[2] == t2[2]) << 2);

          if (t3 != t1 && t3 != t2 &&
            (sharedVerts == 3 || sharedVerts == 5 || sharedVerts == 6) &&
            LCR_renderer.mapVerts[3 * t3[0] + plane] ==
            LCR_renderer.mapVerts[3 * t3[1] + plane] &&
            LCR_renderer.mapVerts[3 * t3[1] + plane] ==
            LCR_renderer.mapVerts[3 * t3[2] + plane] &&
            LCR_renderer.mapVerts[3 * t3[0] + plane] ==
            LCR_renderer.mapVerts[3 * t1[0] + plane] &&
              _LCR_triangleWinding(
                points2D[6],points2D[7],points2D[8],points2D[9],
                points2D[10],points2D[11]) ==
              _LCR_triangleWinding(
                LCR_renderer.mapVerts[3 * t3[0] + coordX],
                LCR_renderer.mapVerts[3 * t3[0] + coordY],
                LCR_renderer.mapVerts[3 * t3[1] + coordX],
                LCR_renderer.mapVerts[3 * t3[1] + coordY],
                LCR_renderer.mapVerts[3 * t3[2] + coordX],
                LCR_renderer.mapVerts[3 * t3[2] + coordY]))
          {
            // here shares exactly 2 verts + is in the same plane + same winding

            uint8_t freeVert =
              sharedVerts == 3 ? 2 : (sharedVerts == 5 ? 1 : 0);

            points2D[12] = LCR_renderer.mapVerts[3 * t3[freeVert] + coordX];
            points2D[13] = LCR_renderer.mapVerts[3 * t3[freeVert] + coordY];

            if (_LCR_rendererQuadLooksConvex(points2D + 6) &&
              _LCR_rendererQuadCoversTri(points2D + 6,points2D))
            {
              result |= 1 << j;
              break;
            }
          }

          t3 += 3;
        }
      }

      // now swap both triangles and do it all again:

      const S3L_Index *tmp = t1;
      t1 = t2;
      t2 = tmp;

      for (int i = 0; i < 3; ++i)
      {
        S3L_Unit *tmpCoord = vertices[i];
        vertices[i] = vertices[3 + i];
        vertices[3 + i] = tmpCoord;
      }
    }
  }

  return result;
}

/**
  Removes map triangles that are covered by other triangles (and also vertices
  that by this become unused). This makes the map model smaller, faster and
  prevents bleeding through due to z-bugger imprecisions.
*/
void _LCR_cullHiddenMapTris(void)
{
  LCR_LOG1("culling invisible triangles");

  int n = 0; // number of removed elements
  int i = 0;

  S3L_Index *t1 = LCR_renderer.mapTris, *t2;

  /*
    We'll be moving the covered triangles to the end of the array, then at the
    end we'll just shorten the array by number of removed triangles.
  */
  while (i < LCR_renderer.mapModel.triangleCount - n)
  {
    t2 = t1 + 3; // t2 is the the other triangle against which we check

    int t1Covered = 0;

    for (int j = i + 1; j < LCR_renderer.mapModel.triangleCount; ++j)
    {
      uint8_t cover = _LCR_rendererCheckMapTriCover(t1,t2);

      t1Covered |= cover & 0x01;

      if ((cover & 0x02) && (j < LCR_renderer.mapModel.triangleCount - n))
      {
        _LCR_rendererSwapMapTris(j,LCR_renderer.mapModel.triangleCount - 1 - n);
        n++;
      }

      t2 += 3; // move to the next triangle
    }

    if (t1Covered)
    {
      _LCR_rendererSwapMapTris(i,LCR_renderer.mapModel.triangleCount - 1 - n);
      n++;
      // we stay at this position because we've swapped the triangle here
    }
    else
    {
      t1 += 3;
      i++;
    }
  }

  LCR_renderer.mapModel.triangleCount -= n; // cut off the removed triangles

  // remove unused vertices:

  i = 0;

  while (i < LCR_renderer.mapModel.vertexCount)
  {
    int used = 0;

    for (int j = 0; j < LCR_renderer.mapModel.triangleCount * 3; ++j)
      if (LCR_renderer.mapTris[j] == i)
      {
        used = 1;
        break;
      }

    if (used)
      i++;
    else
    {
      for (int j = 0; j < 3; ++j)
        LCR_renderer.mapVerts[3 * i + j] = LCR_renderer.mapVerts[
          (LCR_renderer.mapModel.vertexCount - 1) * 3 + j];

      for (int j = 0; j < LCR_renderer.mapModel.triangleCount * 3; ++j)
        if (LCR_renderer.mapTris[j] == LCR_renderer.mapModel.vertexCount - 1)
          LCR_renderer.mapTris[j] = i;

      LCR_renderer.mapModel.vertexCount--;
    }
  }
}

/**
  Rearranges map triangles so that they're grouped by chunks. Order of triangles
  doesn't matter much in rendering, so we exploit this to use order for
  chunking.
*/
void _LCR_makeMapChunks(void)
{
  LCR_LOG1("making map chunks");

  S3L_Index start = 0;

  for (int chunkNo = 0; chunkNo < LCR_RENDERER_CHUNKS_TOTAL; ++chunkNo)
  {
    S3L_Unit chunkCorner[3];
    const S3L_Index *tri = LCR_renderer.mapTris + 3 * start;

    LCR_renderer.chunkStarts[chunkNo] = start;

    chunkCorner[0] =
      (chunkNo & 0x03) * LCR_RENDERER_CHUNK_SIZE_HORIZONTAL;
    chunkCorner[1] =
      ((chunkNo >> 2) & 0x03) * (LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 2);
    chunkCorner[2] =
      ((chunkNo >> 4) & 0x03) * LCR_RENDERER_CHUNK_SIZE_HORIZONTAL;

    chunkCorner[0] -= LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 2;
    chunkCorner[1] -= LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 4;
    chunkCorner[2] -= LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 2;

    for (int i = start; i < LCR_renderer.mapModel.triangleCount; ++i)
    {
      const S3L_Unit *v = LCR_renderer.mapVerts + 3 * tri[0];

      if (v[0] >= chunkCorner[0] &&
        v[0] < chunkCorner[0] + LCR_RENDERER_CHUNK_SIZE_HORIZONTAL +
        ((chunkNo & 0x03) == 0x03) && // includes last edge
        v[1] >= chunkCorner[1] &&
        v[1] < chunkCorner[1] + (LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 2) +
        (((chunkNo >> 2) & 0x03) == 0x03) &&
        v[2] >= chunkCorner[2] &&
        v[2] < chunkCorner[2] + LCR_RENDERER_CHUNK_SIZE_HORIZONTAL +
        (((chunkNo >> 4) & 0x03) == 0x03))
      {
        _LCR_rendererSwapMapTris(i,start);
        start++;
      }

      tri += 3;
    }
  }
}

/**
  Builds the internal 3D model of the currently loaded map. Returns 1 on
  success, 0 otherwise (e.g. not enough space).
*/
uint8_t _LCR_buildMapModel(void)
{
  LCR_LOG1("building map model");

  uint8_t blockShapeBytes[LCR_MAP_BLOCK_SHAPE_MAX_BYTES];
  uint8_t blockShapeByteCount;

  S3L_model3DInit(LCR_renderer.mapVerts,0,LCR_renderer.mapTris,0,
    &LCR_renderer.mapModel);

  LCR_renderer.mapModel.transform.scale.x =
    (_LCR_MAP_MODEL_SCALE * S3L_F) / 1024;
  LCR_renderer.mapModel.transform.scale.y
    = LCR_renderer.mapModel.transform.scale.x;
  LCR_renderer.mapModel.transform.scale.z
    = LCR_renderer.mapModel.transform.scale.x;

  for (int j = 0; j < LCR_currentMap.blockCount; ++j)
  {
    if (((j + 1) % LCR_SETTING_CULLING_PERIOD == 0))
      _LCR_cullHiddenMapTris();

    S3L_Unit originOffset = -1 * LCR_MAP_SIZE_BLOCKS / 2 * LCR_RENDERER_UNIT;
    S3L_Index triIndices[3];
    const uint8_t *block = LCR_currentMap.blocks + j * LCR_BLOCK_SIZE;
    uint8_t
      blockType = block[0],
      edgeBits,   // touching bounds? bottom, top, left, right, front, back
      bx, by, bz, // block coords
      vx, vy, vz, // vertex coords
      vi = 0;     // vertex index (0, 1 or 2)

#ifdef LCR_LOADING_COMMAND
    if (j % 64 == 0)
    {
      LCR_LOADING_COMMAND
    }
#endif

    LCR_mapBlockGetCoords(block,&bx,&by,&bz);

    LCR_mapGetBlockShape(blockType,LCR_mapBlockGetTransform(block),
      blockShapeBytes,&blockShapeByteCount);

    // When nearing limit, cull (can't be inside the loop sadly, trust me).

    if (
      LCR_renderer.mapModel.vertexCount >= LCR_SETTING_MAX_MAP_VERTICES - 16 ||
      LCR_renderer.mapModel.triangleCount >= LCR_SETTING_MAX_MAP_TRIANGLES - 16)
    {
      _LCR_cullHiddenMapTris();

      if (LCR_renderer.mapModel.vertexCount >= LCR_SETTING_MAX_MAP_VERTICES - 16
        || LCR_renderer.mapModel.triangleCount >= LCR_SETTING_MAX_MAP_TRIANGLES
        - 16)
        break; // didn't help, no need to continue
    }

    for (int i = 0; i < blockShapeByteCount; ++i)
    {
      if (vi == 0)
        edgeBits =
          (by == 0)        | ((by == LCR_MAP_SIZE_BLOCKS - 1) << 1) |
          ((bx == 0) << 2) | ((bx == LCR_MAP_SIZE_BLOCKS - 1) << 3) |
          ((bz == 0) << 4) | ((bz == LCR_MAP_SIZE_BLOCKS - 1) << 5);

      LCR_decodeMapBlockCoords(blockShapeBytes[i],&vx,&vy,&vz);

      edgeBits &=
        (vy == 0)        | ((vy == LCR_BLOCK_SHAPE_COORD_MAX) << 1) |
        ((vx == 0) << 2) | ((vx == LCR_BLOCK_SHAPE_COORD_MAX) << 3) |
        ((vz == 0) << 4) | ((vz == LCR_BLOCK_SHAPE_COORD_MAX) << 5);

      triIndices[vi] = _LCR_rendererAddMapVert(
        originOffset + (((S3L_Unit) bx) * LCR_RENDERER_UNIT) +
          (LCR_RENDERER_UNIT * ((S3L_Unit) vx)) / LCR_BLOCK_SHAPE_COORD_MAX,
        (originOffset + (((S3L_Unit) by) * LCR_RENDERER_UNIT)) / 2 +
          (LCR_RENDERER_UNIT / 2 * ((S3L_Unit) vy)) / LCR_BLOCK_SHAPE_COORD_MAX,
        originOffset + (((S3L_Unit) bz) * LCR_RENDERER_UNIT) +
          (LCR_RENDERER_UNIT * ((S3L_Unit) vz)) / LCR_BLOCK_SHAPE_COORD_MAX);

      if (vi < 2)
        vi++;
      else // 3 indices => create and add triangle
      {
        // don't add triangles completely at the boundary of the map
        if (!edgeBits)
        {
          uint8_t triData;

          if (blockType == LCR_BLOCK_CHECKPOINT_0)
            triData = LCR_RENDERER_MAT_CP0 | ((i % 2) << 4);
          else if (blockType == LCR_BLOCK_FINISH)
            triData = LCR_RENDERER_MAT_FIN | ((i % 2) << 4);
          else
          {
            uint8_t blockMat = LCR_mapBlockGetMaterial(block);

#define VERT(n,c) LCR_renderer.mapVerts[3 * n + c]

            triData =
              (((VERT(triIndices[0],0) == VERT(triIndices[1],0)) && // same X?
              (VERT(triIndices[1],0) == VERT(triIndices[2],0))) << 4) |
              (((VERT(triIndices[0],2) == VERT(triIndices[1],2)) && // same Z?
              (VERT(triIndices[1],2) == VERT(triIndices[2],2))) << 5);

            if (!triData)
            {
              // diagonal walls

              triData = (
                (VERT(triIndices[0],0) == VERT(triIndices[1],0) &&
                VERT(triIndices[0],2) == VERT(triIndices[1],2)) |
                (VERT(triIndices[1],0) == VERT(triIndices[2],0) &&
                VERT(triIndices[1],2) == VERT(triIndices[2],2)) |
                (VERT(triIndices[0],0) == VERT(triIndices[2],0) &&
                VERT(triIndices[0],2) == VERT(triIndices[2],2))) << 4;
            }
#undef VERT

            if (triData & 0xf0) // wall?
            {
              triData |= 
                ((blockMat == LCR_BLOCK_MATERIAL_CONCRETE) ||
                (blockMat == LCR_BLOCK_MATERIAL_ICE) ||
                LCR_mapBlockIsAccelerator(blockType) ||
                LCR_mapBlockIsFan(blockType)) ?
                LCR_IMAGE_WALL_CONCRETE : LCR_IMAGE_WALL_WOOD;
            }
            else
            {
              if (LCR_mapBlockIsAccelerator(blockType))
                triData |= LCR_IMAGE_GROUND_ACCEL;
              else if (LCR_mapBlockIsFan(blockType))
                triData |= LCR_IMAGE_GROUND_FAN;
              else
                switch (blockMat)
                {
                  case LCR_BLOCK_MATERIAL_CONCRETE:
                    triData |= LCR_IMAGE_GROUND_CONCRETE;
                    break;

                  case LCR_BLOCK_MATERIAL_GRASS:
                    triData |= LCR_IMAGE_GROUND_GRASS;
                    break;

                  case LCR_BLOCK_MATERIAL_DIRT:
                    triData |= LCR_IMAGE_GROUND_DIRT;
                    break;

                  case LCR_BLOCK_MATERIAL_ICE:
                    triData |= LCR_IMAGE_GROUND_ICE;
                    break;

                  default:
                    break;
                }
            }
          }

          _LCR_rendererAddMapTri(triIndices[0],triIndices[1],triIndices[2],
            triData);
        }

        vi = 0;
      }
    }
  }

  _LCR_cullHiddenMapTris();

  LCR_LOG1("map model built, verts/tris:");
  LCR_LOG1_NUM(LCR_renderer.mapModel.vertexCount);
  LCR_LOG1_NUM(LCR_renderer.mapModel.triangleCount);

  return 1;
}

/**
  Computes the binary 3D grid of simple LODs (these just say whether in given
  area is "something" or not) for currently loaded map.
*/
void _LCR_rendererComputeLOD(void)
{
  LCR_LOG1("computing LOD");

  for (int i = 0; i < LCR_RENDERER_LOD_BLOCKS; ++i)
    LCR_renderer.gridOfLODs[i] = 0;

  for (int i = 0; i < LCR_currentMap.blockCount; ++i)
  {
    uint8_t x, y, z;

    LCR_mapBlockGetCoords(LCR_currentMap.blocks + i * LCR_BLOCK_SIZE,&x,&y,&z);

    x /= 8;
    y /= 8;
    z /= 8;

    LCR_renderer.gridOfLODs[z * 8 + y] |= (0x01 << x);
  }
}

/**
  Unmarks all checkpoints that were marked with LCR_rendererMarkTakenCP.
*/
void LCR_rendererUnmarkCPs(void)
{
  for (int i = 0; i < LCR_renderer.mapModel.triangleCount; ++i)
    if ((LCR_renderer.mapTriangleData[i] & 0x0f) == LCR_RENDERER_MAT_CP1)
      LCR_renderer.mapTriangleData[i] = (LCR_renderer.mapTriangleData[i] & 0xf0)
        | LCR_RENDERER_MAT_CP0;
}

/**
  Marks checkpoint as taken, i.e. changes how a checkpoint will be drawn.
*/
void LCR_rendererMarkTakenCP(int x, int y, int z)
{
  for (int i = 0; i < LCR_renderer.mapModel.triangleCount; ++i)
    if ((LCR_renderer.mapTriangleData[i] & 0x0f) == LCR_RENDERER_MAT_CP0)
    {
      S3L_Unit point[3];

      point[0] = 0;
      point[1] = 0;
      point[2] = 0;

      for (int j = 0; j < 2; ++j)
        for (int k = 0; k < 3; ++k)
          point[k] += LCR_renderer.mapModel.vertices[
            3 * LCR_renderer.mapModel.triangles[3 * i + j] + k] +
              (LCR_MAP_SIZE_BLOCKS / 2) *
                (k == 1 ? LCR_RENDERER_UNIT / 2 : LCR_RENDERER_UNIT);

      point[0] /= 2;
      point[1] /= 2;
      point[2] /= 2;

      if (point[0] / LCR_RENDERER_UNIT == x &&
        point[1] / (LCR_RENDERER_UNIT / 2) == y &&
        point[2] / LCR_RENDERER_UNIT == z)
        LCR_renderer.mapTriangleData[i] = (LCR_renderer.mapTriangleData[i]
          & 0xf0) | LCR_RENDERER_MAT_CP1;
    }
}

/**
  Initializes renderer, only call once.
*/
uint8_t LCR_rendererInit(void)
{
  LCR_LOG0("initializing renderer");

  LCR_renderer.frame = 0;

  LCR_renderer.carModel = LCR_renderer.models + 8;
  LCR_renderer.ghostModel = LCR_renderer.models + 9;
  LCR_renderer.shadowModel = LCR_renderer.models + 10;

#if LCR_SETTING_CAR_SHADOW
  S3L_model3DInit(
    LCR_shadowVertices,
    LCR_SHADOW_VERTEX_COUNT,
    LCR_shadowTriangles,LCR_SHADOW_TRIANGLE_COUNT,
    LCR_renderer.shadowModel);
#else
  S3L_model3DInit(0,0,0,0,LCR_renderer.shadowModel);
  LCR_renderer.shadowModel->config.visible = 0;
#endif

  S3L_model3DInit(
#if LCR_ANIMATE_CAR
    LCR_renderer.animatedCarVerts
#else
    LCR_carVertices
#endif
    ,LCR_CAR_VERTEX_COUNT,
    LCR_carTriangles,LCR_CAR_TRIANGLE_COUNT,
    LCR_renderer.carModel);

  S3L_vec4Set(&(LCR_renderer.carModel->transform.scale),
    LCR_RENDERER_CAR_SCALE,LCR_RENDERER_CAR_SCALE,LCR_RENDERER_CAR_SCALE,0);

  S3L_model3DInit(LCR_carVertices,LCR_CAR_VERTEX_COUNT,LCR_carTriangles,
    LCR_CAR_TRIANGLE_COUNT,LCR_renderer.ghostModel);

  LCR_renderer.ghostModel->transform.scale =
    LCR_renderer.carModel->transform.scale; 

#if LCR_ANIMATE_CAR
  for (int i = 0; i < LCR_CAR_VERTEX_COUNT * 3; ++i)
    LCR_renderer.animatedCarVerts[i] = LCR_carVertices[i];

  int count[2];
  count[0] = 0;
  count[1] = 0;

  for (int i = 0; i < 4; ++i)
    LCR_renderer.wheelRotationCenters[i] = 0;

  for (int i = 0; i < LCR_CAR_VERTEX_COUNT; ++i)
    if (LCR_carVertexTypes[i] > 0) // wheel?
    {
      uint8_t front = LCR_carVertexTypes[i] == 1;

      LCR_renderer.wheelRotationCenters[0 + 2 * front] +=
        LCR_carVertices[3 * i + 2];
      LCR_renderer.wheelRotationCenters[1 + 2 * front] +=
        LCR_carVertices[3 * i + 1];

      count[front]++;
    }

  LCR_renderer.wheelRotationCenters[0] /= count[0];
  LCR_renderer.wheelRotationCenters[1] /= count[0];
  LCR_renderer.wheelRotationCenters[2] /= count[1];
  LCR_renderer.wheelRotationCenters[3] /= count[1];

  LCR_renderer.wheelRotation = 0;
  LCR_renderer.wheelSteer = 0;
#endif

  LCR_LOG2("initializing 3D scene");

  S3L_sceneInit(
    LCR_renderer.models,LCR_RENDERER_MODEL_COUNT,&LCR_renderer.scene);

  LCR_renderer.scene.camera.focalLength =
    (LCR_SETTING_CAMERA_FOCAL_LENGTH * S3L_F) / 16;

  return 1;
}

void LCR_rendererGetCameraTransform(LCR_GameUnit position[3],
  LCR_GameUnit rotation[3], LCR_GameUnit *fov)
{
  position[0] = (LCR_renderer.scene.camera.transform.translation.x *
    LCR_GAME_UNIT) / LCR_RENDERER_UNIT;
  position[1] = (LCR_renderer.scene.camera.transform.translation.y *
    LCR_GAME_UNIT) / LCR_RENDERER_UNIT;
  position[2] = (LCR_renderer.scene.camera.transform.translation.z *
    LCR_GAME_UNIT) / LCR_RENDERER_UNIT;

  rotation[0] = (LCR_renderer.scene.camera.transform.rotation.x *
    LCR_GAME_UNIT) / S3L_F;
  rotation[1] = (LCR_renderer.scene.camera.transform.rotation.y *
    LCR_GAME_UNIT) / S3L_F;
  rotation[2] = (LCR_renderer.scene.camera.transform.rotation.z *
    LCR_GAME_UNIT) / S3L_F;

  *fov = (LCR_renderer.scene.camera.focalLength * LCR_GAME_UNIT) / S3L_F;
}

/**
  Moves and rotates camera by given offset in game units.
*/
void LCR_rendererMoveCamera(LCR_GameUnit forwRightUpOffset[3],
  LCR_GameUnit yawPitchOffset[2])
{  
  LCR_LOG2("moving camera");

  S3L_Vec4 f, r, u;

  S3L_rotationToDirections(LCR_renderer.scene.camera.transform.rotation,
    LCR_RENDERER_UNIT,&f,&r,&u);

  LCR_renderer.scene.camera.transform.translation.x +=
    (f.x * forwRightUpOffset[0] + r.x * forwRightUpOffset[1] +
    u.x * forwRightUpOffset[2]) / LCR_GAME_UNIT;
 
  LCR_renderer.scene.camera.transform.translation.y +=
    (f.y * forwRightUpOffset[0] + r.y * forwRightUpOffset[1] +
    u.y * forwRightUpOffset[2]) / LCR_GAME_UNIT;

  LCR_renderer.scene.camera.transform.translation.z +=
    (f.z * forwRightUpOffset[0] + r.z * forwRightUpOffset[1] +
    u.z * forwRightUpOffset[2]) / LCR_GAME_UNIT;

  LCR_renderer.scene.camera.transform.rotation.y +=
    (yawPitchOffset[0] * S3L_F) / LCR_GAME_UNIT;

  LCR_renderer.scene.camera.transform.rotation.x +=
    (yawPitchOffset[1] * S3L_F) / LCR_GAME_UNIT;

#define CHK(o,c,l) \
  if (LCR_renderer.scene.camera.transform.translation.c o l) \
    LCR_renderer.scene.camera.transform.translation.c = l;

  CHK(<,x,-1 * LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 2)
  CHK(>,x,LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 2)
  CHK(<,y,-1 * LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 4)
  CHK(>,y,LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 4)
  CHK(<,z,-1 * LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 2)
  CHK(>,z,LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT / 2)

#undef CHK
}

/**
  Fast and safe rect drawing function (handles out of screen coords).
*/
void LCR_rendererDrawRect(int x, int y, unsigned int w, unsigned int h,
  uint16_t color, int dither)
{
  if (x >= LCR_EFFECTIVE_RESOLUTION_X || y >= LCR_EFFECTIVE_RESOLUTION_Y)
    return;

  if (x < 0)
  {
    if (-1 * x >= ((int) w))
      return;

    w += x;
    x = 0;
  }

  if (x + w > LCR_EFFECTIVE_RESOLUTION_X)
    w = LCR_EFFECTIVE_RESOLUTION_X - x;

  if (y < 0)
  {
    if (-1 * y > ((int) h))
      return;

    h += y;
    y = 0;
  }

  if (y + h > LCR_EFFECTIVE_RESOLUTION_Y)
    h = LCR_EFFECTIVE_RESOLUTION_Y - y;

  unsigned long index = y * LCR_EFFECTIVE_RESOLUTION_X + x;

  if (dither)
  {
    uint8_t parity = (x % 2) == (y % 2);

    for (unsigned int i = 0; i < h; ++i)
    {
      for (unsigned int j = ((i % 2) == parity); j < w; j += 2)
        LCR_gameDrawPixel(index + j,color);

      index += LCR_EFFECTIVE_RESOLUTION_X;
    }  
  }
  else
    for (unsigned int i = 0; i < h; ++i)
    {
      for (unsigned int j = 0; j < w; ++j)
      {
        LCR_gameDrawPixel(index,color);
        index++;
      }

      index += LCR_EFFECTIVE_RESOLUTION_X - w;
    }
}

/**
  Draws a very simple LOD block (just a few 2D rectangles) meant to represent
  far away geometry. The size parameter says base size in pixels (will be
  affected by perspective), variability is used to create slightly different
  shapes of blocks.
*/
void _LCR_rendererDrawLODBlock(int blockX, int blockY, int blockZ,
  unsigned int size, uint16_t color, uint8_t variability)
{
  LCR_LOG2("drawing LOD block");

  S3L_Vec4 p, r;

  p.x = (blockX - LCR_MAP_SIZE_BLOCKS / 2) * LCR_RENDERER_UNIT
    + LCR_RENDERER_UNIT / 2;

  p.y = (blockY - LCR_MAP_SIZE_BLOCKS / 2) * (LCR_RENDERER_UNIT / 2)
    + LCR_RENDERER_UNIT / 4;

  p.z = (blockZ - LCR_MAP_SIZE_BLOCKS / 2) * LCR_RENDERER_UNIT
    + LCR_RENDERER_UNIT / 2;

  p.w = size;

  S3L_project3DPointToScreen(p,LCR_renderer.scene.camera,&r);

  if (r.w > 0 && r.z > LCR_SETTING_LOD_DISTANCE * LCR_RENDERER_UNIT &&
    r.w < LCR_EFFECTIVE_RESOLUTION_X)
  {
    switch (variability % 4)
    {
      case 0:  r.w += r.w / 4; r.x += LCR_BLOCK_SIZE / 8; break;
      case 1:  r.w += r.w / 8; r.y -= LCR_BLOCK_SIZE / 16; break;
      case 2:  r.w += r.w / 4; break;
      default: r.z += LCR_BLOCK_SIZE / 8; break;
    }

    if (variability % 8 < 5)
      LCR_rendererDrawRect(r.x - r.w / 2,r.y - r.w / 2,r.w,r.w,color,1);
    else
    {
      r.w /= 2;
      LCR_rendererDrawRect(r.x - r.w / 2,r.y - r.w / 2,r.w,r.w,color,1);
      r.w += r.w / 2;
      LCR_rendererDrawRect(r.x - r.w / 8,r.y - r.w / 4,r.w,r.w,color,1);
    }
  }
}

/**
  Draws background sky, offsets are in multiples of screen dimensions
  (e.g. S3L_F / 2 for offsetH means half the screen width).
*/
void LCR_rendererDrawSky(int sky, S3L_Unit offsetH, S3L_Unit offsetV)
{
  LCR_LOG2("drawing sky");

#if LCR_SETTING_POTATO_GRAPHICS
  LCR_rendererDrawRect(0,0,LCR_EFFECTIVE_RESOLUTION_X,
    LCR_EFFECTIVE_RESOLUTION_Y,LCR_CONVERT_COLOR(0xffff),0);
#else
  int anchorPoint[2], y;
  unsigned long pixelIndex;
  unsigned int topColor, bottomColor;

  sky = 8 + 4 * sky;

  LCR_loadImage(sky);
  topColor = LCR_sampleImage(0,0); // top strip color is the first sky pixel

  LCR_loadImage(sky + 3);          // load the last part of the sky (3)
  bottomColor = LCR_sampleImage(LCR_IMAGE_SIZE - 1,LCR_IMAGE_SIZE - 1);

  anchorPoint[0] = ((LCR_IMAGE_SIZE * 2 * LCR_SETTING_SKY_SIZE * offsetH)
    / S3L_F) % (2 * LCR_IMAGE_SIZE * LCR_SETTING_SKY_SIZE);

  if (anchorPoint[0] < 0)
    anchorPoint[0] += 2 * LCR_IMAGE_SIZE * LCR_SETTING_SKY_SIZE;

  anchorPoint[1] = LCR_EFFECTIVE_RESOLUTION_Y / 2 - (LCR_EFFECTIVE_RESOLUTION_Y
    * offsetV) / S3L_F - ((LCR_IMAGE_SIZE * LCR_SETTING_HORIZON_SHIFT) / 100 *
    LCR_SETTING_SKY_SIZE);

  pixelIndex = 0;
  y = anchorPoint[1] < 0 ? anchorPoint[1] : 0;

  while (y < anchorPoint[1] && y < LCR_EFFECTIVE_RESOLUTION_Y) // top strip
  {
    for (int x = 0; x < LCR_EFFECTIVE_RESOLUTION_X; ++x)
    {
      LCR_gameDrawPixel(pixelIndex,topColor);
      pixelIndex++;
    }

    y++;
  }

  anchorPoint[1] += 2 * LCR_IMAGE_SIZE * LCR_SETTING_SKY_SIZE;
 
  int linesLeft = 0;
  int skyPart = 0;

  while (y < anchorPoint[1] && y < LCR_EFFECTIVE_RESOLUTION_Y) // image strip
  {
    if (!linesLeft)
    {
      LCR_loadImage(sky + skyPart);
      linesLeft = LCR_IMAGE_SIZE / 2;
      skyPart++;
    }

    if (y >= 0)
    {
      for (int ix = 0; ix < 2 * LCR_IMAGE_SIZE * LCR_SETTING_SKY_SIZE;
        ix += LCR_SETTING_SKY_SIZE)
      {
        uint_fast16_t color = LCR_getNextImagePixel();
        unsigned long startIndex = pixelIndex;

        for (int k = 0; k < LCR_SETTING_SKY_SIZE; ++k)
        {
          if (y + k >= LCR_EFFECTIVE_RESOLUTION_Y)
            break;

          for (int j = 0; j < LCR_SETTING_SKY_SIZE; ++j)
          {
            int x = anchorPoint[0] + ix + j;

            if (x >= 2 * LCR_IMAGE_SIZE * LCR_SETTING_SKY_SIZE)
              x -= 2 * LCR_IMAGE_SIZE * LCR_SETTING_SKY_SIZE;

            while (x < LCR_EFFECTIVE_RESOLUTION_X)
            {
              LCR_gameDrawPixel(startIndex + x,color);
              x += 2 * LCR_IMAGE_SIZE * LCR_SETTING_SKY_SIZE;
            }
          }

          startIndex += LCR_EFFECTIVE_RESOLUTION_X;
        }
      }
    
      pixelIndex += LCR_EFFECTIVE_RESOLUTION_X * LCR_SETTING_SKY_SIZE;
      y += LCR_SETTING_SKY_SIZE;
    }
    else
    {
      for (int ix = 0; ix < 2 * LCR_IMAGE_SIZE; ++ix)
        LCR_getNextImagePixel();
   
      for (int i = 0; i < LCR_SETTING_SKY_SIZE; ++i)
      {
        if (y >= 0)
          for (int x = 0; x < LCR_EFFECTIVE_RESOLUTION_X; ++x)
          {
            LCR_gameDrawPixel(pixelIndex,topColor);
            pixelIndex++;
          }

        y++;
      }
    }

    linesLeft--;
  }

  while (y < 0) // can still be the case
    y = 0;

  while (y < LCR_EFFECTIVE_RESOLUTION_Y) // bottom strip
  {
    for (int x = 0; x < LCR_EFFECTIVE_RESOLUTION_X; ++x)
    {
      LCR_gameDrawPixel(pixelIndex,bottomColor);
      pixelIndex++;
    }

    y++;
  }
#endif
}

/**
  Loads a map chunk at chunk coords into given scene model. The chunk param says
  into which model to load the chunk (0 to 7), the x, y and z params are the
  coordinates (may even be outside the map).
*/
void _LCR_rendererLoadMapChunk(uint8_t chunk, int8_t x, int8_t y, int8_t z)
{
  LCR_renderer.models[chunk] = LCR_renderer.mapModel;

  if (x < 0 || x >= LCR_RENDERER_CHUNK_RESOLUTION ||
      y < 0 || y >= LCR_RENDERER_CHUNK_RESOLUTION ||
      z < 0 || z >= LCR_RENDERER_CHUNK_RESOLUTION)
  {
    LCR_renderer.models[chunk].triangleCount = 0;
    LCR_renderer.loadedChunks[chunk] = 0;
  }
  else
  {
    int blockNum = x | (y << 2) | (z << 4);

    LCR_renderer.loadedChunks[chunk] = blockNum;

    int triCount =
      (blockNum == LCR_RENDERER_CHUNKS_TOTAL - 1 ?
      (LCR_renderer.mapModel.triangleCount - 1) :
        LCR_renderer.chunkStarts[blockNum + 1])
        - LCR_renderer.chunkStarts[blockNum];

    if (triCount < 0)
      triCount = 0;

    LCR_renderer.models[chunk].triangles =
      LCR_renderer.mapTris + LCR_renderer.chunkStarts[blockNum] * 3;

    LCR_renderer.models[chunk].triangleCount = triCount;
  }
}

/**
  Serves for smoothing out angle change, e.g. that of camera rotation.
*/
S3L_Unit _LCR_rendererSmoothRot(S3L_Unit angleOld, S3L_Unit angleNew,
  unsigned int amount)
{
  /* We have to do the following angle correction -- even if keep angles in
  correct range at the start of frame, subsequent steps may alter the rotations
  and here we could end up with bad ranges again. */
  S3L_Unit angleDiff = S3L_wrap(angleNew,S3L_F) - S3L_wrap(angleOld,S3L_F);

  if (angleDiff == 0)
    return angleNew;

  S3L_Unit angleDiffAbs = S3L_abs(angleDiff);

  if (angleDiffAbs > S3L_F / 2) // consider e.g. 350 degrees minus 1 degree
  {
    angleDiffAbs = S3L_F - angleDiffAbs;
    angleDiff = (angleDiff > 0) ? -1 * angleDiffAbs : angleDiffAbs;
  }

  if (angleDiffAbs > (3 * S3L_F) / 8) // angle too big, rotate immediately
    return angleNew;

  return angleOld + (angleDiff / S3L_nonZero(amount));
}

/**
  Loads the map models with 8 chunks that are nearest to a certain point towards
  which the camera is looking.
*/
void LCR_rendererLoadMapChunks(void)
{
  LCR_LOG2("loading map chunks");

  int8_t camChunk[3], chunkOffsets[3];
  S3L_Vec4 cp = LCR_renderer.scene.camera.transform.translation;
  S3L_Vec4 cf;
  
  S3L_rotationToDirections(LCR_renderer.scene.camera.transform.rotation,
    LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 2,&cf,0,0);

  cp.x += (LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT) / 2;
  cp.y += (LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT) / 4;
  cp.z += (LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT) / 2;

  cf.x += cp.x % LCR_RENDERER_CHUNK_SIZE_HORIZONTAL;
  cf.y += cp.y % (LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 2);
  cf.z += cp.z % LCR_RENDERER_CHUNK_SIZE_HORIZONTAL;

  camChunk[0] = cp.x / LCR_RENDERER_CHUNK_SIZE_HORIZONTAL;
  camChunk[1] = cp.y / (LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 2);
  camChunk[2] = cp.z / LCR_RENDERER_CHUNK_SIZE_HORIZONTAL;

  chunkOffsets[0] =
    (cf.x >= (LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 2)) ? 1 : -1;

  chunkOffsets[1] =
    (cf.y >= (LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 4)) ? 1 : -1;

  chunkOffsets[2] =
    (cf.z >= (LCR_RENDERER_CHUNK_SIZE_HORIZONTAL / 2)) ? 1 : -1;

  for (uint8_t i = 0; i < 8; ++i)
    _LCR_rendererLoadMapChunk(i,
      camChunk[0] + ((i & 0x01) ? chunkOffsets[0] : 0),
      camChunk[1] + ((i & 0x02) ? chunkOffsets[1] : 0),
      camChunk[2] + ((i & 0x04) ? chunkOffsets[2] : 0));
}

/**
  Creates everything that's needed to start rendering the currently loaded map,
  returns success (1 or 0).
*/
uint8_t LCR_rendererLoadMap(void)
{
  LCR_LOG0("loading map");

  if (!_LCR_buildMapModel())
    return 0;

  _LCR_makeMapChunks();
  LCR_rendererLoadMapChunks();
  _LCR_rendererComputeLOD();

  return 1;
}

/**
  Draws the LOD overlay.
*/
void LCR_rendererDrawLOD(void)
{
  LCR_LOG2("drawing LOD");

#if LCR_SETTING_LOD_DISTANCE < 64
  int variability = 0;

  for (unsigned int i = 0; i < LCR_RENDERER_LOD_BLOCKS; ++i)
    if (LCR_renderer.gridOfLODs[i])
    {
      uint8_t byte = LCR_renderer.gridOfLODs[i];
      unsigned int bx, by, bz;

      bz = (i / 8) * 8 + 4;
      by = (i % 8) * 8 + 4;

      for (unsigned int j = 0; j < 8; ++j)
      {
        if (byte & 0x01)
        {
          variability = variability < 14 ? variability + 1 : 0;
          bx = j * 8 + 4;
          _LCR_rendererDrawLODBlock(bx,by,bz,4 * S3L_F,
            LCR_SETTING_LOD_COLOR,variability);
        }

        byte >>= 1;
      }
    }
#endif
}

#if LCR_ANIMATE_CAR
void _LCR_rendererAnimateCar(void)
{
  LCR_LOG2("animating car");

  for (int i = LCR_renderer.frame % LCR_SETTING_CAR_ANIMATION_SUBDIVIDE;
    i < LCR_CAR_VERTEX_COUNT; i += LCR_SETTING_CAR_ANIMATION_SUBDIVIDE)
  {
    if (LCR_carVertexTypes[i] > 0)
    {
      S3L_Unit s = S3L_sin(-1 * LCR_renderer.wheelRotation),
               c = S3L_cos(-1 * LCR_renderer.wheelRotation);

      S3L_Unit v[2], tmp; 

      unsigned int index = 3 * i;
      uint8_t offset = (LCR_carVertexTypes[i] == 1) * 2;

      v[0] = LCR_carVertices[index + 2] -
        LCR_renderer.wheelRotationCenters[offset];
      v[1] = LCR_carVertices[index + 1] -
        LCR_renderer.wheelRotationCenters[offset + 1];

      tmp = v[0];
      v[0] = (v[0] * c - v[1] * s) / S3L_F;
      v[1] = (tmp  * s + v[1] * c) / S3L_F;

      LCR_renderer.animatedCarVerts[index + 2] =
        v[0] + LCR_renderer.wheelRotationCenters[offset];
      LCR_renderer.animatedCarVerts[index + 1] =
        v[1] + LCR_renderer.wheelRotationCenters[offset + 1];

      if (LCR_carVertexTypes[i] == 1)
      {
        /* Turn front wheels; this is not real turning but just a fake by
           skewing in Z and X directions. */
        LCR_renderer.animatedCarVerts[index] = LCR_carVertices[index];

        LCR_renderer.animatedCarVerts[index + 2] -=
          (LCR_renderer.animatedCarVerts[index] * LCR_renderer.wheelSteer)
          / (16 * S3L_F);  //(8 * S3L_F);

        LCR_renderer.animatedCarVerts[index] +=
          ((LCR_renderer.animatedCarVerts[index + 2] -
          LCR_renderer.wheelRotationCenters[2]) * LCR_renderer.wheelSteer)
          / (4 * S3L_F); //(2 * S3L_F);
      }
    }
  }
}
#endif

/**
  Call every rendering frame to make the camera follow the car model. The
  distance parameter can be either 0 (inside of car, car should be made
  invisible for this), 1 (normal distance) or 2 (double distance).
*/
void LCR_rendererCameraFollow(unsigned char distance)
{
  LCR_LOG2("following camera");

  if (distance == 0)
  {
    LCR_renderer.scene.camera.transform.translation =
      LCR_renderer.carModel->transform.translation;

    LCR_renderer.scene.camera.transform.translation.y += LCR_RENDERER_UNIT / 3;

    if (!_LCR_rendererGimbalDanger( // kind of (imperfectly) deals with bugs
      LCR_renderer.carModel->transform.rotation.x))
    {
      LCR_renderer.scene.camera.transform.rotation =
        LCR_renderer.carModel->transform.rotation;

      LCR_renderer.scene.camera.transform.rotation.x = -1 *
        ((LCR_renderer.scene.camera.transform.rotation.x +
        S3L_FRACTIONS_PER_UNIT / 4) % (S3L_FRACTIONS_PER_UNIT / 2) -
        S3L_FRACTIONS_PER_UNIT / 4);

      LCR_renderer.scene.camera.transform.rotation.y +=
        S3L_FRACTIONS_PER_UNIT / 2;

      LCR_renderer.scene.camera.transform.rotation.z *= -1;
    }

    return;
  }

  int shift = distance != 1;

  S3L_Transform3D transPrev = LCR_renderer.scene.camera.transform;

  LCR_renderer.scene.camera.transform.translation.y =
    S3L_clamp(
      LCR_renderer.scene.camera.transform.translation.y,
      LCR_renderer.carModel->transform.translation.y +
      ((LCR_SETTING_CAMERA_HEIGHT << shift) - LCR_SETTING_CAMERA_HEIGHT_BAND) *
        LCR_RENDERER_UNIT / 8,
      LCR_renderer.carModel->transform.translation.y +
      ((LCR_SETTING_CAMERA_HEIGHT << shift) + LCR_SETTING_CAMERA_HEIGHT_BAND) *
        LCR_RENDERER_UNIT / 8);

  S3L_Vec4 toCam = LCR_renderer.scene.camera.transform.translation;

  S3L_vec3Sub(&toCam,LCR_renderer.carModel->transform.translation);

  S3L_Unit horizontalDist = S3L_sqrt(toCam.x * toCam.x + toCam.z * toCam.z);

  if (horizontalDist == 0)
  {
    toCam.z = 1;
    horizontalDist = 1;
  }

  S3L_Unit horizontalDistNew =
    S3L_clamp(horizontalDist,
      ((LCR_SETTING_CAMERA_DISTANCE << shift) - LCR_SETTING_CAMERA_DISTANCE_BAND)
      * (LCR_RENDERER_UNIT / 4),
      ((LCR_SETTING_CAMERA_DISTANCE << shift) + LCR_SETTING_CAMERA_DISTANCE_BAND)
      * (LCR_RENDERER_UNIT / 4));

  if (horizontalDistNew != horizontalDist)
  {
    toCam.x = (toCam.x * horizontalDistNew) / horizontalDist;
    toCam.z = (toCam.z * horizontalDistNew) / horizontalDist;

    LCR_renderer.scene.camera.transform.translation.x =
      LCR_renderer.carModel->transform.translation.x +
      (toCam.x * horizontalDistNew) / horizontalDist;

    LCR_renderer.scene.camera.transform.translation.z =
      LCR_renderer.carModel->transform.translation.z +
      (toCam.z * horizontalDistNew) / horizontalDist;
  }

  S3L_lookAt(LCR_renderer.carModel->transform.translation,
    &(LCR_renderer.scene.camera.transform));

  // look a bit up to see further ahead:
  LCR_renderer.scene.camera.transform.rotation.x += S3L_F / 32;

#if LCR_SETTING_SMOOTH_ANIMATIONS
  // now average with previous transform to smooth the animation out:

  S3L_vec3Add(&(LCR_renderer.scene.camera.transform.translation),
  transPrev.translation);

  LCR_renderer.scene.camera.transform.translation.x /= 2;
  LCR_renderer.scene.camera.transform.translation.y /= 2;
  LCR_renderer.scene.camera.transform.translation.z /= 2;
 
  LCR_renderer.scene.camera.transform.rotation.x = _LCR_rendererSmoothRot(
    transPrev.rotation.x,LCR_renderer.scene.camera.transform.rotation.x,8);

  LCR_renderer.scene.camera.transform.rotation.y = _LCR_rendererSmoothRot(
    transPrev.rotation.y,LCR_renderer.scene.camera.transform.rotation.y,6);
#endif
}

void LCR_rendererBlitImage(uint8_t index, unsigned int x, unsigned int y,
  uint8_t scale, uint16_t transparentColor)
{
  if (scale == 0 || x + LCR_IMAGE_SIZE * scale >= LCR_EFFECTIVE_RESOLUTION_X ||
    y + LCR_IMAGE_SIZE * scale >= LCR_EFFECTIVE_RESOLUTION_Y)
    return;

  for (int m = 0; m < scale; ++m)
    for (int n = 0; n < scale; ++n)
    {
      LCR_loadImage(index);
      for (int k = 0; k < LCR_IMAGE_SIZE; ++k)
      {
        int i = (y + m + k * scale) * LCR_EFFECTIVE_RESOLUTION_X + x + n;

        for (int l = 0; l < LCR_IMAGE_SIZE; ++l)
        {
          uint_fast16_t color = LCR_getNextImagePixel();

          if (color != transparentColor)
            LCR_gameDrawPixel(i,color);

          i += scale;
        }
      }  
    }
}

void LCR_rendererDrawMenu(const char *tabName,const char **items,
  unsigned char itemCount,char selectedItem, char scroll)
{
#if !LCR_ANT_RESOLUTION
  int stripHeight = (2 * LCR_EFFECTIVE_RESOLUTION_Y) / 11;
  int stripHeight2 = LCR_EFFECTIVE_RESOLUTION_Y / 12;
#else
  int stripHeight = 6;
  int stripHeight2 = 0;
#endif

  int i = 0;
#if !(LCR_SETTING_POTATO_GRAPHICS || LCR_SETTING_332_COLOR)
  uint16_t effect = LCR_renderer.frame >> 2;
#endif

  LCR_LOG2("drawing menu");

  while (i < stripHeight * LCR_EFFECTIVE_RESOLUTION_X)
  {
    LCR_gameDrawPixel(i,LCR_CONVERT_COLOR(0x4a8c)
#if LCR_SETTING_POTATO_GRAPHICS || LCR_SETTING_332_COLOR
      );
#else
      ^ (effect & 0x1863) ^ (i & 0x1863));

    effect += 5 + i % 4;
#endif

    ++i;
  }

#if LCR_SETTING_POTATO_GRAPHICS || LCR_ANT_RESOLUTION
  while (i < stripHeight * LCR_EFFECTIVE_RESOLUTION_X)
  {
    LCR_gameDrawPixel(i,LCR_CONVERT_COLOR(0x73ae));
    ++i;
  }
#else
  for (int y = 0; y < stripHeight2; ++y) // strip with arrows
  {
    int limit = y > stripHeight2 / 2 ?
      (stripHeight2 / 2 - y) : (y - stripHeight2 / 2);

    for (int x = 0; x < LCR_EFFECTIVE_RESOLUTION_X; ++x)
    {
      LCR_gameDrawPixel(i,(x > LCR_EFFECTIVE_RESOLUTION_X / 4 - limit && 
        (x < LCR_EFFECTIVE_RESOLUTION_X - LCR_EFFECTIVE_RESOLUTION_X / 4 +
        limit) ? LCR_CONVERT_COLOR(0x73ae) : LCR_CONVERT_COLOR(0x31a6)) +
        ((x + y) % 8));
      i++;
    }
  }
#endif

  while (i < LCR_EFFECTIVE_RESOLUTION_Y * LCR_EFFECTIVE_RESOLUTION_X)
  {
    LCR_gameDrawPixel(i,LCR_CONVERT_COLOR(0xce9c)
#if LCR_SETTING_POTATO_GRAPHICS
      );
#else
      + (i & LCR_CONVERT_COLOR(0x1002)));
#endif

    ++i;
  }

  i = stripHeight + (stripHeight2 - LCR_rendererComputeTextHeight(3)) / 2 + 2;

  for (int j = 0; j < itemCount + 1 + (scroll != 0); ++j)
  {
    const char *s = j ? (j <= itemCount ? items[j - 1] : "...") : tabName;
    const char *s2 = s;
    uint16_t textColor = LCR_CONVERT_COLOR(0x5aeb);

    while (*s2) // if the item contains '#' (check), we draw it as green
    {
      if (*s2 == '#')
        textColor = LCR_CONVERT_COLOR(0x0700);

      s2++;
    }

#define _FONT_SIZE (2 + LCR_EFFECTIVE_RESOLUTION_Y / 512 \
  - (LCR_EFFECTIVE_RESOLUTION_Y < 220))

    if (j == selectedItem + 1)
      for (int y = i - 3 * LCR_FONT_PIXEL_SIZE; y < i +
        LCR_rendererComputeTextHeight(_FONT_SIZE) + 3 * LCR_FONT_PIXEL_SIZE;
        ++y)
#if LCR_EFFECTIVE_RESOLUTION_X > 500
        for (int x = 40; x < LCR_EFFECTIVE_RESOLUTION_X - 40; ++x)
#else
        for (int x = 2; x < LCR_EFFECTIVE_RESOLUTION_X - 2; ++x)
#endif
          LCR_gameDrawPixelXYSafe(x,y,0x5c1b + 4 * ((x & 0x10) == (y & 0x10)));

    LCR_rendererDrawText(s,(LCR_EFFECTIVE_RESOLUTION_X -
      LCR_rendererComputeTextWidth(s,_FONT_SIZE)) / 2,i,
      (j == 0 || j == selectedItem + 1) ? LCR_CONVERT_COLOR(0xf79c) :
        textColor,_FONT_SIZE);

#if LCR_ANT_RESOLUTION
    i += LCR_rendererComputeTextHeight(_FONT_SIZE) + 3 * LCR_FONT_PIXEL_SIZE;
#else
    if (j == 0)
      i = stripHeight + stripHeight2;

    i += LCR_rendererComputeTextHeight(_FONT_SIZE) + 6 * LCR_FONT_PIXEL_SIZE;
#endif

#undef _FONT_SIZE
  }

#if (!LCR_SETTING_POTATO_GRAPHICS) && (!LCR_ANT_RESOLUTION)
  LCR_rendererBlitImage(21,(LCR_EFFECTIVE_RESOLUTION_X -
    LCR_IMAGE_SIZE * (stripHeight / LCR_IMAGE_SIZE)) / 2,8,
    stripHeight / LCR_IMAGE_SIZE,LCR_CONVERT_COLOR(0xffff));
#endif

#if !LCR_ANT_RESOLUTION
  LCR_rendererDrawText(LCR_texts[LCR_TEXTS_VERSION],
    LCR_EFFECTIVE_RESOLUTION_X / 64,LCR_EFFECTIVE_RESOLUTION_Y / 64,
      LCR_CONVERT_COLOR(0xe71c),2);
#endif

  LCR_renderer.frame++;
}

/**
  Resets camera rotation and places it behind the car.
*/
void LCR_rendererCameraReset(void)
{
  LCR_renderer.scene.camera.transform.translation =
    LCR_renderer.carModel->transform.translation;

  LCR_renderer.scene.camera.transform.translation.x -=
    S3L_sin(LCR_renderer.carModel->transform.rotation.y);

  LCR_renderer.scene.camera.transform.translation.z +=
    S3L_cos(LCR_renderer.carModel->transform.rotation.y);

  LCR_renderer.scene.camera.transform.rotation.x = 0;
  LCR_renderer.scene.camera.transform.rotation.z = 0;

  LCR_rendererCameraFollow(2);
  LCR_rendererCameraFollow(1);
}

void LCR_rendererSetWheelState(LCR_GameUnit rotation, LCR_GameUnit steer)
{
#if LCR_ANIMATE_CAR
  LCR_renderer.wheelRotation = rotation;
  LCR_renderer.wheelSteer = steer;
#endif
}

void LCR_rendererDraw3D(void)
{
  LCR_LOG2("rendering frame (start)");

#if LCR_SETTING_CAR_SHADOW
  LCR_renderer.shadowModel->transform.translation =
    LCR_renderer.carModel->transform.translation;

  LCR_renderer.shadowModel->transform.translation.y =
    -1 * (LCR_MAP_SIZE_BLOCKS * LCR_RENDERER_UNIT) / 4;

  S3L_vec4Set(&(LCR_renderer.shadowModel->transform.scale),
    LCR_RENDERER_UNIT,LCR_RENDERER_UNIT,LCR_RENDERER_UNIT,
    LCR_RENDERER_UNIT);

  if (!_LCR_rendererGimbalDanger(LCR_renderer.carModel->transform.rotation.x))
    LCR_renderer.shadowModel->transform.rotation.y =
      LCR_renderer.carModel->transform.rotation.y;
#endif

  // first make sure rotations are in correct range:
  LCR_renderer.scene.camera.transform.rotation.y = S3L_wrap(
    LCR_renderer.scene.camera.transform.rotation.y, S3L_F);

  LCR_renderer.scene.camera.transform.rotation.x = S3L_clamp(
    LCR_renderer.scene.camera.transform.rotation.x,-1 * S3L_F / 4,S3L_F / 4);

  LCR_renderer.previousTriID = -1;
  S3L_newFrame();
  
#if LCR_ANIMATE_CAR  
  _LCR_rendererAnimateCar();
#endif

  if (LCR_renderer.frame % LCR_SETTING_MAP_CHUNK_RELOAD_INTERVAL == 0)
    LCR_rendererLoadMapChunks();

  LCR_rendererDrawSky(LCR_currentMap.environment,
    LCR_renderer.scene.camera.transform.rotation.y,
    -4 * LCR_renderer.scene.camera.transform.rotation.x);

  LCR_rendererDrawLOD();

  uint8_t carGhostVisibility = 0;
  S3L_Model3D *m = LCR_renderer.carModel;

  for (uint8_t i = 0; i < 2; ++i)
  {
    carGhostVisibility <<= 1;

    if (m->config.visible)
    {
      carGhostVisibility |= 1;

      unsigned char camDist = S3L_distanceManhattan(m->transform.translation,
        LCR_renderer.scene.camera.transform.translation) / LCR_RENDERER_UNIT;

      m->config.visible = camDist <= LCR_SETTING_CAR_RENDER_DISTANCE;

#if LCR_SETTING_PARTICLES
      if (camDist > 5)
        LCR_renderer.particleColor = 0x0000;
#endif
    }

    m = LCR_renderer.ghostModel; 
  }

#if LCR_SETTING_CAR_SHADOW
  LCR_renderer.shadowModel->config.visible =
    LCR_renderer.carModel->config.visible;
#endif

  LCR_LOG2("3D rendering (start)");

#if LCR_SETTING_POTATO_GRAPHICS
  /* in potato mode we render twice so that car is always in front of the
     level model (lack of precise depth causes artifacts otherwise) */

  LCR_renderer.carModel->config.visible = 0;
  LCR_renderer.ghostModel->config.visible = 0;

  S3L_drawScene(LCR_renderer.scene);

  for (int i = 0; i < LCR_renderer.scene.modelCount; ++i)
    LCR_renderer.scene.models[i].config.visible = 0;

  LCR_renderer.carModel->config.visible = carGhostVisibility >> 1;
  LCR_renderer.ghostModel->config.visible = carGhostVisibility & 0x01;

  S3L_newFrame();
  S3L_drawScene(LCR_renderer.scene);

  for (int i = 0; i < LCR_renderer.scene.modelCount; ++i)
    LCR_renderer.scene.models[i].config.visible = 1;
#else
  S3L_drawScene(LCR_renderer.scene);

  LCR_renderer.carModel->config.visible = carGhostVisibility >> 1;
  LCR_renderer.ghostModel->config.visible = carGhostVisibility & 0x01;
#endif

  LCR_renderer.frame++;

#if LCR_SETTING_PARTICLES  
  if (LCR_renderer.particleColor)
  {
    S3L_Vec4 p, r, s;

    S3L_rotationToDirections(LCR_renderer.carModel->transform.rotation,
      LCR_RENDERER_UNIT / 2,&p,&r,0);

#define LCR_PARTICLE_SIZE \
  ((16 << (LCR_EFFECTIVE_RESOLUTION_X / 640)) >> LCR_ANT_RESOLUTION)

    p.w = LCR_PARTICLE_SIZE;

    r.x /= 2;
    r.y /= 2;
    r.z /= 2;
      
    S3L_vec3Add(&p,LCR_renderer.carModel->transform.translation);

    for (int i = 0; i < 2; ++i) // for both wheels
    {
      S3L_vec3Add(&p,r);

      S3L_project3DPointToScreen(p,LCR_renderer.scene.camera,&s);

      if (s.w > 0)
        for (int j = 0; j < 4; ++j) // 4 particles
        {
          int pSize =
            ((LCR_renderer.frame + 4 * j) << 2) % (2 * LCR_PARTICLE_SIZE);

          LCR_rendererDrawRect(
            s.x - pSize / 2 + ((j % 2) ? -1 * pSize : pSize),
            s.y - pSize / 2 + ((j / 2) ? -1 * pSize : pSize),
            pSize,pSize,LCR_renderer.particleColor,1);
        }

      r.x *= -2;
      r.y *= -2;
      r.z *= -2;
    }
  }
#undef LCR_PARTICLE_SIZE

#endif

  LCR_LOG2("3D rendering (end)");
  LCR_LOG2("rendering frame (end)");
}

#endif // guard
