#include <stdlib.h>
//#include <conio.h>
//#include <dos.h>
//#include <i86.h>
#include "events.h"


static struct MouseRect *localRects  = NULL;
static struct MouseRect *localRects2 = NULL;


void InitMouse( void )
{
/*  union REGS in, out;

  in.w.ax = 0;
  int386( 0x33, &in, &out);*/
}

void Mouse( int *x, int *y, int *button )
{
	/*
  union REGS in, out;

  in.w.ax = 3;
  int386( 0x33, &in, &out);
  *x = out.w.cx;
  *y = out.w.dx;
  *button = out.w.bx;*/
	*x = 0;
	*y = 0;
	*button = 0;
}

int GetKey( void )
{
	/*
  int c = getch();

  if (c)
    return c;
  else
    return (getch() << 8);
	*/
	return 0;
}

void SetMouseRects( struct MouseRect *rects )
{
  localRects = rects;
  localRects2 = NULL;
}

void SetSecondaryMouseRects( struct MouseRect *rects )
{
  localRects2 = rects;
}

int GetMouseRectTag( int x, int y, int *tag )
{
  struct MouseRect *mRect;

  for (mRect = localRects; mRect && mRect->right > 0; mRect++)
  {
    if (y >= mRect->top && y <= mRect->bottom &&
        x >= mRect->left && x <= mRect->right)
    {
      *tag = mRect->tag;
      return 1;
    }
  }
  for (mRect = localRects2; mRect && mRect->right > 0; mRect++)
  {
    if (y >= mRect->top && y <= mRect->bottom &&
        x >= mRect->left && x <= mRect->right)
    {
      *tag = mRect->tag;
      return 1;
    }
  }
  return 0;
}


