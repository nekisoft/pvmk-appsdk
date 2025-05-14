//cfgwin.h
//Configuration window for Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _CFGWIN_H
#define _CFGWIN_H

#include <wx/wx.h>
#include "prefs.h"

class CfgWin : public wxDialog
{
public:
	CfgWin(wxWindow *parent, const prefs_t &prefs_in);

	prefs_t MutablePrefs;
private:
	
	//Bind button clicked
	void OnBind(wxCommandEvent &event);

	//Mapping between pad/button numbers and the wxWidgets Bind Button controls
	const wxButton *BindButtons[4][16];
	
	//Mapping between pad/button numbers and wxWidgets labels
	wxStaticText *ControlLabels[4][16];

	//Returns the value to show for a bound control
	wxString GetControlValue(int pad, int button) const;
};


#endif //_CFGWIN_H