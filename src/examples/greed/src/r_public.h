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

#ifndef R_PUBLIC_H
#define R_PUBLIC_H

#include "viewsize.h"

/**** CONSTANTS ****/

typedef int32_t fixed_t; //pvmk

#define WALL_CONTACT 1
#define DOOR_CONTACT 2
#define FRACBITS 16
#define FRACUNIT (1 << FRACBITS)
#define TILEUNIT (64 * FRACUNIT)
#define HALFTILEUNIT (32 * FRACUNIT)
#define PI 3.14159265
#define MAPSIZE 64  // there must not be any 65th vertexes
#define MAPROWS 64
#define MAPCOLS 64
#define TILESHIFT 6
#define TILESIZE (1 << TILESHIFT)  // pixels to tile
#define TILEFRACSHIFT (TILESHIFT + FRACBITS)
#define TILEGLOBAL (1 << TILEFRACSHIFT)
#define ANGLES 1023
#define WEST 512
#define EAST 0
#define NORTH 256
#define SOUTH 768
#define DEGREE45 128
#define DEGREE45_2 64
#define MINZ (FRACUNIT / 2)

// first value is the maximum # of tiles to render outwards
#define MAXZ ((32 << (FRACBITS + TILESHIFT)) - 1)
#define MAXZLIGHT (MAXZ >> FRACBITS)
#define MAXDOORS 32
#define MAXSPRITES 700
#define MAXELEVATORS 128
#define MAXSPAWNAREAS 96
#define ANIM_LOOP_MASK 1
#define ANIM_CG_MASK 30
#define ANIM_MG_MASK 480
#define ANIM_SELFDEST 32768
#define ANIM_DELAY_MASK 32256


#define MINDIST (FRACUNIT * 12)
#define PLAYERSIZE (16 << FRACBITS)
#define FRACTILESHIFT (FRACBITS + TILESHIFT)

#define BACKDROPHEIGHT 100
#define MAXSCROLL 60
#define MAXSCROLL2 120

// flags in mapflags
#define FL_DOOR 128
#define FL_FLOOR 7
#define FL_CEILING 56
#define FL_AUX 64  // not used right now
#define FLS_FLOOR 0
#define FLS_CEILING 3
#define POLY_FLAT 0
#define POLY_SLOPE 1
#define POLY_ULTOLR 2
#define POLY_URTOLL 3
// additional POLY_??? can be defined from 4-7


#define STEP_COLOR 132
#define WALL_COLOR 127
#define TRANS_COLOR 79
#define DOOR_COLOR 73

/**** TYPES ****/

typedef byte pixel_t;

typedef enum
{
    rt_one,
    rt_four,
    rt_eight
} rotate_t;

typedef enum
{
    st_none,
    st_noclip,
    st_transparent,
    st_maxlight
} special_t;

typedef enum
{
    dr_horizontal,
    dr_vertical,
    dr_horizontal2,
    dr_vertical2
} orientation_t;

typedef enum
{
    E_NORMAL,
    E_SWITCHDOWN,
    E_SWITCHDOWN2,
    E_TIMED,
    E_SWITCHUP,
    E_SWAP,
    E_SECRET,
    E_TRIGGERED,
    E_PRESSUREHIGH,
    E_PRESSURELOW
} elevtype;


typedef struct scaleobj_s
{
    // list links, don't touch
    struct scaleobj_s *prev, *next;
    // modify this part whenever you want
    int       animation;
    longint   animationTime;    // must accept all possible tick values
    fixed_t   moveSpeed, zadj;  // zadj = height above floor
    fixed_t   x, y, z;          // global position of the BOTTOM of the shape
    fixed_t   lastx, lasty;
    int       basepic;  // lumpnum is spritelump+basepic+rotation
    rotate_t  rotate;
    int       angle;  // 0 - ANGLES
    int       angle2;
    int       movesize;  // how big he is
    boolean   active, nofalling;
    int       intelligence, bullet, enraged;
    longint   movetime, modetime, actiontime, scantime, firetime;
    int       heat, startpic, movemode;
    int       startspot;  // so it doesn't self destruct owner
    int       damage, hitpoints;
    int       type;     // id
    int       spawnid;  // who created it
    int       score;
    int       maxmove;
    int       regen;
    int       deathevent;
    fixed_t   height;       // vertical height
    special_t specialtype;  // transparent, no clipping
    int       scale;
} scaleobj_t;

typedef struct doorobj_s
{
    int tilex, tiley;
    // modify this part whenever you want
    boolean       doorOpen;
    boolean       doorOpening;
    boolean       doorClosing;
    boolean       doorBlocked;
    boolean       doorBumpable;
    int           doorSize;
    longint       doorTimer;
    byte          doorLocks;
    orientation_t orientation;  // probably only want to set this once
    boolean       transparent;  // set true if the pic has any masked areas
    int           pic;          // lumpnum is doorlump+pic
    int           height;       // should generally be set to the floor height
    fixed_t       position;     // range from 0 (open) - FRACUNIT*64 (closed
} doorobj_t;

typedef struct elevobj_s  // elevator structure
{
    struct elevobj_s *prev, *next;

    boolean  elevUp;  // going up?
    boolean  elevDown;
    int      position;   // height
    longint  elevTimer;  // time for each movement
    int      floor;      // set to floorheight[mapspot]
    int      ceiling;    // set to ceilingheight[mapspot]-64
    int      mapspot, speed, eval, endeval, nosave;
    elevtype type;
} elevobj_t;

typedef struct spawnarea_s
{
    int     mapspot;
    fixed_t mapx, mapy;
    int     type;
    longint time;
} spawnarea_t;


/**** VARIABLES ****/

extern int      actionflag;  // if set non 0, the action hook is called
extern pixel_t  viewbuffer[MAX_VIEW_WIDTH * MAX_VIEW_HEIGHT];
extern pixel_t* viewylookup[MAX_VIEW_HEIGHT];
extern int      spritelump, walllump, flatlump;
extern int      numsprites, numwalls, numflats;
extern int *    flattranslation, *walltranslation;
extern fixed_t  costable[ANGLES + 1], sintable[ANGLES + 1];
extern byte     westwall[MAPROWS * MAPCOLS];
extern byte     westflags[MAPROWS * MAPCOLS];
extern byte     northwall[MAPROWS * MAPCOLS];
extern byte     northflags[MAPROWS * MAPCOLS];
extern byte     floorpic[MAPROWS * MAPCOLS];
extern byte     floorflags[MAPROWS * MAPCOLS];
extern byte     ceilingpic[MAPROWS * MAPCOLS];
extern byte     ceilingflags[MAPROWS * MAPCOLS];
extern byte     floorheight[MAPROWS * MAPCOLS];
extern byte     ceilingheight[MAPROWS * MAPCOLS];
extern byte     floordef[MAPROWS * MAPCOLS];
extern byte     floordefflags[MAPROWS * MAPCOLS];
extern byte     ceilingdef[MAPROWS * MAPCOLS];
extern byte     ceilingdefflags[MAPROWS * MAPCOLS];
extern byte     maplights[MAPROWS * MAPCOLS];
extern byte     mapsprites[MAPROWS * MAPCOLS];
extern byte     mapslopes[MAPROWS * MAPCOLS];
extern byte     mapeffects[MAPROWS * MAPCOLS];
extern byte     mapflags[MAPROWS * MAPCOLS];
extern int      reallight[MAPROWS * MAPCOLS];
extern int windowHeight, windowWidth, viewLocation, windowSize, windowLeft, windowTop, scrollmin,
    scrollmax;
extern fixed_t mapcache_height[MAX_VIEW_HEIGHT + MAXSCROLL2];
extern int     frameon, framevalid[MAPROWS * MAPCOLS];
extern fixed_t CENTERY, CENTERX, SCALE, ISCALE;
extern boolean debugmode;


/**** FUNCTIONS *****/

void         RF_PreloadGraphics(void);
fixed_t      FIXEDMUL(fixed_t, fixed_t);
fixed_t      FIXEDDIV(fixed_t, fixed_t);
void         RF_Startup(void);
void         RF_ClearWorld(void);
doorobj_t*   RF_GetDoor(int tilex, int tiley);
scaleobj_t*  RF_GetSprite(void);
elevobj_t*   RF_GetElevator(void);
spawnarea_t* RF_GetSpawnArea(void);
void         RF_RemoveSprite(scaleobj_t* spr);
void         RF_RemoveElevator(elevobj_t* e);
fixed_t      RF_GetFloorZ(fixed_t x, fixed_t y);
fixed_t      RF_GetCeilingZ(fixed_t x, fixed_t y);
void         RF_SetLights(fixed_t intensity);
void         RF_SetActionHook(void (*hook)(void));
void         RF_CheckActionFlag(void);
void         RF_RenderView(fixed_t x, fixed_t y, fixed_t z, int angle);
void         RF_BlitView(void);
void         SetViewSize(int width, int height);


//#pragma aux RF_BlitView "*" parm[];

/*#pragma aux FIXEDMUL =      \
   "imul    ecx          "  \
   "add     eax, 0x8000  "  \
   "adc     edx, 0       "  \
   "mov     ax, dx       "  \
   "rol     eax, 16      "  \
   parm [eax] [ecx]         \
   modify exact [eax edx]; */

//#pragma aux FIXEDMUL = "imul    ebx          "                                                     
//                       "shrd    eax, edx, 16 " parm[eax][ebx] modify[edx];

/*#pragma aux FIXEDDIV =      \
   "sub cx, cx    " \
   "and eax, eax  " \
   "jns FDP1      " \
   "inc cx        " \
   "neg eax       " \
   "FDP1: sub edx, edx " \
   "rol eax, 16   " \
   "mov dx, ax    " \
   "sub ax, ax    " \
   "and ebx, ebx  " \
   "jns FDP2      " \
   "dec cx        " \
   "neg ebx       " \
   "FDP2: div ebx " \
   "shr ebx, 1    " \
   "adc ebx, 0    " \
   "dec ebx       " \
   "cmp ebx, edx  " \
   "adc eax, 0    " \
   "and cx, cx    " \
   "jz FDP3       " \
   "neg eax       " \
   "FDP3:         " \
   parm [eax] [ebx]         \
   modify exact [eax ecx edx ebx]; */

/*   "mov     edx, eax     "  \
   "shl     eax, 16      "  \
   "sar     edx, 16      "  \
   "idiv    ecx          "  \
   "shld    edx, eax, 16 "  \ */

//#pragma aux FIXEDDIV = "cdq               "                                                        
                       //"shld edx, eax, 16 "                                                        
                       //"sal eax, 16       "                                                        
                       //"idiv ebx          " parm[eax][ebx] modify[edx];


//#pragma aux viewbuffer "*";
//#pragma aux viewLocation "*";
//#pragma aux viewylookup "*";
//#pragma aux windowHeight "*";
//#pragma aux windowWidth "*";

#include "protos.h"

#endif
