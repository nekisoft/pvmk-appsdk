#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "grafx.h"
#include "blit.h"


#define FIRST_CHAR      0
#define LAST_CHAR       153
#define CHARS_IN_FONT   (LAST_CHAR - FIRST_CHAR + 1)

#define CHAR_INDEX(c) ((int)c - FIRST_CHAR)


static int dxText = 0;
static int xText = 0;
static int yText = 0;
static int hText = 0;
static void *font[ CHARS_IN_FONT];
static void *compiledFont[ CHARS_IN_FONT];
static void *rleFont[ CHARS_IN_FONT];


void TextInit( const char *filename, int offset, int compile, int rle )
{
  int i, h;

  dxText = offset;
  memset( font, 0, sizeof( font));
  memset( compiledFont, 0, sizeof( compiledFont));
  memset( rleFont, 0, sizeof( rleFont));
  ReadPics( filename, font, CHARS_IN_FONT, NULL);
  if (compile)
    printf( "Compiled font: %d bytes\n",
            CompilePics( CHARS_IN_FONT, font, compiledFont));
  if (rle)
    printf( "RLE font: %d bytes\n",
            RLEncodePics( CHARS_IN_FONT, font, rleFont));

  for (i = 0; i < CHARS_IN_FONT; i++)
  {
    h = PicHeight( font[i]);
    if (h > hText)
      hText = h;
  }
}

void TextChar( char c )
{
  int i = CHAR_INDEX(c);
  if (i >= 0 && i <= CHARS_IN_FONT && font[i])
  {
    DrawTPic( xText, yText, font[i], compiledFont[i]);
    xText += 1 + PicWidth( font[i]) + dxText;
  }
  else
  {
    i = CHAR_INDEX('.');
    DrawTPic( xText, yText, font[i], compiledFont[i]);
    xText += 1 + PicWidth( font[i]) + dxText;
  }
}

void TextCharWithTable( char c, TranslationTable *table )
{
  int i = CHAR_INDEX(c);
  if (i >= 0 && i <= CHARS_IN_FONT && font[i])
  {
    DrawTTPic( xText, yText, font[i], table, rleFont[i]);
    xText += 1 + PicWidth( font[i]) + dxText;
  }
  else
  {
    i = CHAR_INDEX('.');
    DrawTTPic( xText, yText, font[i], table, rleFont[i]);
    xText += 1 + PicWidth( font[i]) + dxText;
  }
}

void TextString( const char *s )
{
  while (*s)
    TextChar( *s++);
}

void TextStringWithTable( const char *s, TranslationTable *table )
{
  while (*s)
    TextCharWithTable( *s++, table);
}

void TextGoto( int x, int y )
{
  xText = x;
  yText = y;
}

void TextStringAt( int x, int y, const char *s )
{
  TextGoto( x, y);
  TextString( s);
}

void TextStringWithTableAt( int x, int y, const char *s, TranslationTable *table )
{
  TextGoto( x, y);
  TextStringWithTable( s, table);
}

int TextCharWidth( int c )
{
  if (c >= FIRST_CHAR && c <= LAST_CHAR && font[CHAR_INDEX(c)])
    return 1 + PicWidth( font[ CHAR_INDEX(c)]) + dxText;
  else
    return 1 + PicWidth( font[ CHAR_INDEX('.')]) + dxText;
}

int TextWidth( const char *s )
{
  int w = 0;

  while (*s)
    w += TextCharWidth( *s++);
  return w;
}

int TextHeight( void )
{
  return hText;
}
