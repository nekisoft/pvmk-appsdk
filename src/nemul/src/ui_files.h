//ui_files.h
//Disk-image picker for the Neki32 simulator front end
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _UI_FILES_H
#define _UI_FILES_H

#include <stdbool.h>

//State of the in-window file picker.
//wxWidgets brought a native file dialog; Dear ImGui does not, and pulling in a dialog library
//would defeat the point of dropping the toolkit, so the browser is part of the front end.
typedef struct ui_files_s
{
	//Whether the window is up, and whether the user picked something this frame
	bool open;
	bool done;

	//Directory being listed, and the file chosen out of it
	char dir[2048];
	char result[2048];

	//Why the last listing or open failed, shown in the window
	char error[512];

	//Only offer *.iso disk images
	bool filter_iso;

	//Index into the listing that is selected, and the entry to scroll to
	int selected;
	int scroll_to;

} ui_files_t;

//Opens the browser at the given path (a directory, or a file whose directory is used)
void ui_files_open(ui_files_t *st, const char *startpath, bool filter_iso);

//Draws the browser. On the frame the user picks a file, sets done and puts the path in result.
void ui_files_draw(ui_files_t *st);

#endif //_UI_FILES_H
