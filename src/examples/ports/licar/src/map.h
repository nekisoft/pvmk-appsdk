#ifndef _LCR_MAP
#define _LCR_MAP

/** @file map.h

  Licar: map module

  This implements maps (i.e. tracks, levels, ...).

  Map coordinates/size:
    - map size is 64x64x64 blocks
    - [0,0,0] is bottom-left-front-most
    - x goes right, y goes up, z goes forward
    - coordinate number is a single number obtained as x + 64 * y + 64 * 64 * z

  The TEXT format serves for editing maps in human readable format, it more or
  less corresponds to the binary storage format (below) with some exceptions.
  It has the following structure:
    - Target time as a decimal number of physics ticks.
    - Non-decimal character.
    - Number of environment (0, 1, 2, ...)
    - A series of block strings. Blocks may be preceded/followed by characters
      that aren't ':' (comments may be added this way). Block format is
      following:

      :BXYZMT

      where:
      - B is block type
      - X, Y and Z are block coordinates, each one a single character. The
        following are characters signifying numbers 0 to 63:
        0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$@
      - M is optional block material ('0' to '3', 0 is default).
      - T is an optional transform string (for more detail see the binary
        format) consisting of 0 to 3 characters, which may be:
        - '|': flip horizontally
        - '-': flip vertically
        - 'L': rotate 90 degrees
        - 'I': rotate 180 degrees
        - 'J': rotate 270 degrees

  Here the block order matters, latter blocks rewrite previously placed ones.

  The internal BINARY map format is made from the text string. It can't contain
  special blocks and blocks are ordered by their coordinate number (for fast
  lookup). The format consists of blocks, each of format:
    - 1 byte type: says the type of block, is the same value as B in the text
      format.
    - 3 bytes: A, B, C, such that:
      - A, B and lowest 2 bits of C form the block coordinate number (A being
        the lowest part etc.)
      - bits C2 and C3 say the block material
      - highest 4 bits of C (C4, C5, C6, C7) say the block's transform:
        - first if C4 is set, the block is flipped in the X direction
        - then the block is rotated around vertical axis by 0, 90, 180 or 270
          degrees if C5C6 is 00, 01, 10 or 11.
        - last if C7 is set, the block is flipped vertically
*/

#include "general.h"

#ifdef LCR_MODULE_NAME
  #undef LCR_MODULE_NAME
#endif

#define LCR_MAP_SIZE_BLOCKS 64

#define LCR_MODULE_NAME "map"

#define LCR_MAP_NAME_MAX_LEN        15   /**< Maximum map name length (without
                                              terminating zero. */
#define LCR_BLOCK_START_CHAR        ':'

/** Maximum number of triangles of a block shape. */
#define LCR_MAP_BLOCK_SHAPE_MAX_BYTES 80

#define LCR_BLOCK_TRANSFORM_FLIP_H  0x10
#define LCR_BLOCK_TRANSFORM_ROT_90  0x20
#define LCR_BLOCK_TRANSFORM_ROT_180 0x40
#define LCR_BLOCK_TRANSFORM_ROT_270 0x60
#define LCR_BLOCK_TRANSFORM_FLIP_V  0x80

#define LCR_MAP_BLOCK(t,x,y,z,m,r)  t,(uint8_t) (x | (y << 6)), \
                                    (uint8_t) ((y >> 2) | (z << 4)), \
                                    (uint8_t) ((z >> 4) | (m << 2) | (r))

#define LCR_BLOCK_SIZE              4    ///< size of map block, in bytes

#define LCR_BLOCK_MATERIAL_CONCRETE 0x00
#define LCR_BLOCK_MATERIAL_GRASS    0x01
#define LCR_BLOCK_MATERIAL_DIRT     0x02
#define LCR_BLOCK_MATERIAL_ICE      0x03

// normal blocks:
#define LCR_BLOCK_FULL              '='  ///< completely filled block
#define LCR_BLOCK_BOTTOM            '-'  ///< filled bottom half
#define LCR_BLOCK_LEFT              ';'  ///< filled left half
#define LCR_BLOCK_BOTTOM_LEFT       ','  ///< filled bottom left quarter
#define LCR_BLOCK_BOTTOM_LEFT_FRONT '.'  ///< filled bottom left front eighth
#define LCR_BLOCK_LEFT_FRONT        'I'  ///< filled left front quarter
#define LCR_BLOCK_RAMP              '^'  ///< plain ramp
#define LCR_BLOCK_RAMP_34           '/'  ///< plain ramp, 3/4 size
#define LCR_BLOCK_RAMP_12           '<'  ///< plain ramp, 1/2 size
#define LCR_BLOCK_RAMP_14           '_'  ///< plain ramp, 1/4 size
#define LCR_BLOCK_RAMP_12_UP        '\'' ///< ramp, 1/2 size, elevated up
#define LCR_BLOCK_RAMP_CORNER       'v'  ///< corner of a ramp
#define LCR_BLOCK_RAMP_CURVED_PLAT  ']'  ///< curved ramp with top platform
#define LCR_BLOCK_RAMP_CURVED       ')'  ///< curv. ramp without top platf.
#define LCR_BLOCK_RAMP_CURVED_WALL  '}'  ///< curved ramp plus small wall
#define LCR_BLOCK_RAMP_STEEP        '|'  ///< extremely steep ramp
#define LCR_BLOCK_CORNER            'A'  ///< diagonal corner
#define LCR_BLOCK_CORNER_12         '\\' ///< diagonal corner (1/2 wide)
#define LCR_BLOCK_HILL              '('  ///< curved "hill"
#define LCR_BLOCK_BUMP              '~'  ///< small bump on the road
#define LCR_BLOCK_CORNER_CONVEX     'n'  ///< curved corner (convex)
#define LCR_BLOCK_CORNER_CONCAVE    'u'  ///< curved corner (concave)
#define LCR_BLOCK_FULL_ACCEL        '>'  ///< full block with accelerator
#define LCR_BLOCK_BOTTOM_ACCEL      'z'  ///< bottom half block with accelerator
#define LCR_BLOCK_RAMP_ACCEL        'y'  ///< ramp block with accelerator
#define LCR_BLOCK_FULL_FAN          'o'  ///< full block with fan
#define LCR_BLOCK_RAMP_FAN          'V'  ///< ramp block with fan
#define LCR_BLOCK_CHECKPOINT_0      '+'  ///< checkpoint, not taken
#define LCR_BLOCK_CHECKPOINT_1      '\t' ///< checkpoint, taken
#define LCR_BLOCK_FINISH            '!'  ///< finish

// special blocks:
#define LCR_BLOCK_NONE              'x'  ///< no block (to make holes etc.)
#define LCR_BLOCK_CUBOID_FILL       'f'  /**< makes a cuboid from the
                                              previously specified block, the
                                              size is given by block coords */
#define LCR_BLOCK_CUBOID_HOLLOW     'h'  /**< same as cuboid special block,
                                              but makes a hollow one */
#define LCR_BLOCK_MIRROR            'm'  /**< makes a mirror copy along each
                                              major axis, starting at coords of
                                              last added block and iterating
                                              over a block of size given by this
                                              block's coords */
#define LCR_BLOCK_START             '*'  ///< specifies start block position
#define LCR_BLOCK_QUIT              'e'  /**< special block, ends reading the 
                                              map (useful when creating maps) */

#define LCR_MAP_BLOCK_CACHE_SIZE (8 * 2) ///< Do not change.

/**
  Cache for accelerating LCR_mapGetBlockAtFast, consists of 8 2-item records,
  the first record item stores block coord number, the second one stores the
  value returned by LCR_mapGetBlockAtFast (-1 is 0xffffffff, unknown is
  0xfffffffe). The record index depends on the block coords: lowest bit is x %
  2, middle bit y % 2, highest one z % 2.
*/
uint32_t _LCR_mapBlockCache[LCR_MAP_BLOCK_CACHE_SIZE];

struct
{
  uint16_t blockCount;
  uint8_t blocks[LCR_SETTING_MAP_MAX_BLOCKS * LCR_BLOCK_SIZE];
  uint8_t startPos[4];      ///< Initial position and rotation.

  uint8_t environment;
  uint8_t checkpointCount;

  uint32_t hash;            ///< Hash of the processed binary map.
  uint32_t targetTime;      ///< Target time in physics ticks.

  char name[LCR_MAP_NAME_MAX_LEN + 1];
} LCR_currentMap;

void LCR_makeMapBlock(uint8_t type, uint8_t x, uint8_t y, uint8_t z,
  uint8_t material, uint8_t transform, uint8_t block[LCR_BLOCK_SIZE])
{
  x &= 0x3f;
  y &= 0x3f;
  z &= 0x3f;
  block[0] = type;
  block[1] = x | (y << 6);
  block[2] = (y >> 2) | (z << 4);
  block[3] = (z >> 4) | (material << 2) | transform;
}

void LCR_mapBlockGetCoords(const uint8_t block[LCR_BLOCK_SIZE],
  uint8_t *x, uint8_t *y, uint8_t *z)
{
  *x = block[1] & 0x3f;
  *y = (block[1] >> 6) | ((block[2] & 0x0f)  << 2);
  *z = (block[2] >> 4) | ((block[3] & 0x03) << 4);
}

uint8_t LCR_mapBlockOppositeTransform(uint8_t transform)
{
  if (!(transform & LCR_BLOCK_TRANSFORM_FLIP_H))
  {
    if ((transform & 0x60) == LCR_BLOCK_TRANSFORM_ROT_90)
      return ((transform & (~0x60)) | LCR_BLOCK_TRANSFORM_ROT_270);

    if ((transform & 0x60) == LCR_BLOCK_TRANSFORM_ROT_270)
      return ((transform & (~0x60)) | LCR_BLOCK_TRANSFORM_ROT_90);
  }

  return transform;
}

uint8_t LCR_mapBlockFlipTransformX(uint8_t transform)
{
  transform ^= LCR_BLOCK_TRANSFORM_FLIP_H;

  if ((transform & 0x60) == LCR_BLOCK_TRANSFORM_ROT_90)
    return ((transform & (~0x60)) | LCR_BLOCK_TRANSFORM_ROT_270);

  else if ((transform & 0x60) == LCR_BLOCK_TRANSFORM_ROT_270)
    return ((transform & (~0x60)) | LCR_BLOCK_TRANSFORM_ROT_90);

  return transform;
}

uint8_t LCR_mapBlockFlipTransformZ(uint8_t transform)
{
  transform ^= LCR_BLOCK_TRANSFORM_FLIP_H;

  if ((transform & 0x60) == LCR_BLOCK_TRANSFORM_ROT_180)
    return transform & (~0x60);

  else if ((transform & 0x60) == 0)
    return transform | LCR_BLOCK_TRANSFORM_ROT_180;

  return transform;
}

uint8_t LCR_mapBlockIsAccelerator(uint8_t block)
{
  return block == LCR_BLOCK_FULL_ACCEL || block == LCR_BLOCK_RAMP_ACCEL ||
    block == LCR_BLOCK_BOTTOM_ACCEL;
}

uint8_t LCR_mapBlockIsFan(uint8_t block)
{
  return block == LCR_BLOCK_FULL_FAN || block == LCR_BLOCK_RAMP_FAN;
}

uint8_t LCR_mapBlockGetTransform(const uint8_t block[LCR_BLOCK_SIZE])
{
  return block[3] & 0xf0;
}

uint8_t LCR_mapBlockGetMaterial(const uint8_t block[LCR_BLOCK_SIZE])
{
  return (LCR_mapBlockIsAccelerator(block[0]) || LCR_mapBlockIsFan(block[0])) ?
    LCR_BLOCK_MATERIAL_CONCRETE : ((block[3] >> 2) & 0x03);
}

uint32_t LCR_mapBlockGetCoordNumber(const uint8_t block[LCR_BLOCK_SIZE])
{
  return block[1] | (((uint32_t) block[2]) << 8) |
    ((((uint32_t) block[3]) & 0x3) << 16);
}

uint32_t LCR_mapBlockCoordsToCoordNumber(uint8_t x, uint8_t y, uint8_t z)
{
  uint8_t b[LCR_BLOCK_SIZE];
  LCR_makeMapBlock(0,x,y,z,0,0,b);
  return LCR_mapBlockGetCoordNumber(b);
}

/**
  Gets dimensions of a non-curved ramp.
*/
void LCR_rampGetDimensions(uint8_t rampType, uint8_t *height4ths,
  uint8_t *length6ths)
{
  *height4ths = (rampType == LCR_BLOCK_RAMP_14) +
    (rampType == LCR_BLOCK_RAMP || rampType == LCR_BLOCK_RAMP_ACCEL ||
     rampType == LCR_BLOCK_RAMP_FAN || rampType == LCR_BLOCK_RAMP_STEEP) * 4 +
    (rampType == LCR_BLOCK_RAMP_12 || rampType == LCR_BLOCK_RAMP_34) * 2 +
    (rampType == LCR_BLOCK_RAMP_34);

  *length6ths = rampType != LCR_BLOCK_RAMP_STEEP ? 6 : 1;
}

/**
  Adds given block to current map, including possibly deleting a block by adding
  LCR_BLOCK_NONE. The function handles sorting the block to the right position.
  Returns 1 if successful, else 0.
*/
uint8_t _LCR_mapAddBlock(const uint8_t block[LCR_BLOCK_SIZE])
{
  LCR_LOG2("adding map block");

  if (LCR_currentMap.blockCount >= LCR_SETTING_MAP_MAX_BLOCKS)
  {
    LCR_LOG0("couldn't add block");
    return 0;
  }

  uint32_t coord = LCR_mapBlockGetCoordNumber(block);
  uint16_t insertAt = 0;

  while (insertAt < LCR_currentMap.blockCount &&
    coord > LCR_mapBlockGetCoordNumber(LCR_currentMap.blocks +
    insertAt * LCR_BLOCK_SIZE))
    insertAt++;

  if (block[0] == LCR_BLOCK_NONE)
  {
    if (insertAt < LCR_currentMap.blockCount &&
      coord == LCR_mapBlockGetCoordNumber(LCR_currentMap.blocks +
      insertAt * LCR_BLOCK_SIZE))
    {
      // shift all left (remove the block):

      for (int i = insertAt * LCR_BLOCK_SIZE;
        i < (LCR_currentMap.blockCount - 1) * LCR_BLOCK_SIZE; ++i)
        LCR_currentMap.blocks[i] = LCR_currentMap.blocks[i + LCR_BLOCK_SIZE];

      LCR_currentMap.blockCount--;
    }
 
    return 1;
  }

  if (insertAt == LCR_currentMap.blockCount ||
    coord != LCR_mapBlockGetCoordNumber(LCR_currentMap.blocks +
    insertAt * LCR_BLOCK_SIZE))
  {
    // shift from here to the right, make room for the new block

    LCR_currentMap.blockCount++;

    for (int16_t i = ((int16_t) LCR_currentMap.blockCount) - 1;
      i > insertAt; i--)
      for (uint8_t j = 0; j < LCR_BLOCK_SIZE; ++j)
        LCR_currentMap.blocks[i * LCR_BLOCK_SIZE + j] =
          LCR_currentMap.blocks[(i - 1) * LCR_BLOCK_SIZE + j];
  }

  insertAt *= LCR_BLOCK_SIZE;
    
  for (uint8_t j = 0; j < LCR_BLOCK_SIZE; ++j)
    LCR_currentMap.blocks[insertAt + j] = block[j]; 

  return 1;
}

/**
  Resets the map to start a run (including e.g. unmarking checkpoints etc.).
*/
void LCR_mapReset(void)
{
  LCR_LOG0("resetting map");

  for (int i = 0; i < LCR_currentMap.blockCount; ++i)
    if (LCR_currentMap.blocks[i * LCR_BLOCK_SIZE] == LCR_BLOCK_CHECKPOINT_1)
      LCR_currentMap.blocks[i * LCR_BLOCK_SIZE] = LCR_BLOCK_CHECKPOINT_0;
}

int _LCR_mapCharToCoord(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 36;

  if (c == '@')
    return 62;

  if (c == '&')
    return 63;

  return -1;
}

void _LCR_mapComputeHash(void)
{
  LCR_LOG1("computing map hash");

  const uint8_t *data = LCR_currentMap.startPos;

  LCR_currentMap.hash = 11 + LCR_currentMap.environment;

  for (int i = 0; i < 4 + LCR_currentMap.blockCount * LCR_BLOCK_SIZE; ++i)
  {
    LCR_currentMap.hash = LCR_currentMap.hash * 101 + *data;
    data = (i != 3) ? data + 1 : LCR_currentMap.blocks;
  }

  LCR_currentMap.hash *= 251;
  LCR_currentMap.hash = ((LCR_currentMap.hash << 19) |
    (LCR_currentMap.hash >> 13)) * 113;
}

/**
  Same as LCR_mapGetBlockAt, but allows to specify start and end block of the
  of the search to make it faster.
*/
int LCR_mapGetBlockAtFast(uint8_t x, uint8_t y, uint8_t z, int start, int end)
{
  // binary search (the blocks are sorted)

  if ((x >= 64) | (y >= 64) | (z >= 64))
    return -1;

  uint32_t n = LCR_mapBlockCoordsToCoordNumber(x,y,z);
  uint8_t cacheIndex = 2 * ((x % 2) | ((y % 2) << 1) | ((z % 2) << 2));
  
  if (_LCR_mapBlockCache[cacheIndex] == n)
    switch (_LCR_mapBlockCache[cacheIndex + 1])
    {
      case 0xffffffff: return -1; break;
      case 0xfffffffe: break;
      default: return ((int) _LCR_mapBlockCache[cacheIndex + 1]); break;
    }

  _LCR_mapBlockCache[cacheIndex] = n;

  while (start <= end)
  {
    int m = (start + end) / 2;
 
    uint32_t n2 = LCR_mapBlockGetCoordNumber(
      LCR_currentMap.blocks + m * LCR_BLOCK_SIZE);

    if (n2 < n)
      start = m + 1;
    else if (n2 > n)
      end = m - 1;
    else
    {
      _LCR_mapBlockCache[cacheIndex + 1] = m;
      return m;
    }
  }

  _LCR_mapBlockCache[cacheIndex + 1] = 0xffffffff;
  return -1;
}

/**
  Gets an index to a map block of the currently loaded map at given
  coordinates. If there is no block at given coordinates, -1 is returned.
*/
int LCR_mapGetBlockAt(uint8_t x, uint8_t y, uint8_t z)
{
  if (LCR_currentMap.blockCount == 0)
    return -1;

  return LCR_mapGetBlockAtFast(x,y,z,0,LCR_currentMap.blockCount - 1);
}

uint8_t LCR_mapLoadFromStr(char (*getNextCharFunc)(void), const char *name)
{
  LCR_LOG0("loading map string");

  LCR_LOG2("clearing map block cache")

  for (int i = 0; i < LCR_MAP_BLOCK_CACHE_SIZE; ++i)
    _LCR_mapBlockCache[i] = 0xfffffffe;

  for (int i = 0; i < LCR_MAP_NAME_MAX_LEN; ++i)
  {
    LCR_currentMap.name[i] = *name;
    LCR_currentMap.name[i + 1] = 0;
    
    if (*name == 0)
      break;

    name++;
  }

  char c;

  uint8_t prevBlock[LCR_BLOCK_SIZE];
  prevBlock[0] = LCR_BLOCK_NONE;

  for (int i = 0; i < 4; ++i)
    LCR_currentMap.startPos[i] = 0;

  LCR_currentMap.targetTime = 0;
  LCR_currentMap.checkpointCount = 0;
  LCR_currentMap.blockCount = 0;
  LCR_currentMap.environment = 0;
  LCR_currentMap.hash = 0;

  while (1) // read target time
  {
    c = getNextCharFunc();

    if (c >= '0' && c <= '9')
      LCR_currentMap.targetTime = LCR_currentMap.targetTime * 10 + c - '0';
    else
      break;
  }

  c = getNextCharFunc();

  if (c < '0' || c > '3')
  {
    LCR_LOG0("bad environment char");
    return 0;
  }

  LCR_currentMap.environment = c - '0';

  while (c)
  {
    if (c == LCR_BLOCK_START_CHAR)
    {
      uint8_t block = getNextCharFunc();
      uint8_t trans = 0;
      uint8_t mat = 0;
      int coords[3];

      if (block == LCR_BLOCK_QUIT)
        break;

      for (int i = 0; i < 3; ++i)
      {
        c = getNextCharFunc();
        coords[i] = _LCR_mapCharToCoord(c);

        if (coords[i] < 0)
        {
          LCR_LOG0("bad coord");
          return 0;
        }
      }

      c = getNextCharFunc();

      if (c >= '0' && c <= '3')
      {
        mat = c - '0';
        c = getNextCharFunc();
      }

      while (1)
      {
        if (c == '|')
          trans |= LCR_BLOCK_TRANSFORM_FLIP_H;
        else if (c == '-')
          trans |= LCR_BLOCK_TRANSFORM_FLIP_V;
        else if (c == 'L')
          trans |= LCR_BLOCK_TRANSFORM_ROT_90;
        else if (c == 'I')
          trans |= LCR_BLOCK_TRANSFORM_ROT_180;
        else if (c == 'J')
          trans |= LCR_BLOCK_TRANSFORM_ROT_270;
        else
          break;

        c = getNextCharFunc();
      }
 
      while (c && c != LCR_BLOCK_START_CHAR)
        c = getNextCharFunc();

      switch (block)
      {
        case LCR_BLOCK_MIRROR:
        {
          uint8_t x, y, z, m, transform, type;
          uint8_t tmpBlock[LCR_BLOCK_SIZE];
                  
          LCR_mapBlockGetCoords(prevBlock,&x,&y,&z);

          for (uint8_t k = 0; k < coords[2]; ++k)
            for (uint8_t j = 0; j < coords[1]; ++j)
              for (uint8_t i = 0; i < coords[0]; ++i)
              {
                int blockIndex = LCR_mapGetBlockAt(x + i,y + j,z + k);

                if (blockIndex >= 0)
                {
                  m = LCR_mapBlockGetMaterial(
                    LCR_currentMap.blocks + blockIndex * LCR_BLOCK_SIZE);

                  transform = LCR_mapBlockGetTransform(
                    LCR_currentMap.blocks + blockIndex * LCR_BLOCK_SIZE);

                  type = LCR_currentMap.blocks[blockIndex * LCR_BLOCK_SIZE];

                  for (uint8_t l = 1; l < 8; ++l)
                  {
                    int8_t x2 = x + i, y2 = y + j, z2 = z + k;
                    uint8_t t2 = transform;

                    if (l & 0x01)
                    {
                      x2 = x - 1 - i;
                      t2 = LCR_mapBlockFlipTransformX(t2);
                    }

                    if (l & 0x02)
                    {
                      y2 = y - 1 - j;
                      t2 ^= LCR_BLOCK_TRANSFORM_FLIP_V;
                    }

                    if (l & 0x04)
                    {
                      z2 = z - 1 - k;
                      t2 = LCR_mapBlockFlipTransformZ(t2);
                    }

                    if (
                      x2 >= 0 && x2 < LCR_MAP_SIZE_BLOCKS &&
                      y2 >= 0 && y2 < LCR_MAP_SIZE_BLOCKS &&
                      z2 >= 0 && z2 < LCR_MAP_SIZE_BLOCKS)
                    {
                      LCR_makeMapBlock(type,x2,y2,z2,m,t2,tmpBlock);

                      if (!_LCR_mapAddBlock(tmpBlock))
                        return 0;
                    }
                  }
                }
              }

          break;
        }

        case LCR_BLOCK_CUBOID_FILL:
        case LCR_BLOCK_CUBOID_HOLLOW:
        {
          uint8_t x, y, z, m, transform;
          uint8_t tmpBlock[LCR_BLOCK_SIZE];

          m = LCR_mapBlockGetMaterial(prevBlock);
          transform = LCR_mapBlockGetTransform(prevBlock);
          LCR_mapBlockGetCoords(prevBlock,&x,&y,&z);

          for (uint8_t k = 0; k < coords[2]; ++k)
            for (uint8_t j = 0; j < coords[1]; ++j)
              for (uint8_t i = 0; i < coords[0]; ++i)
                if ((block == LCR_BLOCK_CUBOID_FILL ||
                    k == 0 || k == coords[2] - 1 ||
                    j == 0 || j == coords[1] - 1 ||
                    i == 0 || i == coords[0] - 1) &&  
                    (x + i < LCR_MAP_SIZE_BLOCKS &&
                    y + j < LCR_MAP_SIZE_BLOCKS &&
                    z + k < LCR_MAP_SIZE_BLOCKS))
                {
                  LCR_makeMapBlock(prevBlock[0],x + i,y + j,z + k,m,transform,
                    tmpBlock);
         
                  if (!_LCR_mapAddBlock(tmpBlock))
                    return 0;
                }

          break;
        }

        case LCR_BLOCK_START:
          LCR_currentMap.startPos[0] = coords[0];
          LCR_currentMap.startPos[1] = coords[1];
          LCR_currentMap.startPos[2] = coords[2];
          LCR_currentMap.startPos[3] = trans;
          break;
                                            // kek, is this legit?  
        case LCR_BLOCK_CHECKPOINT_0:
          LCR_currentMap.checkpointCount++; // fall through
        case LCR_BLOCK_FULL:                // fall through
        case LCR_BLOCK_BOTTOM:              // fall through
        case LCR_BLOCK_LEFT:                // fall through
        case LCR_BLOCK_BOTTOM_LEFT:         // fall through
        case LCR_BLOCK_BOTTOM_LEFT_FRONT:   // fall through
        case LCR_BLOCK_LEFT_FRONT:          // fall through
        case LCR_BLOCK_RAMP:                // fall through
        case LCR_BLOCK_RAMP_34:             // fall through 
        case LCR_BLOCK_RAMP_12:             // fall through
        case LCR_BLOCK_RAMP_14:             // fall through
        case LCR_BLOCK_RAMP_12_UP:          // fall through
        case LCR_BLOCK_RAMP_CORNER:         // fall through
        case LCR_BLOCK_RAMP_CURVED_PLAT:    // fall through
        case LCR_BLOCK_RAMP_CURVED:         // fall through
        case LCR_BLOCK_RAMP_CURVED_WALL:    // fall through
        case LCR_BLOCK_RAMP_STEEP:          // fall through
        case LCR_BLOCK_CORNER:              // fall through
        case LCR_BLOCK_CORNER_12:           // fall through
        case LCR_BLOCK_HILL:                // fall through
        case LCR_BLOCK_BUMP:                // fall through
        case LCR_BLOCK_CORNER_CONVEX:       // fall through
        case LCR_BLOCK_CORNER_CONCAVE:      // fall through
        case LCR_BLOCK_FULL_ACCEL:          // fall through
        case LCR_BLOCK_BOTTOM_ACCEL:        // fall through
        case LCR_BLOCK_RAMP_ACCEL:          // fall through
        case LCR_BLOCK_FULL_FAN:            // fall through
        case LCR_BLOCK_RAMP_FAN:            // fall through
        case LCR_BLOCK_CHECKPOINT_1:        // fall through
        case LCR_BLOCK_FINISH:              // fall through
        case LCR_BLOCK_NONE:                // fall through
        {
          LCR_makeMapBlock(block,coords[0],coords[1],coords[2],mat,trans,
            prevBlock);

          if (!_LCR_mapAddBlock(prevBlock))
            return 0;

          break;
        }

        default:
          LCR_LOG0("bad block type");
          return 0;
          break;
      }
    }
    else
      c = getNextCharFunc();
  }

  _LCR_mapComputeHash();

  LCR_LOG1("map loaded, block count/hash:")
  LCR_LOG1_NUM(LCR_currentMap.blockCount)
  LCR_LOG1_NUM(LCR_currentMap.hash)

  LCR_mapReset();

  return 1;
}

uint8_t _LCR_encodeMapBlockCoords(uint8_t x, uint8_t y, uint8_t z)
{
  return (5 * 7) * z + 7 * y + x;
}

void _LCR_decodeMapBlockCoords(uint8_t byte, uint8_t *x, uint8_t *y, uint8_t *z)
{
  *x = (byte % 7);
  *y = ((byte / 7) % 5);
  *z = (byte / 35);
}

#define LCR_BLOCK_SHAPE_COORD_MAX 12

/**
  Decodes XYZ coordinates encoded in a byte returned by LCR_mapGetBlockShape.
  Each coordinate will be in range 0 to 12 (including both). This unusual range
  is intentional as it for example has an exact mid value.
*/
void LCR_decodeMapBlockCoords(uint8_t byte, uint8_t *x, uint8_t *y, uint8_t *z)
{
  _LCR_decodeMapBlockCoords(byte,x,y,z);
  *x *= 2;
  *y *= 3;
  *z *= 2;
}

void _LCR_addBlockShapeByte(uint8_t *bytes, uint8_t *byteCount,
  int x, int y, int z)
{
  if (*byteCount >= LCR_MAP_BLOCK_SHAPE_MAX_BYTES)
    return;

  bytes[*byteCount] = _LCR_encodeMapBlockCoords(x,y,z);
  *byteCount += 1;
}

/**
  Macro that transforms coordinates according to block transformation.
*/
#define LCR_TRANSFORM_COORDS(trans,cx,cy,cz,maxXZ,maxY)\
  if (trans & LCR_BLOCK_TRANSFORM_FLIP_H) cx = maxXZ - cx;\
  if (trans & 0x20) {         /* for both 90 and 270 */ \
    cx ^= cz; cz ^= cx; cx ^= cz; /* swap */ \
    cx = maxXZ - cx; } \
  if (trans & 0x40) {         /* for both 180 and 270 */ \
    cx = maxXZ - cx; \
    cz = maxXZ - cz; } \
  if (trans & LCR_BLOCK_TRANSFORM_FLIP_V) \
    cy = maxY - cy; 

/**
  Gets a shape of given map block type as a 3D model composed of triangles. The
  model is returned as an array of byte triplets (triangles), with each byte
  representing one coordinate. These coordinates can be decoded with
  LCR_decodeMapBlockCoords function.
*/
void LCR_mapGetBlockShape(uint8_t blockType, uint8_t transform,
  uint8_t bytes[LCR_MAP_BLOCK_SHAPE_MAX_BYTES], uint8_t *byteCount)
{
  /*
    The coordinate format is following: byte B specifies coordinates X (0 to 6)
    = B % 7, Y (vertical, 0 to 4) = (B / 7) % 5, Z (0 to 6) = B % 35. Helper
    side view grid:

    4 . . . . . . . ^
    3 . . . . . . . | y
    2 . . . . . . .
    1 . . . . . . .
    0 . . . . . . .
      0 1 2 3 4 5 6 -> x/z
  */

  *byteCount = 0;

  #define ADD(a,b,c) _LCR_addBlockShapeByte(bytes,byteCount,a,b,c);

  switch (blockType)
  {
    case LCR_BLOCK_CHECKPOINT_0:
    case LCR_BLOCK_CHECKPOINT_1:
    case LCR_BLOCK_FINISH:
      ADD(3,0,3) ADD(3,2,6) ADD(6,2,3)
      ADD(3,0,3) ADD(0,2,3) ADD(3,2,6)
      ADD(3,0,3) ADD(3,2,0) ADD(0,2,3)
      ADD(3,0,3) ADD(6,2,3) ADD(3,2,0)
      ADD(3,4,3) ADD(3,2,6) ADD(0,2,3)
      ADD(3,4,3) ADD(0,2,3) ADD(3,2,0)
      ADD(3,4,3) ADD(3,2,0) ADD(6,2,3)
      ADD(3,4,3) ADD(6,2,3) ADD(3,2,6)
      break;

    case LCR_BLOCK_FULL:
    case LCR_BLOCK_BOTTOM:
    case LCR_BLOCK_LEFT:
    case LCR_BLOCK_BOTTOM_LEFT:
    case LCR_BLOCK_BOTTOM_LEFT_FRONT:
    case LCR_BLOCK_LEFT_FRONT:
    case LCR_BLOCK_FULL_ACCEL:
    case LCR_BLOCK_FULL_FAN:
    case LCR_BLOCK_BOTTOM_ACCEL:
    {
      uint8_t 
        xRight = 6 >>
          ((blockType == LCR_BLOCK_LEFT) |
          (blockType == LCR_BLOCK_BOTTOM_LEFT) |
          (blockType == LCR_BLOCK_BOTTOM_LEFT_FRONT) |
          (blockType == LCR_BLOCK_LEFT_FRONT)),
        yTop = 4 >>
          ((blockType == LCR_BLOCK_BOTTOM) |
          (blockType == LCR_BLOCK_BOTTOM_ACCEL) |
          (blockType == LCR_BLOCK_BOTTOM_LEFT) |
          (blockType == LCR_BLOCK_BOTTOM_LEFT_FRONT)),
        zBack = 6 >>
          ((blockType == LCR_BLOCK_BOTTOM_LEFT_FRONT) |
          (blockType == LCR_BLOCK_LEFT_FRONT));

      ADD(0,0,0)      ADD(xRight,0,0)        ADD(xRight,yTop,0)     // front
      ADD(0,0,0)      ADD(xRight,yTop,0)     ADD(0,yTop,0)
      ADD(xRight,0,0) ADD(xRight,0,zBack)    ADD(xRight,yTop,zBack) // right
      ADD(xRight,0,0) ADD(xRight,yTop,zBack) ADD(xRight,yTop,0)
      ADD(0,0,0)      ADD(0,yTop,0)          ADD(0,yTop,zBack)      // left
      ADD(0,0,0)      ADD(0,yTop,zBack)      ADD(0,0,zBack)
      ADD(0,0,zBack)  ADD(0,yTop,zBack)      ADD(xRight,yTop,zBack) // back
      ADD(0,0,zBack)  ADD(xRight,yTop,zBack) ADD(xRight,0,zBack)
      ADD(0,yTop,0)   ADD(xRight,yTop,0)     ADD(xRight,yTop,zBack) // top
      ADD(0,yTop,0)   ADD(xRight,yTop,zBack) ADD(0,yTop,zBack)
      ADD(0,0,0)      ADD(xRight,0,zBack)    ADD(xRight,0,0)        // bottom
      ADD(0,0,0)      ADD(0,0,zBack)         ADD(xRight,0,zBack)
      break;
    }

    case LCR_BLOCK_RAMP_CURVED_PLAT:
      ADD(0,0,6) ADD(0,4,5) ADD(0,4,6) // left
      ADD(6,0,6) ADD(6,4,6) ADD(6,4,5) // right
      ADD(0,4,5) ADD(6,4,5) ADD(0,4,6) // top
      ADD(0,4,6) ADD(6,4,5) ADD(6,4,6)
      // fall through

    case LCR_BLOCK_RAMP_CURVED:
    {
      uint8_t plusZ = blockType == LCR_BLOCK_RAMP_CURVED;

      ADD(0,0,0)         ADD(6,0,0)         ADD(0,1,3 + plusZ) // ramp
      ADD(0,1,3 + plusZ) ADD(6,0,0)         ADD(6,1,3 + plusZ)
      ADD(0,1,3 + plusZ) ADD(6,1,3 + plusZ) ADD(0,2,4 + plusZ) // ramp
      ADD(0,2,4 + plusZ) ADD(6,1,3 + plusZ) ADD(6,2,4 + plusZ)
      ADD(0,2,4 + plusZ) ADD(6,2,4 + plusZ) ADD(0,4,5 + plusZ) // ramp
      ADD(0,4,5 + plusZ) ADD(6,2,4 + plusZ) ADD(6,4,5 + plusZ)
      ADD(0,0,0)         ADD(0,1,3 + plusZ) ADD(0,0,6)         // left
      ADD(0,0,6)         ADD(0,1,3 + plusZ) ADD(0,2,4 + plusZ)
      ADD(0,0,6)         ADD(0,2,4 + plusZ) ADD(0,4,5 + plusZ)
      ADD(6,0,0)         ADD(6,0,6)         ADD(6,1,3 + plusZ) // right
      ADD(6,0,6)         ADD(6,2,4 + plusZ) ADD(6,1,3 + plusZ)
      ADD(6,0,6)         ADD(6,4,5 + plusZ) ADD(6,2,4 + plusZ)
      ADD(0,0,6)         ADD(0,4,6)         ADD(6,0,6)         // back
      ADD(6,0,6)         ADD(0,4,6)         ADD(6,4,6)
      ADD(0,0,0)         ADD(6,0,6)         ADD(6,0,0)         // bottom
      ADD(0,0,0)         ADD(0,0,6)         ADD(6,0,6)
      break;
    }

    case LCR_BLOCK_RAMP_CURVED_WALL:
      ADD(0,0,0) ADD(5,0,0) ADD(0,1,3) // ramp
      ADD(0,1,3) ADD(5,0,0) ADD(5,1,3)
      ADD(0,1,3) ADD(5,1,3) ADD(0,2,4) // ramp
      ADD(0,2,4) ADD(5,1,3) ADD(5,2,4)
      ADD(0,2,4) ADD(5,2,4) ADD(0,4,5) // ramp
      ADD(0,4,5) ADD(5,2,4) ADD(5,4,5)
      ADD(0,4,5) ADD(5,4,5) ADD(0,4,6) // top
      ADD(0,4,6) ADD(5,4,5) ADD(6,4,6)
      ADD(5,4,5) ADD(6,4,0) ADD(6,4,6) // top
      ADD(5,4,5) ADD(5,4,0) ADD(6,4,0)
      ADD(5,4,0) ADD(5,4,5) ADD(5,2,4) // inner side
      ADD(5,4,0) ADD(5,2,4) ADD(5,1,3)
      ADD(5,4,0) ADD(5,1,3) ADD(5,0,0)
      ADD(5,4,0) ADD(5,0,0) ADD(6,4,0) // front
      ADD(6,4,0) ADD(5,0,0) ADD(6,0,0)
      ADD(6,4,0) ADD(6,0,0) ADD(6,4,6) // right
      ADD(6,4,6) ADD(6,0,0) ADD(6,0,6)
      ADD(0,0,6) ADD(0,4,5) ADD(0,4,6) // left
      ADD(0,0,6) ADD(0,2,4) ADD(0,4,5)
      ADD(0,0,6) ADD(0,1,3) ADD(0,2,4)
      ADD(0,0,6) ADD(0,0,0) ADD(0,1,3)
      ADD(0,0,6) ADD(0,4,6) ADD(6,0,6) // back
      ADD(6,0,6) ADD(0,4,6) ADD(6,4,6)
      ADD(0,0,0) ADD(6,0,6) ADD(6,0,0) // bottom
      ADD(0,0,0) ADD(0,0,6) ADD(6,0,6)
      break;

    case LCR_BLOCK_RAMP_12_UP:
      ADD(0,2,0) ADD(0,4,6) ADD(0,0,6) // side
      ADD(0,0,0) ADD(0,2,0) ADD(0,0,6)
      ADD(6,2,0) ADD(6,0,6) ADD(6,4,6) // side
      ADD(6,0,0) ADD(6,0,6) ADD(6,2,0)
      ADD(0,2,0) ADD(6,2,0) ADD(0,4,6) // top
      ADD(6,2,0) ADD(6,4,6) ADD(0,4,6)
      ADD(0,0,6) ADD(6,4,6) ADD(6,0,6) // back
      ADD(0,0,6) ADD(0,4,6) ADD(6,4,6)
      ADD(0,0,0) ADD(6,0,0) ADD(6,2,0) // front
      ADD(0,0,0) ADD(6,2,0) ADD(0,2,0)
      ADD(0,0,0) ADD(0,0,6) ADD(6,0,6) // bottom
      ADD(0,0,0) ADD(6,0,6) ADD(6,0,0)
      break;

    case LCR_BLOCK_RAMP:
    case LCR_BLOCK_RAMP_12:
    case LCR_BLOCK_RAMP_14:
    case LCR_BLOCK_RAMP_34:
    case LCR_BLOCK_RAMP_STEEP:
    case LCR_BLOCK_RAMP_ACCEL:
    case LCR_BLOCK_RAMP_FAN:
    {
      uint8_t front, top;
      LCR_rampGetDimensions(blockType,&top,&front);
      front = 6 - front;

      ADD(0,0,front) ADD(0,top,6)   ADD(0,0,6)     // side
      ADD(6,0,front) ADD(6,0,6)     ADD(6,top,6)
      ADD(0,0,front) ADD(6,0,front) ADD(0,top,6)   // top
      ADD(6,0,front) ADD(6,top,6)   ADD(0,top,6)
      ADD(0,0,6)     ADD(6,top,6)   ADD(6,0,6)     // back
      ADD(0,0,6)     ADD(0,top,6)   ADD(6,top,6)
      ADD(0,0,front) ADD(0,0,6)     ADD(6,0,6)     // bottom
      ADD(0,0,front) ADD(6,0,6)     ADD(6,0,front)

      break;
    }

    case LCR_BLOCK_CORNER:
    case LCR_BLOCK_CORNER_12:
    {
      uint8_t right = blockType == LCR_BLOCK_CORNER ? 6 : 3;

      ADD(0,0,0)     ADD(right,0,6) ADD(right,4,6) // front/right
      ADD(0,0,0)     ADD(right,4,6) ADD(0,4,0)
      ADD(0,0,0)     ADD(0,4,6)     ADD(0,0,6)     // left
      ADD(0,0,0)     ADD(0,4,0)     ADD(0,4,6)
      ADD(right,0,6) ADD(0,0,6)     ADD(0,4,6)     // back
      ADD(0,4,6)     ADD(right,4,6) ADD(right,0,6)
      ADD(0,4,0)     ADD(right,4,6) ADD(0,4,6)     // top
      ADD(0,0,6)     ADD(right,0,6) ADD(0,0,0)     // bottom

      break;
    }

    case LCR_BLOCK_CORNER_CONVEX:
    case LCR_BLOCK_CORNER_CONCAVE:
    {
      uint8_t
        mx = blockType == LCR_BLOCK_CORNER_CONVEX ? 4 : 2,
        mz = blockType == LCR_BLOCK_CORNER_CONVEX ? 2 : 4;

      ADD(0,0,0)     ADD(0,4,6)     ADD(0,0,6)     // left
      ADD(0,0,0)     ADD(0,4,0)     ADD(0,4,6)     // left
      ADD(6,0,6)     ADD(0,0,6)     ADD(0,4,6)     // back
      ADD(0,4,6)     ADD(6,4,6)     ADD(6,0,6)     // back
      ADD(0,0,0)     ADD(mx,4,mz)   ADD(0,4,0)     // right
      ADD(mx,0,mz)   ADD(mx,4,mz)   ADD(0,0,0)
      ADD(6,4,6)     ADD(mx,4,mz)   ADD(6,0,6)
      ADD(6,0,6)     ADD(mx,4,mz)   ADD(mx,0,mz)
      ADD(0,4,0)     ADD(mx,4,mz)   ADD(0,4,6)     // top
      ADD(0,4,6)     ADD(mx,4,mz)   ADD(6,4,6)
      ADD(0,0,0)     ADD(0,0,6)     ADD(mx,0,mz)   // bottom
      ADD(0,0,6)     ADD(6,0,6)     ADD(mx,0,mz)
      break;
    }

    case LCR_BLOCK_BUMP:
      ADD(3,0,0)     ADD(6,0,3)     ADD(3,1,3)     // top
      ADD(6,0,3)     ADD(3,0,6)     ADD(3,1,3)
      ADD(3,0,6)     ADD(0,0,3)     ADD(3,1,3)
      ADD(0,0,3)     ADD(3,0,0)     ADD(3,1,3)
      ADD(3,0,0)     ADD(3,0,6)     ADD(6,0,3)     // bottom
      ADD(3,0,6)     ADD(3,0,0)     ADD(0,0,3)     // bottom
      break;

    case LCR_BLOCK_RAMP_CORNER:
      ADD(6,0,6)     ADD(0,0,0)     ADD(6,4,0)     // diagonal
      ADD(6,0,6)     ADD(6,4,0)     ADD(6,0,0)     // right
      ADD(0,0,0)     ADD(6,0,0)     ADD(6,4,0)     // front
      ADD(0,0,0)     ADD(6,0,6)     ADD(6,0,0)     // bottom
      break;

    case LCR_BLOCK_HILL:
      ADD(0,0,0)     ADD(6,0,0)     ADD(0,2,1)     // front
      ADD(6,0,0)     ADD(6,2,1)     ADD(0,2,1)
      ADD(0,2,1)     ADD(6,2,1)     ADD(0,3,2)     // front 2
      ADD(6,2,1)     ADD(6,3,2)     ADD(0,3,2)
      ADD(0,3,2)     ADD(6,3,2)     ADD(0,4,4)     // front 3
      ADD(6,3,2)     ADD(6,4,4)     ADD(0,4,4)
      ADD(0,4,4)     ADD(6,4,4)     ADD(0,4,6)     // top
      ADD(6,4,4)     ADD(6,4,6)     ADD(0,4,6)
      ADD(0,0,0)     ADD(0,0,6)     ADD(6,0,0)     // bottom
      ADD(6,0,0)     ADD(0,0,6)     ADD(6,0,6)
      ADD(0,0,6)     ADD(0,4,6)     ADD(6,4,6)     // back
      ADD(0,0,6)     ADD(6,4,6)     ADD(6,0,6)
      ADD(0,0,0)     ADD(0,2,1)     ADD(0,0,6)     // left
      ADD(0,2,1)     ADD(0,3,2)     ADD(0,0,6)
      ADD(0,3,2)     ADD(0,4,4)     ADD(0,0,6)
      ADD(0,4,4)     ADD(0,4,6)     ADD(0,0,6)
      ADD(6,0,0)     ADD(6,0,6)     ADD(6,2,1)     // right
      ADD(6,2,1)     ADD(6,0,6)     ADD(6,3,2)
      ADD(6,3,2)     ADD(6,0,6)     ADD(6,4,4)
      ADD(6,4,4)     ADD(6,0,6)     ADD(6,4,6)
      break;

    default: break;
  }

  if (transform)
  {
    for (int i = 0; i < *byteCount; ++i)
    {
      uint8_t x, y, z;

      _LCR_decodeMapBlockCoords(bytes[i],&x,&y,&z);
      LCR_TRANSFORM_COORDS(transform,x,y,z,6,4)

      bytes[i] = _LCR_encodeMapBlockCoords(x,y,z);
    }

    if (((transform & LCR_BLOCK_TRANSFORM_FLIP_H) == 0) !=
      ((transform & LCR_BLOCK_TRANSFORM_FLIP_V) == 0)) // flip triangles
      for (int i = 0; i < *byteCount; i += 3)
      {
        uint8_t tmp = bytes[i];
        bytes[i] = bytes[i + 1];
        bytes[i + 1] = tmp;
      }
  }

  #undef ADD
}

#endif // guard
