//sys/statvfs.h
//Filesystem usage declarations for using picolibc.
//Bryan E. Topp <betopp@betopp.com> 2023
#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#include <sys/types.h>

struct statvfs
{
	unsigned long f_bsize; //Block size
	unsigned long f_frsize; //Block size (?)
	fsblkcnt_t f_blocks; //Total size of filesystem, units of f_frsize bytes
	fsblkcnt_t f_bfree; //Free blocks in filesystem
	fsblkcnt_t f_bavail; //Free blocks usable to user programs
	fsfilcnt_t f_files; //Total number of inodes
	fsfilcnt_t f_ffree; //Free inodes
	fsfilcnt_t f_favail; //Free inodes usable to user programs
	unsigned long f_fsid; //ID of filesystem
	unsigned long f_flag; //ST_RDONLY and/or ST_NOSUID
	unsigned long f_namemax; //Max length of filename
	
	//For Linux compatibility
	char f_mntonname[16];
	char f_mntfromname[16];
	char f_fstypename[16];
	unsigned int f_type;
};

//f_flag values like BSD
#define ST_RDONLY 1
#define ST_NOSUID 2

int statvfs(const char * path, struct statvfs * buf);
int fstatvfs(int fd, struct statvfs *buf);

#endif // _SYS_STATVFS_H
