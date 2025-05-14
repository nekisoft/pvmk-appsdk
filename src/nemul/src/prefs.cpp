//prefs.cpp
//Data about user preferences in Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#include "prefs.h"
#include <wx/config.h>

#include <stdio.h>
#include <string.h>


//Reads configuration or initializes defaults
void prefs_read(prefs_t *out)
{
	//Set defaults
	memset(out, 0, sizeof(*out));
	out->rsp_port = 14292;
	out->rsp_enabled = 1;
	
	//Load pad input bindings
	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		for(int cc = 0; cc < PREFS_PAD_BTN_MAX; cc++)
		{
			char key[64] = {0};
			
			snprintf(key, sizeof(key)-1, "/Pad%c/Bit%d/Src", pp + 'A', cc);
			int src = 0;
			wxConfigBase::Get()->Read(key, &src);
			out->pads[pp].btn_src[cc] = (prefs_pad_src_t)src;
			
			snprintf(key, sizeof(key)-1, "/Pad%c/Bit%d/Val", pp + 'A', cc);
			int val = 0;
			wxConfigBase::Get()->Read(key, &val);
			out->pads[pp].btn_val[cc] = val;
		}
	}

	//Load RSP configuration
	wxConfigBase::Get()->Read("/Rsp/Enabled", &(out->rsp_enabled));
	wxConfigBase::Get()->Read("/Rsp/Port", &(out->rsp_port));
}

//Writes configuration
void prefs_write(const prefs_t *in)
{
	//Write pad input bindings
	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		for(int cc = 0; cc < PREFS_PAD_BTN_MAX; cc++)
		{
			char key[64] = {0};
			
			snprintf(key, sizeof(key)-1, "/Pad%c/Bit%d/Src", pp + 'A', cc);
			int src = in->pads[pp].btn_src[cc];
			wxConfigBase::Get()->Write(key, src);
			
			snprintf(key, sizeof(key)-1, "/Pad%c/Bit%d/Val", pp + 'A', cc);
			int val = in->pads[pp].btn_val[cc];
			wxConfigBase::Get()->Write(key, val);
		}
	}
	
	//Write RSP configuration
	wxConfigBase::Get()->Write("/Rsp/Enabled", in->rsp_enabled);
	wxConfigBase::Get()->Write("/Rsp/Port", in->rsp_port);

	//Make sure it gets out to disk
	wxConfigBase::Get()->Flush();
}