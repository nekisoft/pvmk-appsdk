//nemul.cpp
//Entry point for Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#include <wx/wx.h>
#include <wx/filedlg.h>

#include "process.h"
#include "prefs.h"
#include "sysc.h"
#include "cfgwin.h"
#include "rsp.h"

/*
enum EmulCommands
{
	ID_Restart = 1,
};
*/

int tracing = 0;
prefs_t EmulPrefs;
int YelledAboutCrash = 0;

class EmulTimer : public wxTimer
{
private:
	wxPanel *ScreenPanel;
public:
	EmulTimer(wxPanel *ScreenPanel);
	void Notify();
	static bool RunMode;
};

bool EmulTimer::RunMode = false;
uint32_t EmulTimerTicks = 0;
uint32_t EmulVsyncs = 0;
int EmulDisk = -1;

uint16_t EmulPadState[PREFS_PAD_MAX] = {0};

EmulTimer::EmulTimer(wxPanel *ScreenPanel)
: wxTimer()
{
	this->ScreenPanel = ScreenPanel;
	Start(2, wxTIMER_CONTINUOUS); //For some reason 1ms ticks are unreliable
	RunMode = false;
}

void EmulTimer::Notify()
{
	for(int tt = 0; tt < 2; tt++) //Compensate for being 2ms instead of 1ms
	{
		rsp_poll();
		
		if(RunMode)
		{
			process_step();
			EmulTimerTicks++;
			if(EmulTimerTicks * 60ull > EmulVsyncs * 1000ull)
			{
				EmulVsyncs++;
				ScreenPanel->Refresh();
				
				//Check if there's a debug-stopped program with no debugger attached
				if(!rsp_present())
				{
					int nstopped = 0;
					process_t *pptr = NULL;
					for(int pp = 0; pp < PROCESS_MAX; pp++)
					{
						if(process_table[pp].dbgstop)
						{
							nstopped++;
							pptr = &(process_table[pp]);
						}
					}
					
					if(nstopped && !YelledAboutCrash)
					{
						//Haven't told the user about the crashed program
						static const char *reasons[PROCESS_DBGSTOP_MAX] = {0};
						reasons[PROCESS_DBGSTOP_CTRLC] = "Interrupted by User";
						reasons[PROCESS_DBGSTOP_SIGNAL] = "Signal Sent";
						reasons[PROCESS_DBGSTOP_BKPT] = "Breakpoint Hit";
						reasons[PROCESS_DBGSTOP_ABT] = "Data Abort";
						reasons[PROCESS_DBGSTOP_AC] = "Alignment Check";
						reasons[PROCESS_DBGSTOP_PF] = "Prefetch Abort";
						reasons[PROCESS_DBGSTOP_FATAL] = "Neki32 Interpreter Bug";
						
						wxString emsg = wxString::Format("PID %d stopped: %s at program location 0x%8.8X.",
							pptr->pid, reasons[pptr->dbgstop], pptr->regs[15]);
						
						dynamic_cast<wxFrame*>(wxGetTopLevelParent(ScreenPanel))->SetStatusText(emsg);
						
						wxString emsg2 = wxString::Format(
							"The process with PID %d has stopped due to a crash.\n\n"
							"The cause is: %s, at program location 0x%8.8X.\n\n"
							"Attach a debugger or restart the simulation to continue.",
							pptr->pid, reasons[pptr->dbgstop], pptr->regs[15]);
						
						wxMessageBox(emsg2, "Simulated Crash", wxICON_STOP);
						
						YelledAboutCrash = 1;
					}
					else if(YelledAboutCrash && (nstopped == 0))
					{
						//Simulation has been reset
						YelledAboutCrash = 0;
					}
				}
			}
		}
	}
}

class EmulScreenPanel : public wxPanel
{
public:
	EmulScreenPanel(wxFrame *parent);

	void paintEvent(wxPaintEvent &evt);
	void paintNow();
	void render(wxDC &dc);


	void OnKey(wxKeyEvent &event);

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(EmulScreenPanel, wxPanel)
EVT_PAINT(EmulScreenPanel::paintEvent)
END_EVENT_TABLE()

EmulScreenPanel::EmulScreenPanel(wxFrame *parent)
: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN | wxWANTS_CHARS)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinClientSize(wxSize(640,480));
	SetMaxClientSize(wxSize(640,480));
	
	Bind(wxEVT_KEY_DOWN, &EmulScreenPanel::OnKey, this);
	Bind(wxEVT_KEY_UP, &EmulScreenPanel::OnKey, this);
}

void EmulScreenPanel::paintEvent(wxPaintEvent &evt)
{
	(void)evt;
	wxPaintDC dc(this);
	render(dc);
}

void EmulScreenPanel::paintNow()
{
	wxClientDC dc(this);
	render(dc);
}

void EmulScreenPanel::render(wxDC &dc)
{
	//Copy the latest frame from the simulation to the screen
	wxBrush bb = dc.GetBackground();
	bb.SetColour(0,0,0);
	dc.SetBackground(bb);
	dc.Clear();
	
	//Get RGB565 buffer from emulation
	uint16_t *fb_ptr = NULL;
	int fb_mode = 0;
	sysc_popfbptr(&fb_ptr, &fb_mode);
	
	//Stretch out to 640x480 24bpp for wxWidgets to draw
	static unsigned char scaledpx[480][640][3];
	if(fb_mode == 0)
	{
		memset(scaledpx, 0, sizeof(scaledpx));
	}
	else if(fb_mode == 1)
	{
		//640x480 RGB565
		for(int yy = 0; yy < 480; yy++)
		{
			for(int xx = 0; xx < 640; xx++)
			{
				scaledpx[yy][xx][2] = (((*fb_ptr) >>  0) & 0x1F) << 3;
				scaledpx[yy][xx][1] = (((*fb_ptr) >>  5) & 0x3F) << 2;
				scaledpx[yy][xx][0] = (((*fb_ptr) >> 11) & 0x1F) << 3;
				fb_ptr++;
			}
		}
	}
	else if(fb_mode == 2)
	{
		//320x240 RGB565
		for(int yy = 0; yy < 480; yy += 2)
		{
			for(int xx = 0; xx < 640; xx += 2)
			{
				scaledpx[yy][xx][2] = (((*fb_ptr) >>  0) & 0x1F) << 3;
				scaledpx[yy][xx][1] = (((*fb_ptr) >>  5) & 0x3F) << 2;
				scaledpx[yy][xx][0] = (((*fb_ptr) >> 11) & 0x1F) << 3;
				fb_ptr++;
				
				for(int cc = 0; cc < 3; cc++)
				{
					scaledpx[yy+1][xx  ][cc] = scaledpx[yy][xx][cc];
					scaledpx[yy  ][xx+1][cc] = scaledpx[yy][xx][cc];
					scaledpx[yy+1][xx+1][cc] = scaledpx[yy][xx][cc];
				}
			}
		}
	}
	
	dc.DrawBitmap(wxBitmap(wxImage(640,480,&(scaledpx[0][0][0]),true)), 0, 0);
	
	//Submit new controls to emulation
	sysc_pushpads(EmulPadState);
}


void EmulScreenPanel::OnKey(wxKeyEvent &event)
{
	int bound = 0;
	
	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		for(int bb = 0; bb < PREFS_PAD_BTN_MAX; bb++)
		{
			if(EmulPrefs.pads[pp].btn_src[bb] != PREFS_PAD_SRC_KEY)
				continue;
			
			if(EmulPrefs.pads[pp].btn_val[bb] != event.GetKeyCode())
				continue;
			
			bound++;
			if(event.GetEventType() == wxEVT_KEY_DOWN)
				EmulPadState[pp] |= (1u << bb);
			else
				EmulPadState[pp] &= ~(1u << bb);
		}
	}
	
	if(!bound)
		event.Skip();
}

class EmulFrame : public wxFrame
{
public:
	EmulFrame();

private:
	void ResetSim(void);

	void OnRestart(wxCommandEvent &event);
	void OnExit(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnOpenImage(wxCommandEvent &event);
	void OnOpenDevice(wxCommandEvent &event);
	void OnOpenMenu(wxCommandEvent &event);
	void OnPreferences(wxCommandEvent &event);

};

EmulFrame::EmulFrame()
: wxFrame(NULL, wxID_ANY, "Neki32 Simulator")
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(wxID_CDROM, "Open &Card\tCtrl-C", "Open a real game card");
	menuFile->Append(wxID_OPEN, "Open &Image\tCtrl-I", "Open a disk-image of a game card");
	menuFile->Append(wxID_EXECUTE, "Open &Menu\tCtrl-M", "Run the simulation with no game in");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_REFRESH, "&Restart\tCtrl-R", "Restart the current game");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu *menuEdit = new wxMenu;
	menuEdit->Append(wxID_PREFERENCES, "&Preferences\tCtrl-P", "Setup controls and audiovisual options");
	
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuEdit, "&Edit");
	menuBar->Append(menuHelp, "&Help");
	
	SetMenuBar(menuBar);
	
	CreateStatusBar();
	SetStatusText("Neki32 Simulator - No Image Loaded");
	
	Bind(wxEVT_MENU, &EmulFrame::OnExit, this, wxID_EXIT);
	Bind(wxEVT_MENU, &EmulFrame::OnRestart, this, wxID_REFRESH);
	Bind(wxEVT_MENU, &EmulFrame::OnAbout, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &EmulFrame::OnOpenImage, this, wxID_OPEN);
	Bind(wxEVT_MENU, &EmulFrame::OnOpenDevice, this, wxID_CDROM);
	Bind(wxEVT_MENU, &EmulFrame::OnOpenMenu, this, wxID_EXECUTE);
	Bind(wxEVT_MENU, &EmulFrame::OnPreferences, this, wxID_PREFERENCES);
	
	wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
	EmulScreenPanel *screen = new EmulScreenPanel(this);
	sizer->AddStretchSpacer(1);
	sizer->Add(screen, 0, wxALL | wxALIGN_CENTER, 8);
	sizer->AddStretchSpacer(1);
	SetSizerAndFit(sizer);
	SetAutoLayout(true);
	
	EmulTimer *timer = new EmulTimer(screen);
	(void)timer;
	
	ResetSim();
}

void EmulFrame::OnRestart(wxCommandEvent &event)
{
	(void)event;
	ResetSim();
	
	SetStatusText("Restarted simulation.");
}

void EmulFrame::OnExit(wxCommandEvent &event)
{
	(void)event;
	Close(true);
}

void EmulFrame::OnAbout(wxCommandEvent &event)
{
	(void)event;
	wxMessageBox(
		"Neki32 Game Console Simulator\n"
		"\n"
		"This program performs instruction-level simulation of\n"
		"a Neki32 program in userland. It intercepts syscalls\n"
		"and executes them using devices from the host system.\n"
		"\n"
		"Copyright 2025 Nekisoft Pty Ltd (ACN 680 583 251)\n"
		"http://www.nekisoft.com.au",
		"About",
		wxOK | wxICON_INFORMATION
	);
}

void EmulFrame::ResetSim()
{
	EmulTimer::RunMode = false;
	EmulTimerTicks = 0;
	EmulVsyncs = 0;
	
	sysc_setdiskfd(EmulDisk);
	process_reset();
	
	EmulTimer::RunMode = true;	
}

void EmulFrame::OnOpenImage(wxCommandEvent &event)
{
	//Show file picker dialog to find .iso images
	wxFileDialog dlg(
		this,
		_("Open Disk Image"),
		"", 
		"",
		"ISO disk images (*.iso)|*.iso",
		wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	
	if(dlg.ShowModal() == wxID_CANCEL)
		return; //User canceled
	
	int newfd = open(dlg.GetPath().c_str(), O_RDONLY);
	if(newfd < 0)
	{
		//Failed to open the given file
		wxMessageBox(
			wxString::Format("Cannot open the given file (%s): %s\n", dlg.GetPath(), strerror(errno)),
			_("Failed to open"), wxICON_ERROR | wxOK, this);
		return;
	}
	
	//Close the existing disk fd if open
	if(EmulDisk >= 0)
	{
		close(EmulDisk);
		EmulDisk = -1;
	}
	
	//Use the new FD as our disk image
	EmulDisk = newfd;
	
	//Update status bar
	char stbuf[1024] = {0};
	snprintf(stbuf, sizeof(stbuf)-1, "Opened %s", (const char*)(dlg.GetPath().c_str()));
	SetStatusText(stbuf);
	
	//Reset simulation with the new game inserted
	(void)event;
	ResetSim();
}

void EmulFrame::OnOpenDevice(wxCommandEvent &event)
{
	(void)event;
	ResetSim();
}

void EmulFrame::OnOpenMenu(wxCommandEvent &event)
{
	(void)event;
	
	//Launch with no disk image loaded
	if(EmulDisk >= 0)
	{
		close(EmulDisk);
		EmulDisk = -1;
	}
	
	ResetSim();
}

void EmulFrame::OnPreferences(wxCommandEvent &event)
{
	(void)event;
	
	CfgWin *w = new CfgWin(this, EmulPrefs);
	if(w->ShowModal() == wxID_OK)
	{
		//Take new prefs
		EmulPrefs = w->MutablePrefs;
		prefs_write(&EmulPrefs);
		
		//Reset pad state
		memset(EmulPadState, 0, sizeof(EmulPadState));
	}
	else
	{
		//Discard changed prefs
	}
	w->Destroy();
}


class EmulApp : public wxApp
{
public:
	virtual bool OnInit();
};

bool EmulApp::OnInit()
{
	SetAppName("nemul");
	
	prefs_read(&EmulPrefs);
	process_reset();
	rsp_init(&EmulPrefs);
	
	EmulFrame *frame = new EmulFrame();
	frame->Show(true);
	return true;
}

wxIMPLEMENT_APP(EmulApp);
