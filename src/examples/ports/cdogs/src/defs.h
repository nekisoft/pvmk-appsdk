#ifndef __DEFS
#define __DEFS

#ifndef YES
#define YES            -1
#define NO             0
#endif

#define CDOGS_VERSION   "v1.06"

// Defines
#define BODY_UNARMED        0
#define BODY_ARMED          1
#define BODY_COUNT          2

#define FACE_JONES          0
#define FACE_ICE            1
#define FACE_OGRE           2
#define FACE_HAN            3
#define FACE_WARBABY        4
#define FACE_BUGEYE         5
#define FACE_BLONDIE        6
#define FACE_OGREBOSS       7
#define FACE_GRUNT          8
#define FACE_PROFESSOR      9
#define FACE_PIRAT1        10
#define FACE_PIRAT2        11
#define FACE_PIRAT3        12
#define FACE_MAD_BUGEYE    13
#define FACE_CYBORG        14
#define FACE_ROBOT         15
#define FACE_LADY          16
#define FACE_COUNT         17

#define GUNPIC_BLASTER      0
#define GUNPIC_KNIFE        1
#define GUNPIC_COUNT        2

// Commands
#define CMD_LEFT            1
#define CMD_RIGHT           2
#define CMD_UP              4
#define CMD_DOWN            8
#define CMD_BUTTON1        16
#define CMD_BUTTON2        32
#define CMD_BUTTON3        64
#define CMD_BUTTON4       128
#define CMD_ESC           256

// Command macros
#define Left(x)       (((x) & CMD_LEFT) != 0)
#define Right(x)      (((x) & CMD_RIGHT) != 0)
#define Up(x)         (((x) & CMD_UP) != 0)
#define Down(x)       (((x) & CMD_DOWN) != 0)
#define Button1(x)    (((x) & CMD_BUTTON1) != 0)
#define Button2(x)    (((x) & CMD_BUTTON2) != 0)
#define AnyButton(x)  (((x) & (CMD_BUTTON1 | CMD_BUTTON2)) != 0)

// Directions
#define DIRECTION_UP        0
#define DIRECTION_UPRIGHT   1
#define DIRECTION_RIGHT     2
#define DIRECTION_DOWNRIGHT 3
#define DIRECTION_DOWN      4
#define DIRECTION_DOWNLEFT  5
#define DIRECTION_LEFT      6
#define DIRECTION_UPLEFT    7
#define DIRECTION_COUNT     8

// States
#define STATE_IDLE          0
#define STATE_IDLELEFT      1
#define STATE_IDLERIGHT     2
#define STATE_WALKING_1     3
#define STATE_WALKING_2     4
#define STATE_WALKING_3     5
#define STATE_WALKING_4     6
#define STATE_SHOOTING      7
#define STATE_RECOIL        8
#define STATE_COUNT         9

// Gun states
#define GUNSTATE_READY      0
#define GUNSTATE_FIRING     1
#define GUNSTATE_RECOIL     2
#define GUNSTATE_COUNT      3


extern int cmd2dir[16];
extern int dir2cmd[8];
extern int dir2angle[8];

#define CmdToDirection(c)   cmd2dir[(c)&15]
#define DirectionToCmd(d)   dir2cmd[(d)&7]

void GetVectorsForAngle( int angle, int *dx, int *dy );


#endif
