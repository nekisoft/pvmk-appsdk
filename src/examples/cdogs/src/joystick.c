#include <string.h>
#include <stdio.h>
//#include <conio.h>
//#include <i86.h>
#include "joystick.h"

#include <sc.h> //pvmk


struct JoyRec gSticks[2];


#define VGA_PORT       0x03DA
#define STICK_PORT     0x0201
#define MAX_WAIT       60000



void PollSticks( int maxWait )
{
	(void)maxWait;
	/*
  int b;
  int mask;
  int laps;
  int waitCount;

  waitCount = (maxWait > 0 ? maxWait : MAX_WAIT);
  mask = 0;
  if (gSticks[0].present && gSticks[0].inUse)
    mask = 3;
  if (gSticks[1].present && gSticks[1].inUse)
    mask = mask | 12;

  gSticks[0].x = gSticks[0].y = gSticks[1].x = gSticks[1].y = 0;
  laps = 0;

  _disable();

  outp( STICK_PORT, 0xFF); // Write anything to trigger countdown
  do
  {
    b = inp( STICK_PORT);
    if (b&1)
      gSticks[0].x++;
    if (b&2)
      gSticks[0].y++;
    if (b&4)
      gSticks[1].x++;
    if (b&8)
      gSticks[1].y++;
    laps++;
  }
  while ((b&mask) != 0 && laps < waitCount);

  _enable();

  if (!maxWait)
  {
    gSticks[0].present = (b & 3) == 0;
    gSticks[1].present = (b & 12) == 0;
  }

  if (gSticks[0].present)
  {
    gSticks[0].buttons = (b >> 4) & 3;
    gSticks[0].buttons |= ((b >> 4) & 12);
  }

  if (gSticks[1].present)
  {
    gSticks[1].buttons = ((b >> 6) & 3);
    gSticks[1].buttons |= ((b >> 2) & 12);
  }
  */
}

void InitSticks( void )
{
 // gSticks[0].present = YES;
 // gSticks[1].present = YES;
 // gSticks[0].inUse = YES;
 // gSticks[1].inUse = YES;
 // PollSticks( 0);
}

void AutoCalibrate( void )
{
  //PollSticks( 0);
 // gSticks[0].xMid = gSticks[0].x;
  //gSticks[0].yMid = gSticks[0].y;
 // gSticks[1].xMid = gSticks[1].x;
 // gSticks[1].yMid = gSticks[1].y;
}

void PollDigiSticks( int *joy1, int *joy2 )
{
	/*
  int max = 0;

  if (gSticks[0].present && gSticks[0].inUse)
  {
    if (gSticks[0].xMid > max) max = gSticks[0].xMid;
    if (gSticks[0].yMid > max) max = gSticks[0].yMid;
  }
  if (gSticks[1].present && gSticks[1].inUse)
  {
    if (gSticks[1].xMid > max) max = gSticks[1].xMid;
    if (gSticks[1].yMid > max) max = gSticks[1].yMid;
  }
  max = (4*max)/3;
  PollSticks( max);

  if (joy1)
    *joy1 = 0;
  if (joy1 && gSticks[0].present)
  {
    if (gSticks[0].x < gSticks[0].xMid/2)
      *joy1 |= JOYSTICK_LEFT;
    else if (gSticks[0].x > (7*gSticks[0].xMid)/6)
      *joy1 |= JOYSTICK_RIGHT;
    if (gSticks[0].y < gSticks[0].yMid/2)
      *joy1 |= JOYSTICK_UP;
    else if (gSticks[0].y > (7*gSticks[0].yMid)/6)
      *joy1 |= JOYSTICK_DOWN;
    if ((gSticks[0].buttons & 1) == 0)
      *joy1 |= JOYSTICK_BUTTON1;
    if ((gSticks[0].buttons & 2) == 0)
      *joy1 |= JOYSTICK_BUTTON2;
    if (!joy2 || !gSticks[1].present)
    {
      if ((gSticks[0].buttons & 4) == 0)
        *joy1 |= JOYSTICK_BUTTON3;
      if ((gSticks[0].buttons & 8) == 0)
        *joy1 |= JOYSTICK_BUTTON4;
    }
  }
  if (joy2)
    *joy2 = 0;
  if (joy2 && gSticks[1].present)
  {
    if (gSticks[1].x < gSticks[1].xMid/2)
      *joy2 |= JOYSTICK_LEFT;
    else if (gSticks[1].x > (7*gSticks[1].xMid/6))
      *joy2 |= JOYSTICK_RIGHT;
    if (gSticks[1].y < gSticks[1].yMid/2)
      *joy2 |= JOYSTICK_UP;
    else if (gSticks[1].y > (7*gSticks[1].yMid/6))
      *joy2 |= JOYSTICK_DOWN;
    if ((gSticks[1].buttons & 1) == 0)
      *joy2 |= JOYSTICK_BUTTON1;
    if ((gSticks[1].buttons & 2) == 0)
      *joy2 |= JOYSTICK_BUTTON2;
    if (!joy1 || !gSticks[0].present)
    {
      if ((gSticks[1].buttons & 4) == 0)
        *joy2 |= JOYSTICK_BUTTON3;
      if ((gSticks[1].buttons & 8) == 0)
        *joy2 |= JOYSTICK_BUTTON4;
    }
  }
  */  
	//pvmk port
	_sc_input_t inp = {0};
	while(_sc_input(&inp, sizeof(inp), sizeof(inp)) > 0)
	{
		//Figure out which joystick data we'll write, based on who's sending the input
		int *bits = NULL;
		if(inp.format == 'A')
			bits = joy1;
		if(inp.format == 'B')
			bits = joy2;
		if(bits == NULL)
			continue;
		
		//Translate to CDogs input bits
		*bits = 0;
		static const int jsmap[16] = 
		{
			[_SC_BTNIDX_UP]    = JOYSTICK_UP,
			[_SC_BTNIDX_DOWN]  = JOYSTICK_DOWN,
			[_SC_BTNIDX_LEFT]  = JOYSTICK_LEFT,
			[_SC_BTNIDX_RIGHT] = JOYSTICK_RIGHT,
			[_SC_BTNIDX_A]     = JOYSTICK_BUTTON1,
			[_SC_BTNIDX_B]     = JOYSTICK_BUTTON2,
			[_SC_BTNIDX_C]     = JOYSTICK_BUTTON3,
			[_SC_BTNIDX_X]     = JOYSTICK_BUTTON4,
		};
		for(int bb = 0; bb < 16; bb++)
		{
			if(inp.buttons & (1u << bb))
				*bits |= jsmap[bb];
		}
	}
}

void EnableSticks( int joy1, int joy2 )
{
  gSticks[0].inUse = joy1;
  gSticks[1].inUse = joy2;
}
