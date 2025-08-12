/** @file frontend_helper.h

  Helper file for generic PC frontends that can use the stdio library, to avoid
  duplication of code. Just include it.
*/

#include <stdint.h>
#include <stdio.h>

// Before including this file, it's possible to define a quality preset:
#if PRESET_QUALITY == 1                       // ultra low
  #define LCR_SETTING_RESOLUTION_X 256
  #define LCR_SETTING_RESOLUTION_Y 200
  #define LCR_SETTING_POTATO_GRAPHICS 1
  #define LCR_SETTING_332_COLOR 1
  #define LCR_SETTING_FPS 20
  #define LCR_SETTING_CAR_SHADOW 0
  #define LCR_SETTING_CAR_ANIMATION_SUBDIVIDE 0
  #define LCR_SETTING_PARTICLES 0
  #define LCR_SETTING_TEXTURE_SUBSAMPLE 16
  #define LCR_SETTING_FOG 0
  #define LCR_SETTING_MUSIC 0
#elif PRESET_QUALITY == 2                     // low
  #define LCR_SETTING_RESOLUTION_X 512
  #define LCR_SETTING_RESOLUTION_Y 400
  #define LCR_SETTING_RESOLUTION_SUBDIVIDE 2
  #define LCR_SETTING_CAR_SHADOW 0
  #define LCR_SETTING_CAR_ANIMATION_SUBDIVIDE 0
  #define LCR_SETTING_PARTICLES 0
  #define LCR_SETTING_TEXTURE_SUBSAMPLE 8
  #define LCR_SETTING_FOG 0
  #define LCR_SETTING_FPS 25
#elif PRESET_QUALITY == 3                     // normal
  #define LCR_SETTING_RESOLUTION_X 800
  #define LCR_SETTING_RESOLUTION_Y 600
  #define LCR_SETTING_RESOLUTION_SUBDIVIDE 1
  #define LCR_SETTING_CAR_SHADOW 1
  #define LCR_SETTING_CAR_ANIMATION_SUBDIVIDE 1
  #define LCR_SETTING_PARTICLES 1
  #define LCR_SETTING_TEXTURE_SUBSAMPLE 4
  #define LCR_SETTING_FOG 0
  #define LCR_SETTING_FPS 30
#elif PRESET_QUALITY == 4                     // high
  #define LCR_SETTING_RESOLUTION_X 1024
  #define LCR_SETTING_RESOLUTION_Y 768
  #define LCR_SETTING_RESOLUTION_SUBDIVIDE 1
  #define LCR_SETTING_CAR_SHADOW 1
  #define LCR_SETTING_CAR_ANIMATION_SUBDIVIDE 1
  #define LCR_SETTING_PARTICLES 1
  #define LCR_SETTING_TEXTURE_SUBSAMPLE 1
  #define LCR_SETTING_FOG 1
  #define LCR_SETTING_FPS 45
#endif

#define DATA_FILE_NAME "data"

FILE *dataFile = 0;

char LCR_getNextDataFileChar(void)
{
#ifdef __EMSCRIPTEN__
  return 0;
#else
  if (!dataFile)
    return 0;

  int c = fgetc(dataFile);

  if (c == EOF)
  {
    rewind(dataFile);
    return 0;
  }

  return c;
#endif
}

void LCR_appendDataStr(const char *str)
{
#ifndef __EMSCRIPTEN__
  if (!dataFile)
    return;

  if (str == 0 || *str == 0)
    rewind(dataFile);
  else
  {
#if LCR_SETTING_LOG_LEVEL > 1
    printf("appending data: \"%s\"\n",str);
#endif 

    fclose(dataFile);
    
    dataFile = fopen(DATA_FILE_NAME,"a");
    
    if (dataFile)
    {
      fprintf(dataFile,"%s",str);
      fclose(dataFile);
      dataFile = fopen(DATA_FILE_NAME,"r");
    }
  }
#else
  printf("%s",str);
#endif
}

void LCR_log(const char *str)
{
  printf("%s\n",str);
}

void openDataFile(void)
{
#ifndef __EMSCRIPTEN__
  dataFile = fopen(DATA_FILE_NAME,"r");

  if (!dataFile)
    puts("WARNING: couldn't open data file");
#endif
}

void closeDataFile(void)
{
#ifndef __EMSCRIPTEN__
  if (dataFile)
    fclose(dataFile);
#endif
}
