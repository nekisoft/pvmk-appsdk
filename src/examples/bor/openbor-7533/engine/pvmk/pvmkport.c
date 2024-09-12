/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <stdlib.h>
#include <string.h>
#include "pvmkport.h"
#include "../source/gamelib/packfile.h"
#include "video.h"
#include "control.h"
#include "../source/utils.h"
#include "../source/ramlib/ram.h"
#include "menu.h"

extern void __exception_setreload(int t);

char packfile[MAX_FILENAME_LEN];
char paksDir[MAX_FILENAME_LEN];
char savesDir[MAX_FILENAME_LEN];
char logsDir[MAX_FILENAME_LEN];
char screenShotsDir[MAX_FILENAME_LEN];
char rootDir[MAX_FILENAME_LEN]; // note: this one ends with a slash

/*
 * Given a file's path relative to the OpenBOR executable;
 */
char* getFullPath(char *relPath)
{
	static char filename[MAX_FILENAME_LEN];
	strcpy(filename, rootDir);
	strcat(filename, relPath);
	return filename;
}

void borExit(int reset)
{
    exit(reset);
}

// TODO merge into getFullPath
void initDirPath(char* dest, char* relPath)
{
	strcpy(dest, rootDir);
	strcat(dest, relPath);
}

int main(int argc, char * argv[])
{
	// Launch a pak directly from a loader with plugin ability(Wiiflow, Postloader etc).
	// With WiiFlow, the first definable plugin's argument in openbor.ini is argv[1].
	//int directlaunch = (argc > 1 && (argv[1][0] == 'u' || argv[1][0] == 's')) ? 1 : 0;
	
	video_init();

	setSystemRam();
	packfile_mode(0);
	
	//new system to get base directory on usb or sd.
	char root[MAX_FILENAME_LEN];
	memset(root, '\0', sizeof(root));
	
	strcpy(root, "/");
	
	// Root path sent by the loader's argument(apps/OpenBOR by default)
	//if(directlaunch)
	//{
	//	strncpy(root, argv[1], strrchr(argv[1], '/') - argv[1]);
	//}
	//else
	//{
	//	strncpy(root, argv[0], strrchr(argv[0], '/') - argv[0]);
	//}

	strcpy(rootDir, "/");
	strcpy(savesDir, "/");
	strcpy(paksDir, "/paks");
	strcpy(logsDir, "/logs");
	strcpy(screenShotsDir, "/screenshots");
		

	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);
	
	// Pack's name sent by the loader's argument
	strcpy(packfile, "/paks/bor.pak");
	
	//if(directlaunch)
	//{
	//	getBasePath(packfile, argv[2], 1);
	//}

	//Menu();
	openborMain(argc, argv);
	borExit(0);
	return 0;
}

