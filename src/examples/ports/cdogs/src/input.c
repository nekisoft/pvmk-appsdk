#include <stdlib.h>
#include "input.h"
#include "joystick.h"
#include "keyboard.h"
#include "defs.h"
#include "sounds.h"
#include "gamedata.h"
#include "grafx.h"    // Need vsync...


static int SwapButtons( int cmd )
{
  int c = (cmd & ~(CMD_BUTTON1 | CMD_BUTTON2));
  if (cmd & CMD_BUTTON1)
    c |= CMD_BUTTON2;
  if (cmd & CMD_BUTTON2)
    c |= CMD_BUTTON1;
  return c;
}

static void GetOnePlayerCmd( struct PlayerData *data,
                             int *cmd,
                             int joy1, int joy2,
                             int single )
{
  if (cmd)
  {
    if (data->controls == JOYSTICK_ONE)
    {
      *cmd = joy1;
      if (gOptions.swapButtonsJoy1)
        *cmd = SwapButtons( *cmd);
      if (single)
      {
        if ((joy2 & CMD_BUTTON1) != 0)
          *cmd |= CMD_BUTTON3;
        if ((joy2 & CMD_BUTTON2) != 0)
          *cmd |= CMD_BUTTON4;
      }
    }
    else if (data->controls == JOYSTICK_TWO)
    {
      *cmd = joy2;
      if (gOptions.swapButtonsJoy2)
        *cmd = SwapButtons( *cmd);
      if (single)
      {
        if ((joy1 & CMD_BUTTON1) != 0)
          *cmd |= CMD_BUTTON3;
        if ((joy1 & CMD_BUTTON2) != 0)
          *cmd |= CMD_BUTTON4;
      }
    }
    else
    {
      *cmd = 0;
      if (KeyDown( data->keys[0]))
        *cmd |= CMD_LEFT;
      else if (KeyDown( data->keys[1]))
        *cmd |= CMD_RIGHT;
      if (KeyDown( data->keys[2]))
        *cmd |= CMD_UP;
      else if (KeyDown( data->keys[3]))
        *cmd |= CMD_DOWN;
      if (KeyDown( data->keys[4]))
        *cmd |= CMD_BUTTON1;
      if (KeyDown( data->keys[5]))
        *cmd |= CMD_BUTTON2;
    }
  }
}

void GetPlayerCmd( int *cmd1, int *cmd2 )
{
  int joy1 = 0, joy2 = 0;

  PollDigiSticks( &joy1, &joy2);
  GetOnePlayerCmd( &gPlayer1Data, cmd1, joy1, joy2, cmd2 == NULL);
  GetOnePlayerCmd( &gPlayer2Data, cmd2, joy1, joy2, cmd1 == NULL);
}

void GetMenuCmd( int *cmd )
{
  if (KeyDown( keyEsc))
  {
    *cmd = CMD_ESC;
    return;
  }
  if (KeyDown( keyF10))
    AutoCalibrate();
  if (KeyDown( keyF9))
  {
    gPlayer1Data.controls = KEYBOARD;
    gPlayer2Data.controls = KEYBOARD;
  }

  GetPlayerCmd( cmd, NULL);
  if (*cmd)
    return;

  if (KeyDown( keyArrowLeft))
    *cmd |= CMD_LEFT;
  else if (KeyDown( keyArrowRight))
    *cmd |= CMD_RIGHT;
  if (KeyDown( keyArrowUp))
    *cmd |= CMD_UP;
  else if (KeyDown( keyArrowDown))
    *cmd |= CMD_DOWN;
  if (KeyDown( keyEnter))
    *cmd |= CMD_BUTTON1;
  if (KeyDown( keyBackspace))
    *cmd |= CMD_BUTTON2;
}

void WaitForRelease( void )
{
  int cmd1, cmd2;
  int releaseCount = 0;

  do
  {
    GetPlayerCmd( &cmd1, &cmd2);
    if (((cmd1 | cmd2) & (CMD_BUTTON1 | CMD_BUTTON2)) != 0 || AnyKeyDown())
      releaseCount = 0;
    else
      releaseCount++;
    SoundTick();
    vsync();
  }
  while (releaseCount < 4);
}

void WaitForPress( void )
{
  int cmd1, cmd2;

  do
  {
    GetPlayerCmd( &cmd1, &cmd2);
    SoundTick();
  }
  while (((cmd1 | cmd2) & (CMD_BUTTON1 | CMD_BUTTON2)) == 0 &&
        !AnyKeyDown());
}

void Wait( void )
{
  WaitForRelease();
  WaitForPress();
  WaitForRelease();
}
