//cfgwin.cpp
//Configuration window for Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2025

#include "cfgwin.h"
#include "bindprompt.h"
#include <stdio.h>
#include <wx/notebook.h>


CfgWin::CfgWin(wxWindow *parent, const prefs_t &prefs_in)
: wxDialog(parent, wxID_ANY, "Preferences")
{
	//Initialize the preferences we'll alter, with the input (existing) values
	MutablePrefs = prefs_in;
	
	//Make layout...
	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
	wxNotebook *controls_notebook = new wxNotebook(
		this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT, "Controls");
	for(int pp = 0; pp < 4; pp++)
	{
		char boxheader[16] = {0};
		snprintf(boxheader, sizeof(boxheader)-1, "Pad %c", 'A'+pp);
		
		wxPanel *controls_panel = new wxPanel(controls_notebook);
		wxBoxSizer *controls_sizer = new wxBoxSizer(wxVERTICAL);
		
		char panelheader[32] = {0};
		snprintf(panelheader, sizeof(panelheader)-1, "Pad %c Input Bindings", 'A'+pp);
		
		controls_sizer->Add(new wxStaticText(controls_panel, wxID_ANY, panelheader), 0, wxCENTER | wxALL, 12);
		
		static const char *btnnames[] = 
		{
			"Up", "Left", "Down", "Right", "A", "B", "C", "X", "Y", "Z", "Start", "Mode", NULL
		};
		for(int cc = 0; btnnames[cc] != NULL; cc++)
		{
			wxBoxSizer *line_sizer = new wxBoxSizer(wxHORIZONTAL);
			line_sizer->Add(new wxStaticText(controls_panel, wxID_ANY, btnnames[cc]), 0, wxCENTER);
			line_sizer->AddStretchSpacer(1);
			
			wxStaticText *control_val = new wxStaticText(controls_panel, wxID_ANY, GetControlValue(pp,cc));
			line_sizer->Add(control_val, 0, wxCENTER | wxLEFT | wxRIGHT, 16);
			ControlLabels[pp][cc] = control_val;
			
			wxButton *bind_button = new wxButton(controls_panel, wxID_ANY, "Key...");
			bind_button->Bind(wxEVT_BUTTON, &CfgWin::OnBind, this, bind_button->GetId());
			BindButtons[pp][cc] = bind_button;
			
			line_sizer->Add(bind_button, 0, wxEXPAND);
			
			controls_sizer->Add(line_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
		}
		
		controls_panel->SetSizerAndFit(controls_sizer);
		controls_notebook->AddPage(controls_panel, boxheader, false);
	}
	
	main_sizer->Add(controls_notebook, 1, wxEXPAND | wxALL, 8);
	
	//Buttons at the bottom
	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	if(button_sizer != NULL)
	{
		main_sizer->Add(button_sizer, 0, wxEXPAND);
	}
	
	//main_sizer->SetMinSize(wxSize(500,500));
	SetSizerAndFit(main_sizer);
}

void CfgWin::OnBind(wxCommandEvent &evt)
{
	//Work out what was clicked
	int pad = -1;
	int button = -1;
	for(int pp = 0; pp < 4; pp++)
	{
		for(int bb = 0; bb < 16; bb++)
		{
			if(BindButtons[pp][bb] == evt.GetEventObject())
			{
				pad = pp;
				button = bb;
				break;
			}
		}
		
		if(pad != -1)
			break;
	}
	if(pad == -1 || button == -1)
	{
		//...didn't come from a valid button?
		return;
	}
	
	//Show the prompt for rebinding that control
	BindPrompt *b = new BindPrompt(this, MutablePrefs, pad, button);
	if(b->ShowModal() == wxID_OK)
	{
		//Take new prefs
		//EmulPrefs = w->MutablePrefs;
	}
	else
	{
		//Discard changed prefs
	}
	
	//Update the control
	ControlLabels[pad][button]->SetLabel(GetControlValue(pad, button));
	
	b->Destroy();	
}

wxString CfgWin::GetControlValue(int pad, int button) const
{
	if(MutablePrefs.pads[pad].btn_src[button] == PREFS_PAD_SRC_KEY)
	{
		int bound_key = MutablePrefs.pads[pad].btn_val[button];
		if(bound_key != 0)
		{
			char ss[8] = {0};
			if(isprint(bound_key))
			{
				snprintf(ss, 7, "%c", (char)bound_key);
			}
			else
			{
				snprintf(ss, 7, "%d", bound_key);
			}
			return ss;
		}			
	}
	
	return "Unset";
}