#ifndef _LCR_GENERAL_H
#define _LCR_GENERAL_H

/** @file general.h

  Licar: general

  This file holds general definitions used by all Licar modules.

  All Licar code uses LCR_ (or _LCR_) prefix as a kind of namespace preventing
  collisions with 3rd party identifiers.
*/

#include <stdint.h>
#include "settings.h"

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__APPLE__)
  #warning "Your OS sucks, go fuck yourself."
#endif

#define LCR_EFFECTIVE_RESOLUTION_X \
  (LCR_SETTING_RESOLUTION_X / LCR_SETTING_RESOLUTION_SUBDIVIDE)

#define LCR_EFFECTIVE_RESOLUTION_Y \
  (LCR_SETTING_RESOLUTION_Y / LCR_SETTING_RESOLUTION_SUBDIVIDE)

#ifdef LCR_MODULE_NAME
  #undef LCR_MODULE_NAME
#endif

#define LCR_MODULE_NAME "general" ///< Used by logging functions.

#ifndef LCR_LOG0
  #define LCR_LOG0(s) ;
#endif

#ifndef LCR_LOG1
  #define LCR_LOG1(s) ;
#endif

#ifndef LCR_LOG2
  #define LCR_LOG2(s) ;
#endif

#ifndef LCR_LOG0_NUM
  #define LCR_LOG0_NUM(x) ;
#endif

#ifndef LCR_LOG1_NUM
  #define LCR_LOG1_NUM(x) ;
#endif

#ifndef LCR_LOG2_NUM
  #define LCR_LOG2_NUM(x) ;
#endif

#if LCR_SETTING_332_COLOR
  #define LCR_CONVERT_COLOR(c) \
    (((c & 0xe000) >> 8) | ((c & 0x0700) >> 6) | ((c & 0x001f) >> 3))
#else
  #define LCR_CONVERT_COLOR(c) c
#endif

char _LCR_hexDigit(int i)
{
  return i < 10 ? '0' + i : ('a' - 10 + i);
}

int _LCR_hexDigitVal(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  return -1;
}

/**
  Computes a simple hash of a string represented by a function returning next
  string character, ending at 0 or endChar. This is intended for simple (but
  not 100% reliable) string comparison.
*/
uint16_t _LCR_simpleStrHash(char (*nextChar)(void), char endChar)
{
  uint16_t r = 0;
 
  while (1)
  {
    char c = nextChar();

    if (c == 0 || c == endChar)
      break;

    r = ((r << 5) | (r >> 11)) + c;
  }

  return r;
}

int _LCR_strCmp(const char *s1, const char *s2)
{
  while (1)
  {
    if (*s1 != *s2)
      return 0;

    if (*s1 == 0)
      break;

    s1++;
    s2++;
  }
 
  return 1;
}

int _LCR_min(int a, int b)
{
  return a <= b ? a : b;
}

int _LCR_triangleWinding(int_least32_t x0, int_least32_t y0, int_least32_t x1,
  int_least32_t y1, int_least32_t x2, int_least32_t y2)
{
  x0 = (y1 - y0) * (x2 - x1) - (x1 - x0) * (y2 - y1);
  return x0 != 0 ? (x0 > 0 ? 1 : -1) : 0;
}

#undef LCR_MODULE_NAME
#endif // guard
