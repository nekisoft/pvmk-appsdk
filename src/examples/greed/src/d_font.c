/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1996 by Softdisk Publishing                               */
/*                                                                         */
/* Original Design:                                                        */
/*  John Carmack of id Software                                            */
/*                                                                         */
/* Enhancements by:                                                        */
/*  Robert Morgan of Channel 7............................Main Engine Code */
/*  Todd Lewis of Softdisk Publishing......Tools,Utilities,Special Effects */
/*  John Bianca of Softdisk Publishing..............Low-level Optimization */
/*  Carlos Hasan..........................................Music/Sound Code */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "d_global.h"
#include "d_video.h"
#include "d_misc.h"
#include "d_font.h"
#include "d_ints.h"
#include "r_public.h"
#include "protos.h"


/**** VARIABLES ****/

#define MSGQUESIZE 3

font_t* font;
int     fontbasecolor;
int     fontspacing = 1;
char    str[MAXPRINTF];  // general purpose string buffer
char    msgstr[MSGQUESIZE][MAXPRINTF];
int     printx, printy;  // the printing position (top left corner)
longint msgtime;


/**** FUNCTIONS ****/

void FN_RawPrint4(char* str)
/* Draws a string of characters to the buffer */
{
    byte  b;
    byte *dest, *source;
    int   width, height, y, yh;
    char  ch;

    dest   = viewylookup[printy] + printx;
    height = font->height;
    while ((ch = *str++) != 0)
    {
        width  = font->width[(unsigned char)ch];
        source = ((byte*) font) + font->charofs[(unsigned char)ch];
        while (width--)
        {
            for (y = 0, yh = 0; y < height; y++, yh += windowWidth)
            {
                b = *source++;
                if (b)
                    dest[yh] = fontbasecolor + b;
                else
                    dest[yh] = 0;
            }
            dest++;
            printx++;
        }
        dest += fontspacing;
        printx += fontspacing;
    }
}


void FN_RawPrint2(char* str)
/* Draws a string of characters to the buffer */
{
    byte  b;
    byte *dest, *source;
    int   width, height, y, yh;
    char  ch;

    dest   = viewylookup[printy] + printx;
    height = font->height;
    while ((ch = *str++) != 0)
    {
        width  = font->width[(unsigned char)ch];
        source = ((byte*) font) + font->charofs[(unsigned char)ch];
        while (width--)
        {
            for (y = 0, yh = 0; y < height; y++, yh += windowWidth)
            {
                b = *source++;
                if (b)
                    dest[yh] = fontbasecolor + b;
            }
            dest++;
            printx++;
        }
        dest += fontspacing;
        printx += fontspacing;
    }
}


void FN_RawPrint(char* str)
/* Draws a string of characters to the screen */
{
    byte  b;
    byte *dest, *source;
    int   width, height, y, yh;
    char  ch;

    dest   = ylookup[printy] + printx;
    height = font->height;
    while ((ch = *str++) != 0)
    {
        width  = font->width[(unsigned char)ch];
        source = ((byte*) font) + font->charofs[(unsigned char)ch];
        while (width--)
        {
            for (y = 0, yh = 0; y < height; y++, yh += 320)
            {
                b = *source++;
                if (b)
                    dest[yh] = fontbasecolor + b;
                else
                    dest[yh] = 0;
            }
            dest++;
            printx++;
        }
        dest += fontspacing;
        printx += fontspacing;
    }
}


void FN_RawPrint3(char* str)
/* Draws a string of characters to the screen */
{
    byte  b;
    byte *dest, *source;
    int   width, height, y, yh;
    char  ch;
	
    dest   = ylookup[printy] + printx;
    height = font->height;
    while ((ch = *str++) != 0)
    {
        width  = font->width[(unsigned char)ch];
        source = ((byte*) font) + font->charofs[(unsigned char)ch];
        while (width--)
        {
            for (y = 0, yh = 0; y < height; y++, yh += 320)
            {
                b = *source++;
                if (b)
                    dest[yh] = fontbasecolor + b;
            }
            dest++;
            printx++;
        }
        dest += fontspacing;
        printx += fontspacing;
    }
}


int FN_RawWidth(char* str)
/* Returns the width of a string
   Does NOT handle newlines     */
{
    int width;

    width = 0;
    while (*str)
    {
        width += font->width[(unsigned char)(*str++)];
        width += fontspacing;
    }
    return width;
}


void FN_Print(char* s)
/* Prints a string in the current window, with newlines
   going down a line and back to 0 */
{
    char     c, *se;
    unsigned h;

    h = font->height;
    while (*s)
    {
        se = s;
        c  = *se;
        while (c && c != '\n')
            c = *++se;
        *se = '\0';
        FN_RawPrint(s);
        s = se;
        if (c)
        {
            *se = c;
            s++;
            printx = 0;
            printy += h;
        }
    }
}


void FN_PrintCentered(char* s)
/* Prints a multi line string with each line centered */
{
    char     c, *se;
    unsigned w, h;

    h = font->height;
    while (*s)
    {
        se = s;
        c  = *se;
        while (c && c != '\n')
            c = *++se;
        *se    = '\0';
        w      = FN_RawWidth(s);
        printx = (320 - w) / 2;
        FN_RawPrint3(s);
        s = se;
        if (c)
        {
            *se = c;
            s++;
            printx = 0;
            printy += h;
        }
    }
}


void FN_Printf(char* fmt, ...)
/* Prints a printf style formatted string at the current print position
    using the current print routines  */
{
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(str, fmt, argptr);
    va_end(argptr);
#ifdef PARMCHECK
    if (cnt >= MAXPRINTF)
        MS_Error("FN_Printf: String too long: %s", fmt);
#endif
    FN_Print(str);
}


void FN_CenterPrintf(char* fmt, ...)
/* As FN_Printf, but centers each line of text in the window bounds */
{
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(str, fmt, argptr);
    va_end(argptr);
#ifdef PARMCHECK
    if (cnt >= MAXPRINTF)
        MS_Error("FN_CPrintf: String too long: %s", fmt);
#endif
    FN_PrintCentered(str);
}


void FN_BlockCenterPrintf(char* fmt, ...)
/* As FN_CenterPrintf, but also enters the entire set of lines vertically in
   the window bounds */
{
    va_list argptr;
    char*   s;
    int     height;

    va_start(argptr, fmt);
    vsprintf(str, fmt, argptr);
    va_end(argptr);
#ifdef PARMCHECK
    if (cnt >= MAXPRINTF)
        MS_Error("FN_CCPrintf: String too long: %s", fmt);
#endif
    height = 1;
    s      = str;
    while (*s)
        if (*s++ == '\n')
            height++;
    height *= font->height;
    printy = 0 + (200 - height) / 2;
    FN_PrintCentered(str);
}


void rewritemsg(void)
/* write the current msg to the view buffer */
{
    int i;

    fontbasecolor = 73;
    font          = font1;
    for (i = 0; i < MSGQUESIZE; i++)
        if (msgstr[i][0])
        {
            printx = 2;
            printy = 1 + i * 6;
            FN_RawPrint2(msgstr[i]);
        }
    if (timecount > msgtime)
    {
        for (i = 1; i < MSGQUESIZE; i++)
            strcpy(msgstr[i - 1], msgstr[i]);
        msgstr[MSGQUESIZE - 1][0] = 0;
        msgtime                   = timecount + MSGTIME;
    }
}


void writemsg(char* s)
/* update current msg */
{
    int i;

    if (msgstr[MSGQUESIZE - 1][0] != 0)
        for (i = 1; i < MSGQUESIZE; i++)
            strcpy(msgstr[i - 1], msgstr[i]);
    strcpy(msgstr[MSGQUESIZE - 1], s);
    msgtime = timecount + MSGTIME;  // 10 secs
}
