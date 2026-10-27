//prefs.cpp
//Nonvolatile user preferences in Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025
//
//Plain text file, no toolkit behind it. The wxWidgets front end kept these settings in a
//wxConfig (registry on Windows, a dotfile on Unix), which was the only reason the emulator
//needed wxWidgets at all. This one writes the same keys to an INI-style file, so the core and
//the front end have no library in common beyond the C++ runtime.
//
//  PadA/Bit0/Src=1            source of the button: 0 none, 1 key, 2 joystick
//  PadA/Bit0/Val=1073741904   SDL keycode when Src is 1
//  Rsp/Enabled=1
//  Rsp/Port=14292
//
//Values are SDL keycodes (SDLK_LEFT and friends), not wx keycodes, so a binding saved by a wx
//build does not carry over; the file is in a different place and a different format anyway.

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#include "prefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <SDL.h>

#ifdef _WIN32
	#include <direct.h>
	#define prefs_mkdir(x) _mkdir(x)
#else
	#define prefs_mkdir(x) mkdir(x, 0777)
#endif

//Where the configuration lives, overridable with NEMUL_PREFS
static void prefs_path(char *out, size_t outsz)
{
	const char *over = getenv("NEMUL_PREFS");
	if(over != NULL && over[0] != '\0')
	{
		snprintf(out, outsz, "%s", over);
		return;
	}

#ifdef _WIN32
	const char *appdata = getenv("APPDATA");
	if(appdata != NULL && appdata[0] != '\0')
		snprintf(out, outsz, "%s\\nemul.ini", appdata);
	else
		snprintf(out, outsz, "nemul.ini");
#else
	const char *xdg = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");
	if(xdg != NULL && xdg[0] != '\0')
		snprintf(out, outsz, "%s/nemul/nemul.ini", xdg);
	else if(home != NULL && home[0] != '\0')
		snprintf(out, outsz, "%s/.config/nemul/nemul.ini", home);
	else
		snprintf(out, outsz, "nemul.ini");
#endif
}

//Makes every directory a path needs, so a first save works without the user making them
static void prefs_mkdirs(const char *path)
{
	char scratch[4096];
	snprintf(scratch, sizeof(scratch), "%s", path);
	for(char *cc = scratch + 1; *cc != '\0'; cc++)
	{
		if(*cc != '/' && *cc != '\\')
			continue;
		char sep = *cc;
		*cc = '\0';
		if(scratch[0] != '\0')
			prefs_mkdir(scratch);
		*cc = sep;
	}
}

//Trims whitespace in place, returns the new start
static char *prefs_trim(char *str)
{
	while(*str != '\0' && isspace((unsigned char)*str))
		str++;
	char *end = str + strlen(str);
	while(end > str && isspace((unsigned char)end[-1]))
		end--;
	*end = '\0';
	return str;
}

//Reads one "Key=Value" line into a number. Returns 1 if the key matched.
//The line is chopped at the '=', so callers that look for a second key pass a copy.
static int prefs_parse_int(char *line, const char *key, int *out)
{
	char *eq = strchr(line, '=');
	if(eq == NULL)
		return 0;
	*eq = '\0';
	if(strcmp(prefs_trim(line), key) != 0)
		return 0;
	*out = (int)strtol(prefs_trim(eq + 1), NULL, 0);
	return 1;
}

//Default bindings, so a first run is playable without a trip to the preferences window.
//A and B are Z and X as on every keyboard-emulated console; the top row follows that.
void prefs_defaults(prefs_t *out)
{
	memset(out, 0, sizeof(*out));

	static const int defkeys[PREFS_PAD_BTN_MAX] =
	{
		(int)SDLK_UP,
		(int)SDLK_LEFT,
		(int)SDLK_DOWN,
		(int)SDLK_RIGHT,
		(int)SDLK_z,
		(int)SDLK_x,
		(int)SDLK_c,
		(int)SDLK_a,
		(int)SDLK_s,
		(int)SDLK_d,
		(int)SDLK_RETURN,
		(int)SDLK_TAB,
	};

	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		out->pads[pp].js_id = -1;
		for(int bb = 0; bb < PREFS_PAD_BTN_MAX; bb++)
		{
			//Only pad A gets bindings by default: four pads on one keyboard would collide
			if(pp == 0)
			{
				out->pads[pp].btn_src[bb] = PREFS_PAD_SRC_KEY;
				out->pads[pp].btn_val[bb] = defkeys[bb];
			}
			else
			{
				out->pads[pp].btn_src[bb] = PREFS_PAD_SRC_NONE;
				out->pads[pp].btn_val[bb] = 0;
			}
		}
	}

	out->rsp_enabled = true;
	out->rsp_port = 14292;
}

//Reads configuration or initializes defaults
void prefs_read(prefs_t *out)
{
	prefs_defaults(out);

	char path[4096];
	prefs_path(path, sizeof(path));

	FILE *in = fopen(path, "r");
	if(in == NULL)
		return; //First run: the defaults stand until something is saved

	char line[1024];
	while(fgets(line, sizeof(line), in) != NULL)
	{
		//Strip comments and line endings before the key/value split
		char *hash = strchr(line, '#');
		if(hash != NULL)
			*hash = '\0';
		char *nl = strpbrk(line, "\r\n");
		if(nl != NULL)
			*nl = '\0';
		if(prefs_trim(line)[0] == '\0')
			continue;

		int val = 0;
		int matched = 0;
		char key[64];
		char copy[1024];

		for(int pp = 0; pp < PREFS_PAD_MAX && !matched; pp++)
		{
			for(int bb = 0; bb < PREFS_PAD_BTN_MAX && !matched; bb++)
			{
				snprintf(copy, sizeof(copy), "%s", line);
				snprintf(key, sizeof(key), "Pad%c/Bit%d/Src", 'A' + pp, bb);
				if(prefs_parse_int(copy, key, &val))
				{
					out->pads[pp].btn_src[bb] = (prefs_pad_src_t)val;
					matched = 1;
					break;
				}

				snprintf(copy, sizeof(copy), "%s", line);
				snprintf(key, sizeof(key), "Pad%c/Bit%d/Val", 'A' + pp, bb);
				if(prefs_parse_int(copy, key, &val))
				{
					out->pads[pp].btn_val[bb] = val;
					matched = 1;
					break;
				}
			}
		}

		if(matched)
			continue;

		//Separate copies: prefs_parse_int() chops the line at the '='
		snprintf(copy, sizeof(copy), "%s", line);
		if(prefs_parse_int(copy, "Rsp/Enabled", &val))
		{
			out->rsp_enabled = (val != 0);
			continue;
		}

		snprintf(copy, sizeof(copy), "%s", line);
		if(prefs_parse_int(copy, "Rsp/Port", &val))
			out->rsp_port = val;
	}

	fclose(in);
}

//Writes configuration
void prefs_write(const prefs_t *in)
{
	char path[4096];
	prefs_path(path, sizeof(path));
	prefs_mkdirs(path);

	//Write beside the target and rename over it, so a crash or a full disk cannot leave a
	//half-written configuration behind with the user's bindings gone
	char temp[4096];
	snprintf(temp, sizeof(temp), "%s.tmp", path);

	FILE *out = fopen(temp, "w");
	if(out == NULL)
		return;

	fprintf(out, "#nemul configuration\n");
	fprintf(out, "#Src: 0 none, 1 key, 2 joystick. Val: SDL keycode.\n");
	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		for(int cc = 0; cc < PREFS_PAD_BTN_MAX; cc++)
		{
			fprintf(out, "Pad%c/Bit%d/Src=%d\n", 'A' + pp, cc, (int)in->pads[pp].btn_src[cc]);
			fprintf(out, "Pad%c/Bit%d/Val=%d\n", 'A' + pp, cc, in->pads[pp].btn_val[cc]);
		}
	}

	fprintf(out, "Rsp/Enabled=%d\n", in->rsp_enabled ? 1 : 0);
	fprintf(out, "Rsp/Port=%d\n", in->rsp_port);

	fflush(out);
	fclose(out);

	remove(path);
	if(rename(temp, path) != 0)
		remove(temp);
}
