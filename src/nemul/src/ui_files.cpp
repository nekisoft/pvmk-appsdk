//ui_files.cpp
//Disk-image picker for the Neki32 simulator front end
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#include "ui_files.h"
#include "imgui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <string>
#include <vector>
#include <algorithm>

//One line of the listing
typedef struct ui_files_entry_s
{
	std::string name;	//Name only, no directory
	std::string full;	//Full path
	bool is_dir;
	long long size;		//Bytes, for files
} ui_files_entry_t;

//Listing cache, so the directory is read once per visit and not once per frame
static std::string ui_files_cache_dir;
static std::vector<ui_files_entry_t> ui_files_cache;

//Case-insensitive ordering, so a directory of lowercase and uppercase names stays readable
static int ui_files_compare(const ui_files_entry_t &a, const ui_files_entry_t &b)
{
	size_t nn = a.name.size() < b.name.size() ? a.name.size() : b.name.size();
	for(size_t ii = 0; ii < nn; ii++)
	{
		int ca = tolower((unsigned char)a.name[ii]);
		int cb = tolower((unsigned char)b.name[ii]);
		if(ca != cb)
			return ca - cb;
	}
	if(a.name.size() != b.name.size())
		return a.name.size() < b.name.size() ? -1 : 1;
	return 0;
}

//Directories first, then files, each alphabetical, with ".." pinned at the top
static bool ui_files_sort(const ui_files_entry_t &a, const ui_files_entry_t &b)
{
	if(a.name == "..")
		return true;
	if(b.name == "..")
		return false;
	if(a.is_dir != b.is_dir)
		return a.is_dir;
	return ui_files_compare(a, b) < 0;
}

//Whether a name ends in .iso, the only thing the browser offers
static bool ui_files_is_iso(const char *name)
{
	size_t len = strlen(name);
	if(len < 4)
		return false;
	const char *ext = name + len - 4;
	return tolower((unsigned char)ext[0]) == '.' &&
		tolower((unsigned char)ext[1]) == 'i' &&
		tolower((unsigned char)ext[2]) == 's' &&
		tolower((unsigned char)ext[3]) == 'o';
}

//Drops the last component of a path, leaving "/" when there is nothing left
static void ui_files_parent(const char *dir, char *out, size_t outsz)
{
	snprintf(out, outsz, "%s", dir);
	char *slash = strrchr(out, '/');
	if(slash == NULL)
	{
		//A bare relative name like ".." needs .. to get above it
		snprintf(out, outsz, "..");
		return;
	}
	if(slash == out)
	{
		snprintf(out, outsz, "/");
		return;
	}
	*slash = '\0';
}

//Reads a directory into the cache, and moves the browser there
static void ui_files_refresh(ui_files_t *st, const char *dir)
{
	ui_files_cache.clear();
	ui_files_cache_dir = dir;

	DIR *dd = opendir(dir);
	if(dd == NULL)
	{
		snprintf(st->error, sizeof(st->error), "cannot read %s", dir);
		return;
	}

	st->error[0] = '\0';

	//Parent entry, unless we are at the root
	if(strcmp(dir, "/") != 0)
	{
		ui_files_entry_t up;
		up.name = "..";
		char parent[2048];
		ui_files_parent(dir, parent, sizeof(parent));
		up.full = parent;
		up.is_dir = true;
		up.size = 0;
		ui_files_cache.push_back(up);
	}

	struct dirent *de = NULL;
	while((de = readdir(dd)) != NULL)
	{
		if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;

		ui_files_entry_t ent;
		ent.name = de->d_name;
		size_t dlen = strlen(dir);
		if(dlen > 0 && dir[dlen - 1] == '/')
			ent.full = std::string(dir) + ent.name;
		else
			ent.full = std::string(dir) + "/" + ent.name;
		ent.is_dir = false;
		ent.size = 0;

		struct stat sb;
		if(stat(ent.full.c_str(), &sb) == 0)
		{
			ent.is_dir = S_ISDIR(sb.st_mode) ? true : false;
			ent.size = (long long)sb.st_size;
		}

		//Only directories and disk images, so the list is the thing being chosen from
		if(!ent.is_dir && st->filter_iso && !ui_files_is_iso(ent.name.c_str()))
			continue;

		ui_files_cache.push_back(ent);
	}

	closedir(dd);

	std::sort(ui_files_cache.begin(), ui_files_cache.end(), ui_files_sort);
}

//Opens the browser at the given path (a directory, or a file whose directory is used)
void ui_files_open(ui_files_t *st, const char *startpath, bool filter_iso)
{
	st->open = true;
	st->done = false;
	st->filter_iso = filter_iso;
	st->selected = 0;
	st->scroll_to = -1;
	st->error[0] = '\0';
	st->result[0] = '\0';

	//Start where the caller said, if it is a directory; otherwise in its directory
	snprintf(st->dir, sizeof(st->dir), "%s", (startpath != NULL && startpath[0] != '\0') ? startpath : ".");

	struct stat sb;
	if(stat(st->dir, &sb) != 0 || !S_ISDIR(sb.st_mode))
	{
		char parent[2048];
		ui_files_parent(st->dir, parent, sizeof(parent));
		snprintf(st->dir, sizeof(st->dir), "%s", parent);
	}

	//Absolute, so ".." always has somewhere to go and the path field shows where we really are
	char resolved[2048];
#ifdef _WIN32
	if(_fullpath(resolved, st->dir, sizeof(resolved)) != NULL)
		snprintf(st->dir, sizeof(st->dir), "%s", resolved);
#else
	if(realpath(st->dir, resolved) != NULL)
		snprintf(st->dir, sizeof(st->dir), "%s", resolved);
#endif

	ui_files_refresh(st, st->dir);
}

//Chooses the given path and closes the browser
static void ui_files_choose(ui_files_t *st, const char *path)
{
	snprintf(st->result, sizeof(st->result), "%s", path);
	st->done = true;
	st->open = false;
}

//Draws the browser. On the frame the user picks a file, sets done and puts the path in result.
void ui_files_draw(ui_files_t *st)
{
	if(!st->open)
		return;

	ImGui::SetNextWindowSize(ImVec2(760, 500), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	if(!ImGui::Begin("Open Disk Image", &st->open))
	{
		ImGui::End();
		return;
	}

	//Where we are, editable so a deep game directory can be typed instead of clicked to
	static char typed[2048] = {0};
	static std::string typed_for;
	if(typed_for != st->dir)
	{
		snprintf(typed, sizeof(typed), "%s", st->dir);
		typed_for = st->dir;
	}

	ImGui::SetNextItemWidth(-100.0f);
	bool entered = ImGui::InputText("##path", typed, sizeof(typed), ImGuiInputTextFlags_EnterReturnsTrue);
	bool go_typed = entered || ImGui::IsItemDeactivatedAfterEdit();
	ImGui::SameLine();

	//Up one level
	if(ImGui::Button("Up") && strcmp(st->dir, "/") != 0)
	{
		char parent[2048];
		ui_files_parent(st->dir, parent, sizeof(parent));
		snprintf(st->dir, sizeof(st->dir), "%s", parent);
		ui_files_refresh(st, st->dir);
		typed_for.clear();
		st->selected = 0;
	}

	//A typed path is taken on Enter, or when the field loses focus
	if(go_typed && strcmp(typed, st->dir) != 0)
	{
		struct stat sb;
		if(stat(typed, &sb) == 0 && S_ISDIR(sb.st_mode))
		{
			snprintf(st->dir, sizeof(st->dir), "%s", typed);
			ui_files_refresh(st, st->dir);
			typed_for.clear();
			st->selected = 0;
		}
		else if(stat(typed, &sb) == 0)
		{
			ui_files_choose(st, typed);
			ImGui::End();
			return;
		}
		else
		{
			snprintf(st->error, sizeof(st->error), "no such directory or file: %s", typed);
		}
	}

	if(st->error[0] != '\0')
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", st->error);
	else if(st->filter_iso)
		ImGui::TextDisabled("Showing directories and *.iso disk images");
	else
		ImGui::TextDisabled("Showing everything");

	ImGui::Separator();

	//The listing itself
	float footer = ImGui::GetFrameHeightWithSpacing() * 2.0f;
	ImGui::BeginChild("##list", ImVec2(0.0f, -footer), ImGuiChildFlags_Borders);

	for(int ii = 0; ii < (int)ui_files_cache.size(); ii++)
	{
		const ui_files_entry_t &ent = ui_files_cache[ii];

		char label[512];
		if(ent.is_dir)
			snprintf(label, sizeof(label), "[dir]  %s", ent.name.c_str());
		else if(ent.size >= (1024 * 1024))
			snprintf(label, sizeof(label), "%7lld.%lldM  %s", ent.size / (1024 * 1024), ((ent.size / (1024 * 100)) % 10), ent.name.c_str());
		else
			snprintf(label, sizeof(label), "%7lldK  %s", ent.size / 1024, ent.name.c_str());

		if(ImGui::Selectable(label, st->selected == ii, ImGuiSelectableFlags_AllowDoubleClick))
		{
			st->selected = ii;
			if(ImGui::IsMouseDoubleClicked(0))
			{
				if(ent.is_dir)
				{
					snprintf(st->dir, sizeof(st->dir), "%s", ent.full.c_str());
					ui_files_refresh(st, st->dir);
					typed_for.clear();
					st->selected = 0;
					break;
				}
				else
				{
					ui_files_choose(st, ent.full.c_str());
					ImGui::EndChild();
					ImGui::End();
					return;
				}
			}
		}
	}

	if(st->scroll_to >= 0 && st->scroll_to < (int)ui_files_cache.size())
	{
		ImGui::SetScrollHereY();
		st->scroll_to = -1;
	}

	ImGui::EndChild();

	//Footer: what is selected, and the way out
	const ui_files_entry_t *sel = NULL;
	if(st->selected >= 0 && st->selected < (int)ui_files_cache.size())
		sel = &ui_files_cache[st->selected];

	ImGui::TextUnformatted(sel != NULL ? sel->full.c_str() : "(nothing selected)");

	bool can_open = (sel != NULL && !sel->is_dir);

	ImGui::BeginDisabled(!can_open);
	if(ImGui::Button("Open", ImVec2(120, 0)))
		ui_files_choose(st, sel->full.c_str());
	ImGui::EndDisabled();

	ImGui::SameLine();
	if(ImGui::Button("Cancel", ImVec2(120, 0)))
		st->open = false;

	//Enter and Escape, so the dialog behaves like the one it replaces
	if(can_open && ImGui::IsKeyPressed(ImGuiKey_Enter, false) && !ImGui::GetIO().WantTextInput)
		ui_files_choose(st, sel->full.c_str());
	if(ImGui::IsKeyPressed(ImGuiKey_Escape, false))
		st->open = false;

	ImGui::End();
}
