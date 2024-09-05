//cdfs.h
//Access to ISO9660 filesystem on card
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef CDFS_H
#define CDFS_H

#include "kstd.h"
#include "sc.h"

//Looks for and initializes CD filesystem at beginning of block device
void cdfs_init(void);

//Returns the size of the CD filesystem in bytes - the first free byte-offset of the block device afterwards.
uint64_t cdfs_fslen(void);

//Returns the inode number of the CD's root directory
uint32_t cdfs_rootino(void);

//Returns status information about an inode
int cdfs_stat(uint32_t ino, _sc_stat_t *buf);

//Returns status information about the CD filesystem
int cdfs_statvfs(_sc_statvfs_t *buf);

//Reads from an inode
int cdfs_read(uint32_t ino, uint32_t off, void *buf, int len);

//Searches a directory inode for a link with the given name, returning its referenced inode
int cdfs_search(uint32_t ino, const char *filename);

#endif //CDFS_H
