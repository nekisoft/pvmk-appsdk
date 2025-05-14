//prefs.h
//Nonvolatile user preferences in Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _PREFS_H
#define _PREFS_H

//Things that can trigger a gamepad button (input sources)
typedef enum prefs_pad_src_e
{
	PREFS_PAD_SRC_NONE = 0, //Unconfigured
	PREFS_PAD_SRC_KEY, //Keyboard
	PREFS_PAD_SRC_JOY, //Joystick via wxWidgets joystick interface
	PREFS_PAD_SRC_MAX //Number of different sources
} prefs_pad_src_t;

//Enumeration of button bits that can be trigered, as in real hardware
typedef enum prefs_pad_btn_e
{
	PREFS_PAD_BTN_UP    =  0,
	PREFS_PAD_BTN_LEFT  =  1,
	PREFS_PAD_BTN_DOWN  =  2,
	PREFS_PAD_BTN_RIGHT =  3,
	PREFS_PAD_BTN_A     =  4,
	PREFS_PAD_BTN_B     =  5,
	PREFS_PAD_BTN_C     =  6,
	PREFS_PAD_BTN_X     =  7,
	PREFS_PAD_BTN_Y     =  8,
	PREFS_PAD_BTN_Z     =  9,
	PREFS_PAD_BTN_START = 10,
	PREFS_PAD_BTN_MODE  = 11,
	PREFS_PAD_BTN_MAX //Number of buttons supported
} prefs_pad_btn_t;

//Configuration of a gamepad
typedef struct prefs_pad_s
{
	int             js_id; //Joystick ID used with this input
	prefs_pad_src_t btn_src[PREFS_PAD_BTN_MAX]; //Source of each button input, key or button
	int             btn_val[PREFS_PAD_BTN_MAX]; //Which key/button is used to trigger each button
} prefs_pad_t;

//Overall configuration
typedef struct prefs_s
{
	//Configuration of each gamepad
	#define PREFS_PAD_MAX 4
	prefs_pad_t pads[PREFS_PAD_MAX];
	
	//Configuration of GDB-RSP debug listener
	bool rsp_enabled;
	int rsp_port;
	
} prefs_t;

//Reads configuration or initializes defaults
void prefs_read(prefs_t *out);

//Writes configuration
void prefs_write(const prefs_t *in);

#endif //_PREFS_H
