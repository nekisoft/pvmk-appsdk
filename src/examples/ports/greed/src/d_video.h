/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1995 by Softdisk Publishing                               */
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

#ifndef D_VIDEO_H
#define D_VIDEO_H

/**** CONSTANTS ****/

//#define outbyte(x, y) (outp((unsigned) (x), (unsigned) (y)))
//#define _outhword(x, y) (outpw((unsigned) (x), (unsigned) (y)))
//#define inbyte(x) (inp((unsigned) (x)))
//#define _inhword(x) (inpw((unsigned) (x)))
//#define CLI _disable()
//#define STI _enable()

#define SC_INDEX 0x3C4
#define SC_RESET 0
#define SC_CLOCK 1
#define SC_MAPMASK 2
#define SC_CHARMAP 3
#define SC_MEMMODE 4
#define CRTC_INDEX 0x3D4
#define CRTC_H_TOTAL 0
#define CRTC_H_DISPEND 1
#define CRTC_H_BLANK 2
#define CRTC_H_ENDBLANK 3
#define CRTC_H_RETRACE 4
#define CRTC_H_ENDRETRACE 5
#define CRTC_V_TOTAL 6
#define CRTC_OVERFLOW 7
#define CRTC_ROWSCAN 8
#define CRTC_MAXSCANLINE 9
#define CRTC_CURSORSTART 10
#define CRTC_CURSOREND 11
#define CRTC_STARTHIGH 12
#define CRTC_STARTLOW 13
#define CRTC_CURSORHIGH 14
#define CRTC_CURSORLOW 15
#define CRTC_V_RETRACE 16
#define CRTC_V_ENDRETRACE 17
#define CRTC_V_DISPEND 18
#define CRTC_OFFSET 19
#define CRTC_UNDERLINE 20
#define CRTC_V_BLANK 21
#define CRTC_V_ENDBLANK 22
#define CRTC_MODE 23
#define CRTC_LINECOMPARE 24
#define GC_INDEX 0x3CE
#define GC_SETRESET 0
#define GC_ENABLESETRESET 1
#define GC_COLORCOMPARE 2
#define GC_DATAROTATE 3
#define GC_READMAP 4
#define GC_MODE 5
#define GC_MISCELLANEOUS 6
#define GC_COLORDONTCARE 7
#define GC_BITMASK 8
#define ATR_INDEX 0x3c0
#define ATR_MODE 16
#define ATR_OVERSCAN 17
#define ATR_COLORPLANEENABLE 18
#define ATR_PELPAN 19
#define ATR_COLORSELECT 20
#define STATUS_REGISTER_1 0x3da
#define PEL_WRITE_ADR 0x3c8
#define PEL_READ_ADR 0x3c7
#define PEL_DATA 0x3c9

#define SCREENWIDTH 320
#define SCREENHEIGHT 200
#define SCREENPX (SCREENWIDTH*SCREENHEIGHT)
#define SCREENBYTES (SCREENPX*1)


/**** VARIABLES ****/

//#pragma noalign(pic_t)
typedef struct
{
    short width, height;
    short orgx, orgy;
    byte  data;
} __attribute__((packed)) pic_t;

extern byte* screen;
extern byte* ylookup[SCREENHEIGHT];
extern byte* transparency;
extern byte* translookup[255];


/**** FUNCTIONS ****/

void VI_Init(int specialbuffer);
void VI_SetVGAMode(void);
void VI_SetTextMode(void);
void VI_WaitVBL(int vbls);
void VI_SetPalette(byte* pallete);
void VI_GetPalette(byte* pallete);
void VI_FillPalette(int red, int green, int blue);
void VI_SetColor(int color, int red, int green, int blue);
void VI_GetColor(int color, int* red, int* green, int* blue);
void VI_FadeOut(int start, int end, int red, int green, int blue, int steps);
void VI_FadeIn(int start, int end, byte* pallete, int steps);
void VI_ColorBorder(int color);
void VI_Plot(int x, int y, int color);
void VI_Hlin(int x, int y, int width, int color);
void VI_Vlin(int x, int y, int height, int color);
void VI_Bar(int x, int y, int width, int height, int color);
void VI_DrawPic(int x, int y, pic_t* pic);
void VI_DrawMaskedPic(int x, int y, pic_t* pic);
void VI_DrawMaskedPicToBuffer(int x, int y, pic_t* pic);
void VI_DrawMaskedPicToBuffer2(int x, int y, pic_t* pic);
void VI_DrawTransPicToBuffer(int x, int y, pic_t* pic);

//#pragma aux VI_DrawMaskedPicToBuffer "*" parm[];


#endif
