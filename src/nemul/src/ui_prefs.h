//ui_prefs.h
//Preferences window for the Neki32 simulator front end
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _UI_PREFS_H
#define _UI_PREFS_H

#include <stdbool.h>
#include "prefs.h"

//State of the preferences window.
//Replaces cfgwin.cpp (the notebook of pad bindings) and bindprompt.cpp (the modal that ate one
//keypress). Both become one window here, because an ImGui modal for a single key is more
//machinery than the key is worth.
typedef struct ui_prefs_s
{
	//Whether the window is up
	bool open;

	//Set for the frame the user pressed OK; the caller copies working over its prefs
	bool apply;

	//Preferences being edited, so Cancel can throw them away
	prefs_t working;

	//Pad and button waiting for a key, or -1 when nothing is being rebound
	int bind_pad;
	int bind_btn;

	//Set briefly after a successful save, so the window says so
	int saved_frames;

} ui_prefs_t;

//Opens the window on a copy of the current preferences
void ui_prefs_open(ui_prefs_t *st, const prefs_t *current);

//Draws the window
void ui_prefs_draw(ui_prefs_t *st);

//Offers a keypress to a pending binding. Returns true when it was taken, so the caller does
//not also treat it as pad input or a shortcut.
bool ui_prefs_take_key(ui_prefs_t *st, int keycode);

#endif //_UI_PREFS_H
