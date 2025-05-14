//bindprompt.h
//Dialog box for binding an emulated input in Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _BINDPROMPT_H
#define _BINDPROMPT_H

#include <wx/wx.h>
#include "prefs.h"

class BindPrompt : public wxDialog
{
public:
	BindPrompt(wxWindow *parent, prefs_t &prefs_ref, int which_pad, int which_btn);

private:
	prefs_t &Prefs;
	int WhichPad;
	int WhichBtn;
	
	//Called when key is released
	void OnKeyUp(wxKeyEvent &event);
};

#endif //_BINDPROMPT_H

