//cdfs.h
//Access to ISO9660 filesystem on card
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef CDFS_H
#define CDFS_H

#include <sc.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

//Looks for and initializes CD filesystem at beginning of block device
void _cdfs_init(void);

//Returns the size of the CD filesystem in bytes - the first free byte-offset of the block device afterwards.
uint64_t _cdfs_fslen(void);

//Returns the inode number of the CD's root directory
uint32_t _cdfs_rootino(void);

//Returns status information about an inode
int _cdfs_stat(uint32_t ino, struct stat *buf);

//Returns status information about the CD filesystem
int _cdfs_statvfs(struct statvfs *buf);

//Reads from an inode
int _cdfs_read(uint32_t ino, uint32_t off, void *buf, int len);

//Searches a directory inode for a link with the given name, returning its referenced inode
int _cdfs_search(uint32_t ino, const char *filename);

#endif //CDFS_H
