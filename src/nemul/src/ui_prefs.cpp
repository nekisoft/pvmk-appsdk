//ui_prefs.cpp
//Preferences window for the Neki32 simulator front end
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#include "ui_prefs.h"
#include "imgui.h"

#include <stdio.h>
#include <string.h>
#include <SDL.h>

//Names of the buttons, in the bit order the hardware reports them
static const char *ui_prefs_btnnames[PREFS_PAD_BTN_MAX] =
{
	"Up", "Left", "Down", "Right", "A", "B", "C", "X", "Y", "Z", "Start", "Mode"
};

//What a binding is currently set to, as the user sees it
static const char *ui_prefs_value(int src, int val)
{
	static char scratch[64];

	if(src == PREFS_PAD_SRC_KEY && val != 0)
	{
		//SDL's own name, so the keyboard layout the user actually has is what is shown
		const char *name = SDL_GetKeyName((SDL_Keycode)val);
		if(name != NULL && name[0] != '\0')
		{
			snprintf(scratch, sizeof(scratch), "%s", name);
			return scratch;
		}
	}

	if(src == PREFS_PAD_SRC_JOY)
		return "(joystick)";

	return "Unset";
}

//Opens the window on a copy of the current preferences
void ui_prefs_open(ui_prefs_t *st, const prefs_t *current)
{
	st->open = true;
	st->apply = false;
	st->working = *current;
	st->bind_pad = -1;
	st->bind_btn = -1;
	st->saved_frames = 0;
}

//Offers a keypress to a pending binding. Returns true when it was taken, so the caller does
//not also treat it as pad input or a shortcut.
bool ui_prefs_take_key(ui_prefs_t *st, int keycode)
{
	if(!st->open || st->bind_pad < 0 || st->bind_btn < 0)
		return false;

	//Escape and Backspace cancel the binding rather than becoming one, so a user who opened
	//the prompt by mistake can get out without a mouse
	if(keycode == SDLK_ESCAPE)
	{
		st->bind_pad = -1;
		st->bind_btn = -1;
		return true;
	}

	if(keycode > 0)
	{
		st->working.pads[st->bind_pad].btn_src[st->bind_btn] = PREFS_PAD_SRC_KEY;
		st->working.pads[st->bind_pad].btn_val[st->bind_btn] = keycode;
	}

	st->bind_pad = -1;
	st->bind_btn = -1;
	return true;
}

//Draws the window
void ui_prefs_draw(ui_prefs_t *st)
{
	if(!st->open)
		return;

	ImGui::SetNextWindowSize(ImVec2(470, 540), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	if(!ImGui::Begin("Preferences", &st->open))
	{
		ImGui::End();
		return;
	}

	if(ImGui::BeginTabBar("##pads"))
	{
		for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
		{
			char tab[16];
			snprintf(tab, sizeof(tab), "Pad %c", 'A' + pp);

			if(!ImGui::BeginTabItem(tab))
				continue;

			ImGui::TextDisabled("Press a button, then a key for it.");
			ImGui::Spacing();

			if(ImGui::BeginTable("##bindings", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
			{
				for(int bb = 0; bb < PREFS_PAD_BTN_MAX; bb++)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(ui_prefs_btnnames[bb]);

					ImGui::TableNextColumn();
					if(st->bind_pad == pp && st->bind_btn == bb)
						ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Press a key...");
					else
						ImGui::TextUnformatted(ui_prefs_value(
							(int)st->working.pads[pp].btn_src[bb],
							st->working.pads[pp].btn_val[bb]));

					ImGui::TableNextColumn();
					char id[32];
					snprintf(id, sizeof(id), "Key##%d_%d", pp, bb);
					if(ImGui::Button(id))
					{
						st->bind_pad = pp;
						st->bind_btn = bb;
					}
					ImGui::SameLine();
					snprintf(id, sizeof(id), "X##%d_%d", pp, bb);
					if(ImGui::Button(id))
					{
						st->working.pads[pp].btn_src[bb] = PREFS_PAD_SRC_NONE;
						st->working.pads[pp].btn_val[bb] = 0;
						if(st->bind_pad == pp && st->bind_btn == bb)
						{
							st->bind_pad = -1;
							st->bind_btn = -1;
						}
					}
				}

				ImGui::EndTable();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::Separator();

	if(ImGui::Button("OK", ImVec2(110, 0)))
	{
		st->apply = true;
		st->open = false;
		st->bind_pad = -1;
		st->bind_btn = -1;
	}

	ImGui::SameLine();
	if(ImGui::Button("Cancel", ImVec2(110, 0)))
	{
		st->open = false;
		st->bind_pad = -1;
		st->bind_btn = -1;
	}

	ImGui::SameLine();
	if(ImGui::Button("Reset All", ImVec2(110, 0)))
	{
		prefs_defaults(&st->working);
		st->bind_pad = -1;
		st->bind_btn = -1;
	}

	if(st->saved_frames > 0)
	{
		st->saved_frames--;
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Saved.");
	}

	ImGui::End();
}
