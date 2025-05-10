//cfgwin.h
//Configuration window for Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2025
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