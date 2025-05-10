//bindprompt.h
//Dialog box for binding an emulated input in Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2025
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

