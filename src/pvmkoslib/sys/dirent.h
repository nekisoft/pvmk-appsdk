//sys/dirent.h
//Directory entry structure for using picolibc.
//Bryan E. Topp <betopp@betopp.com> 2020
#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

//Opaque type for directory-stream objects
typedef struct _DIR_s DIR;

struct dirent
{
	ino_t d_ino; //File serial number
	mode_t d_type; //Mode with permission bits masked out (for Linux compatibility)
	char d_name[]; //Filename string of entry
};

//For Linux compatibility with d_type field
#define DT_DIR S_IFDIR
#define DT_REG S_IFREG
#define DT_LNK S_IFLNK

#endif //_SYS_DIRENT_H
