
#ifndef __MAP
#define __MAP

#define YMAX    64
#define XMAX    64

#define TILE_WIDTH      16
#define TILE_HEIGHT     12

#define X_TILES         21
#define X_TILES_HALF    11
#define Y_TILES         19

#define NO_WALK           1
#define NO_SEE            2
#define NO_SHOOT          4
#define IS_SHADOW         8
#define IS_WALL          16
#define OFFSET_PIC       32
#define IS_SHADOW2       64
#define TILE_TRIGGER    128

// This constant is used internally in draw, it is never set in the map
#define DELAY_DRAW      256

#define KIND_CHARACTER      0
#define KIND_PIC            1
#define KIND_MOBILEOBJECT   2
#define KIND_OBJECT         3

#define TILEITEM_IMPASSABLE     1
#define TILEITEM_CAN_BE_SHOT    2
#define TILEITEM_CAN_BE_TAKEN   4
#define TILEITEM_OBJECTIVE      (8 + 16 + 32 + 64 + 128)
#define OBJECTIVE_SHIFT         3


typedef void (*TileItemDrawFunc)( int, int, const void * );

struct TileItem
{
  int x, y;
  int w, h;
  int kind;
  int flags;
  void *data;
  TileItemDrawFunc drawFunc;
  struct TileItem *next;
  struct TileItem *nextToDisplay;
};
typedef struct TileItem TTileItem;


struct Tile
{
  int pic;
  int flags;
  TTileItem *things;
};
typedef struct Tile TTile;


struct Buffer
{
  int xTop, yTop;
  int xStart, yStart;
  int dx, dy;
  int width;
  TTile tiles[ Y_TILES][ X_TILES];
};

extern TTile gMap[ YMAX][ XMAX];
#define Map( x, y)  gMap[y][x]
#define HitWall(x,y) ((gMap[(y)/TILE_HEIGHT][(x)/TILE_WIDTH].flags & NO_WALK) != 0)

extern unsigned char gAutoMap[ YMAX][ XMAX];
#define AutoMap( x, y)  gAutoMap[y][x]

int CheckWall( int x, int y, int w, int h );
int HasHighAccess( void );
int IsHighAccess( int x, int y );
int MapAccessLevel( int x, int y );

void MoveTileItem( TTileItem *t, int x, int y );
void RemoveTileItem( TTileItem *t );
int ItemsCollide( const TTileItem *item1, const TTileItem *item2, int x, int y );
TTileItem *CheckTileItemCollision( TTileItem *item, int x, int y, int mask );

void SetupMap( void );
int OKforPlayer( int x, int y );
void ChangeFloor( int x, int y, int normal, int shadow );
void MarkAsSeen( int x, int y );
int ExploredPercentage( void );

#endif
