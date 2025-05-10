//bindprompt.cpp
//Dialog box for binding an emulated input in Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2025

#include "bindprompt.h"
#include <stdio.h>

BindPrompt::BindPrompt(wxWindow *parent, prefs_t &prefs_ref, int which_pad, int which_btn)
: wxDialog(parent, wxID_ANY, "Bind Control"), Prefs(prefs_ref), WhichPad(which_pad), WhichBtn(which_btn)
{
	
	//Make layout...
	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
	
	//Label for what button we're configuring
	static const char *button_names[] = 
	{
		"Up", "Left", "Down", "Right", "A", "B", "C", "X", "Y", "Z", "Start", "Mode", NULL
	};
	char label_string[64] = {0};
	snprintf(label_string, sizeof(label_string)-1, "Press a key to use for Pad %c: %s Button...", 'A' + which_pad, button_names[which_btn]);
	wxStaticText *label_text = new wxStaticText(this, wxID_ANY, label_string);
	main_sizer->Add(label_text, 0, wxALL, 8);
	
	//Label for the current key binding
	//char setting_string[64] = {0};
	//snprintf(setting_string, sizeof(setting_string)-1, "Press key to set...");
	//wxStaticText *current_setting = new wxStaticText(this, wxID_ANY, setting_string);
	//main_sizer->Add(current_setting, 0, wxALL, 8);
	
	//Buttons at the bottom
	wxSizer *button_sizer = CreateButtonSizer(wxCANCEL);
	if(button_sizer != NULL)
	{
		main_sizer->Add(button_sizer, 0, wxEXPAND);
	}
	
	//Listen for keyboard input
	Bind(wxEVT_CHAR_HOOK, &BindPrompt::OnKeyUp, this);

	
	SetSizerAndFit(main_sizer);
}

void BindPrompt::OnKeyUp(wxKeyEvent &event)
{
	//Set the binding
	Prefs.pads[WhichPad].btn_src[WhichBtn] = PREFS_PAD_SRC_KEY;
	Prefs.pads[WhichPad].btn_val[WhichBtn] = event.GetKeyCode();
	EndModal(wxOK);
}