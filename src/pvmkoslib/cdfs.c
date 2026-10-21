//cdfs.c
//Access to ISO9660 filesystem on card
//Bryan E. Topp <betopp@betopp.com> 2024

#include "cdfs.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include <sys/stat.h>
#include <sys/dirent.h>

//Whether we found a working ISO9660 filesystem.
static bool _cdfs_present;

//Date-time value used in ISO9660
typedef struct _cdfs_datetime_s
{
	char whatever[17]; //Don't care
} _cdfs_datetime_t;

//ISO9660 directory entry (may be shorter than this)
typedef struct _cdfs_dirent_s
{
	uint8_t length;      //Length of directory entry
	uint8_t ext_length;  //Length of extended attribute record
	uint32_t lba;         //Sector number of data
	uint32_t lba_msb;
	uint32_t len;         //Length of data, bytes
	uint32_t len_msb;
	char    date[7];
	uint8_t flags;
	char    interleave_unit;
	char    interleave_gap;
	uint16_t volume_sequence;
	uint16_t volume_sequence_msb;
	uint8_t filename_len;
	char    filename_and_system_use[223]; //Can actually only be 222 bytes long, limited by length=255
} __attribute__((packed)) _cdfs_dirent_t;

//Structure of a ISO9660 primary volume descriptor
typedef struct _cdfs_pvd_s
{
	char magic[8];
	char system_identifier[32];
	char volume_identifier[32];
	char unused72[8];
	uint32_t volume_space_size;
	uint32_t volume_space_size_msb;
	char unused88[32];
	uint16_t volume_set_size;
	uint16_t volume_set_size_msb;
	uint16_t volume_sequence_number;
	uint16_t volume_sequence_number_msb;
	uint16_t logical_block_size;
	uint16_t logical_block_size_msb;
	uint32_t path_table_size;
	uint32_t path_table_size_msb;
	uint32_t path_table_type_l;
	uint32_t path_table_type_l_opt;
	uint32_t path_table_type_m;
	uint32_t path_table_type_m_opt;
	char root_dir_entry[34];
	char volume_set_identifier[128];
	char publisher_identifier[128];
	char data_preparer_identifier[128];
	char application_identifier[128];
	char copyright_file_identifier[37];
	char abstract_file_identifier[37];
	char bibliographic_file_identifier[37];
	_cdfs_datetime_t creation_date;
	_cdfs_datetime_t modification_date;
	_cdfs_datetime_t expiration_date;
	_cdfs_datetime_t effective_date;
	char file_structure_version;
	char unused882;
	char application_used[512];
	char reserved[653];
} __attribute__((packed)) _cdfs_pvd_t;
static _cdfs_pvd_t _cdfs_pvd;
static uint64_t _cdfs_pvd_byteoff; //Location of the PVD in the block device, in bytes

//Block cache
#define BLK_CACHE_LINE 8192
typedef struct _blk_cache_s
{
	int valid;
	int sector;
	int age;
	uint32_t data[BLK_CACHE_LINE];
} _blk_cache_t;
#define BLK_CACHE_MAX 64
static _blk_cache_t _blk_cache[BLK_CACHE_MAX];

//Optional hook for looking at data as it is read
__attribute__((weak)) void _cdfs_blk_hook(uint32_t sector, void *buf, int nsectors)
{
	(void)sector;
	(void)buf;
	(void)nsectors;
}

//Loads sector of data into block cache
void *_blk_cache_fill(int sector)
{
	#define SECTOR_PER_LINE (BLK_CACHE_LINE/2048)
	
	//Look for existing entries
	for(int cc = 0; cc < BLK_CACHE_MAX; cc++)
	{
		if(_blk_cache[cc].valid == 0)
			continue;
		if((_blk_cache[cc].sector / SECTOR_PER_LINE) != (sector / SECTOR_PER_LINE))
			continue;
		
		//Found existing entry
		_blk_cache[cc].age /= 2;
		return _blk_cache[cc].data + (((sector % SECTOR_PER_LINE) * 2048)/sizeof(uint32_t));
	}
	
	//No existing entry. Find which entry we'll evict.
	int best = 0;
	for(int cc = 0; cc < BLK_CACHE_MAX; cc++)
	{
		_blk_cache[cc].age++;
		if(_blk_cache[cc].valid == 0)
		{
			//Invalid ones are always best to overwrite
			best = cc;
		}
		else if(_blk_cache[best].valid && (_blk_cache[cc].age > _blk_cache[best].age))
		{
			//Otherwise, older ones are better
			best = cc;
		}
	}
			
	_blk_cache[best].valid = 1;
	_blk_cache[best].sector = sector;
	_blk_cache[best].age = 0;
	
	while(1)
	{
		int read_result = _sc_disk_read2k((sector / SECTOR_PER_LINE) * SECTOR_PER_LINE, _blk_cache[best].data, SECTOR_PER_LINE);
		
		if(read_result == -_SC_EAGAIN)
		{
			//In progress
			_sc_pause();
			continue;
		}
		
		if(read_result < 0)
		{
			//Other errors, just retry
			continue;
		}
		
		//Allow debugging hooks
		_cdfs_blk_hook((uint32_t)((sector / SECTOR_PER_LINE) * SECTOR_PER_LINE), _blk_cache[best].data, SECTOR_PER_LINE);

		return _blk_cache[best].data + (((sector % SECTOR_PER_LINE) * 2048)/sizeof(uint32_t));
	}
}

//Reads bytes from block device via sector-level cache
int _blk_read(int byteoff, void *dstv, int len)
{
	unsigned char *dst = (unsigned char *)dstv;
	int nread = 0;
	while(len > 0)
	{
		const char *blkbuf = _blk_cache_fill(byteoff / 2048);
		int misalign = byteoff % 2048;
		int to_copy = 2048 - misalign;
		if(to_copy > len)
			to_copy = len;
		
		memcpy(dst, blkbuf + misalign, to_copy);
		byteoff += to_copy;
		dst += to_copy;
		nread += to_copy;
		len -= to_copy;
	}
	return nread;
}

//Writes bytes to block device, bypassing cache, invalidating cached entries
int _blk_write(int byteoff, const void *srcv, int len)
{
	//Look for any block-cache entries that we touch. Invalidate them.
	int firstblock = byteoff / 2048;
	int lastblock = (byteoff + len - 1) / 2048;
	for(int bb = 0; bb < BLK_CACHE_MAX; bb++)
	{
		if( (_blk_cache[bb].sector/SECTOR_PER_LINE) < (firstblock/SECTOR_PER_LINE) - 1)
			continue;
		
		if( (_blk_cache[bb].sector/SECTOR_PER_LINE) > (lastblock/SECTOR_PER_LINE) + 1)
			continue;
		
		_blk_cache[bb].valid = 0;
	}
	
	const unsigned char *src = (const unsigned char *)srcv;
	int nwritten = 0;
	while(len > 0)
	{
		//Compute location/size within the next block
		int misalign = byteoff % 2048;
		int to_copy = 2048 - misalign;
		if(to_copy > len)
			to_copy = len;
		
		//Fill the buffer with old data if necessary
		static uint8_t blkbuf[2048];
		if(misalign != 0 || to_copy != 2048)
		{
			//Need to read-modify-write
			while(1)
			{
				int read_result = _sc_disk_read2k(byteoff/2048, blkbuf, 1);
				
				if(read_result == -_SC_EAGAIN)
				{
					//In progress
					_sc_pause();
					continue;
				}
				
				if(read_result < 0)
				{
					//Other errors, just retry
					continue;
				}
				
				//Success
				break;
			}
		}			
		
		//Copy the new data in the buffer
		memcpy(blkbuf + misalign, src, to_copy);
		
		//Write back to disk
		while(1)
		{
			int write_result = _sc_disk_write2k(byteoff/2048, blkbuf, 1);
			
			if(write_result == -_SC_EAGAIN)
			{
				//In progress
				_sc_pause();
				continue;
			}
			
			if(write_result < 0)
			{
				//Other errors, just retry
				continue;
			}
			
			//Success
			break;
		}
		
		byteoff += to_copy;
		src += to_copy;
		nwritten += to_copy;
		len -= to_copy;
		
		
	}
	return nwritten;
}

//Translates an ISO9660 directory entry to PVMK directory entry and file status.
//This forms most of the "hard work" we do, as the ISO9660 directory entries are essentially their file structure.
//We use the location of a file's directory-entry as its "inode number" for our identification.
static int _cdfs_translate_dirent_uncached(uint64_t byteoff, uint64_t maxlen, struct _dirent_storage *de_out, struct stat *st_out, uint64_t *loc_out, uint64_t *len_out)
{
	//Don't even try if we don't have a valid filesystem
	if(!_cdfs_present)
		return -_SC_ENXIO;
	
	//Validate that the directory entry fits in the space of the card
	//if(byteoff >= blk_size())
	//	return -_SC_EIO;
	
	//if(byteoff + maxlen > blk_size())
	//	maxlen = blk_size() - byteoff;
	
	//Only read up to the max directory entry size at once
	_cdfs_dirent_t de = {0};
	int to_read = maxlen;
	if(to_read > (int)sizeof(de))
		to_read = sizeof(de);
	
	//Read the directory entry from the ISO9660 FS
	int nread = _blk_read(byteoff, &de, sizeof(de));
	if(nread < 0)
		return nread;
	
	//There may be padding at the end of a sector, which doesn't contain more directory entries.
	//This may or may not be the actual end of the directory though.
	if(de.length == 0)
	{
		//Return a dummy entry, meaning there's no actual dirent here
		if(de_out != NULL)
			memset(de_out, 0, sizeof(*de_out));
		
		if(st_out != NULL)
			memset(st_out, 0, sizeof(*st_out));
		
		if(loc_out != NULL)
			*loc_out = 0;
		
		if(len_out != NULL)
			*len_out = 0;
		
		//Consume the rest of the CD sector
		int lbs_log2 = 0;
		int lbs = _cdfs_pvd.logical_block_size;
		while(lbs > 1)
		{
			lbs_log2++;
			lbs /= 2;
		}
		return _cdfs_pvd.logical_block_size - (byteoff & ~(0xFFFFFFFFu << lbs_log2));
	}
	
	//Sanity check the size of the directory entry and its contents	
	if(de.length > maxlen)
		return -_SC_EIO;
	
	//if( (de.lba * _cdfs_pvd.logical_block_size) >= blk_size() )
	//	return -_SC_EIO;
	
	//if( (de.lba * _cdfs_pvd.logical_block_size) + de.len > blk_size() )
	//	return -_SC_EIO;
	
	if(de.filename_len >= (int)sizeof(de.filename_and_system_use))
		return -_SC_EIO;
	
	//See if the directory entry encodes Rock Ridge info.
	//Search for the SUSP tags that we care about in the System Use area.
	enum susp_tag_e { SUSP_RR, SUSP_NM, SUSP_PX, SUSP_MAX };
	static const char susp_tag[SUSP_MAX][2] =
	{
		[SUSP_RR] = "RR", //Rock Ridge tag
		[SUSP_NM] = "NM", //Filename
		[SUSP_PX] = "PX", //POSIX attributes (file mode, especially)
	};
	
	const char *susp_loc[SUSP_MAX] = {0};
	uint8_t susp_len[SUSP_MAX] = {0};
	
	//Only actually look for the info if the caller might care about it
	if(st_out != NULL || de_out != NULL)
	{
		const char *system_use = de.filename_and_system_use + de.filename_len;
		if(!(de.filename_len % 2))
			system_use++;
		
		if(system_use[0] != ' ')
		{
			while(1)
			{
				if( (system_use + 3) > ((const char*)&de) + sizeof(de))
					break;
				
				if( (system_use + 3) > ((const char*)&de) + de.length)
					break;
				
				//Check if this is a tag we recognize
				for(int ss = 0; ss < SUSP_MAX; ss++)
				{
					if(!memcmp(susp_tag[ss], system_use, 2))
					{
						//Found the tag
						susp_loc[ss] = system_use;
						susp_len[ss] = system_use[2];
						break;
					}
				}
				
				//Move past it
				system_use += (uint8_t)(system_use[2]);
			}
		}
	}
		
	//Output the status information
	if(st_out != NULL)
	{
		memset(st_out, 0, sizeof(*st_out));
		st_out->st_dev = 1;
		st_out->st_ino = byteoff;
		st_out->st_size = de.len;
		st_out->st_blocks = de.len / 512;
		st_out->st_nlink = 1;
		st_out->st_rdev = 0;
		
		if( /*susp_loc[SUSP_RR] != NULL &&*/ susp_loc[SUSP_PX] != NULL) //Why doesn't xorriso output a RR record?
		{
			//Has Rock Ridge mode info
			uint32_t rr_mode = 0;
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][7]);
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][6]);
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][5]);
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][4]);
			
			//Permission bits are the same as in Rock Ridge (and everywhere else ever)
			st_out->st_mode = rr_mode & 0777;
			
			//Translate the file type field
			switch(rr_mode & 0770000)
			{
				case 0140000: st_out->st_mode |= S_IFSOCK; break;
				case 0120000: st_out->st_mode |= S_IFLNK; break;
				case 0100000: st_out->st_mode |= S_IFREG; break;
				case 0060000: st_out->st_mode |= S_IFBLK; break;
				case 0020000: st_out->st_mode |= S_IFCHR; break;
				case 0040000: st_out->st_mode |= S_IFDIR; break;
				case 0010000: st_out->st_mode |= S_IFIFO; break;
				default: break;
			}
			
			if( (st_out->st_mode & S_IFMT) == 0 )
			{
				//Didn't get a valid mode...?
				st_out->st_mode = S_IFREG | 0000;
			}
		}
		else if(de.flags & (1u << 1))
		{
			//Directory
			st_out->st_mode = S_IFDIR | 0444;
		}
		else
		{
			//Regular file
			st_out->st_mode = S_IFREG | 0444;
			
			//If the filename ends in .nne, force it to be executable.
			//Note that this only looks at the level-1 ISO9660 name field.
			//It's assumed that using Rock Ridge or similar, permissions would be specified explicitly.
			if((de.filename_len > 6) && !memcmp(de.filename_and_system_use + de.filename_len - 6, ".NNE;1", 6))
				st_out->st_mode |= 0111;
		}
	}
	
	//Output the directory entry
	if(de_out != NULL)
	{
		memset(de_out, 0, sizeof(*de_out));
		de_out->d_ino = byteoff;
		
		if(de.filename_and_system_use[0] == 0 && de.filename_len == 1)
		{
			//Empty string represents current directory in ISO9660. Report as ".".
			de_out->d_name[0] = '.';
			de_out->d_name[1] = '\0';
		}
		if(de.filename_and_system_use[0] == 1 && de.filename_len == 1)
		{
			//String "\x01" represents parent directory in ISO9660. Report as "..".
			de_out->d_name[0] = '.';
			de_out->d_name[1] = '.';
			de_out->d_name[2] = '\0';
		}
		else if(/*susp_loc[SUSP_RR] != NULL &&*/ susp_loc[SUSP_NM] != NULL) //Why doesn't xorriso output a RR record?
		{
			//Found a Rock Ridge filename
			int to_copy = susp_len[SUSP_NM] - 5;
			if(to_copy > (int)sizeof(de_out->d_name) - 1)
				to_copy = sizeof(de_out->d_name) - 1;
			
			if(to_copy > 0)
				strncpy(de_out->d_name, susp_loc[SUSP_NM] + 5, to_copy);
		}
		else
		{
			//Normal filename
			int to_copy = de.filename_len - 2; //Exclude ";1"
			if(to_copy > (int)sizeof(de_out->d_name) - 1)
				to_copy = sizeof(de_out->d_name) - 1;
		
			if(to_copy > 0)
				strncpy(de_out->d_name, de.filename_and_system_use, to_copy);
		}
	}
	
	//Output the data location/size
	if(loc_out != NULL)
		*loc_out = de.lba * _cdfs_pvd.logical_block_size;
	
	if(len_out != NULL)
		*len_out = de.len;
	
	//Return the length of the directory entry consumed from the CD FS
	return de.length;
}

//Cache of translation results, turning ISO9660 directory entries into our status + dirents.
typedef struct _cdfs_inode_cache_s
{
	uint64_t ino;    //Inode (byte position of directory entry) being cached here
	
	int entryage;    //How recently this cache entry was used
	
	int delen;       //Length of directory entry originally translated
	struct _dirent_storage de; //Resulting directory entry when translated
	struct stat st;   //Resulting file status when translated
	uint64_t loc;    //Resulting data/contents location (byte offset) when translated
	uint64_t len;    //Resulting data/contents length (byte count) when translated
} _cdfs_inode_cache_t;
#define CDFS_INODE_CACHE_MAX 128
static _cdfs_inode_cache_t _cdfs_inode_cache[CDFS_INODE_CACHE_MAX];

//Translates an ISO9660 directory entry, looking it up in the cache if possible
static int _cdfs_translate_dirent(uint64_t byteoff, uint64_t maxlen, struct _dirent_storage *de_out, struct stat *st_out, uint64_t *loc_out, uint64_t *len_out)
{
	//See if the inode is in the cache
	int lru_idx = 0;
	int lru_age = 0;
	for(int cc = 0; cc < CDFS_INODE_CACHE_MAX; cc++)
	{
		_cdfs_inode_cache_t *cptr = &(_cdfs_inode_cache[cc]);
		cptr->entryage++;
		
		if(cptr->ino == byteoff)
		{
			//Found it
			if(de_out != NULL)
				*de_out = cptr->de;
			if(st_out != NULL)
				*st_out = cptr->st;
			if(loc_out != NULL)
				*loc_out = cptr->loc;
			if(len_out != NULL)
				*len_out = cptr->len;
			
			cptr->entryage = 0;
			return cptr->delen;	
		}
		
		if(cptr->entryage > lru_age)
		{
			//Found a new best entry to replace, if it comes to that
			lru_idx = cc;
			lru_age = cptr->entryage;
		}
	}
	
	//Wasn't in the cache. Translate and put it in the cache.
	_cdfs_inode_cache_t *newentry = &(_cdfs_inode_cache[lru_idx]);
	memset(newentry, 0, sizeof(*newentry));
	
	int result = _cdfs_translate_dirent_uncached(byteoff, maxlen, &(newentry->de), &(newentry->st), &(newentry->loc), &(newentry->len));
	if(result < 0)
	{
		//Failure... return the error code without putting it in the cache
		return result;
	}
	
	//Successfully filled a new cache entry with this translation
	newentry->entryage = 0;
	newentry->delen = result;
	newentry->ino = byteoff;
	
	if(de_out != NULL)
		*de_out = newentry->de;
	if(st_out != NULL)
		*st_out = newentry->st;
	if(loc_out != NULL)
		*loc_out = newentry->loc;
	if(len_out != NULL)
		*len_out = newentry->len;
	
	return newentry->delen;
}

void _cdfs_init(void)
{
	//PANIC_ASSERT(sizeof(_cdfs_pvd_t) == 2048);
	//PANIC_ASSERT(sizeof(_cdfs_dirent_t) == 256);
	
	//Try to examine the ISO9660 filesystem at the beginning of the block device.
	//If it's not valid, we'll just return -_SC_ENXIO for everything.
	_cdfs_present = false;
	
	//Look through the volume descriptors to find the Primary Volume Descriptor.
	//Volume descriptors start at sector 16 (+32KByte into the volume).
	int pvd_sector = 16;
	while(1)
	{
		//Read the next 2KByte sector and see what's there
		uint64_t offset = pvd_sector * 2048ull;
		//if(offset >= blk_size())
		//{
		//	//Block device ended really soon...? Or maybe not present at all.
		//	return;
		//}
		
		int nread = _blk_read(offset, &_cdfs_pvd, sizeof(_cdfs_pvd));
		if(nread < 0)
			return;
		
		//If the first byte is 0xFF, it's the end of the volume descriptors.
		if((uint8_t)(_cdfs_pvd.magic[0]) == 0xFF)
		{
			//Reached end of volume descriptors, didn't find the primary volume descriptor
			return;
		}
		
		//The primary volume descriptor will always start with these 8 bytes.
		//Other volume descriptors may start with other sequences.
		if(memcmp(_cdfs_pvd.magic, "\x01" "CD001" "\x01" "\x00", 8) != 0)
		{
			//Found some other volume descriptor
			pvd_sector++;
			if(pvd_sector >= 128)
			{
				//Searched through 128 sectors and none of them contained the PVD
				return;
			}
		
			//Keep looking...
			continue;
		}
		
		//Okay, this is a primary volume descriptor.
		//Validate that it's sane.
		if(_cdfs_pvd.file_structure_version != 1)
		{
			//Unrecognized file structure version.
			return;
		}
		
		if(_cdfs_pvd.volume_space_size < 1 || _cdfs_pvd.logical_block_size < 1)
		{
			//Bad size
			return;
		}
		
		//uint64_t size_in_bytes = _cdfs_pvd.volume_space_size * _cdfs_pvd.logical_block_size;
		//if(size_in_bytes > blk_size())
		//{
		//	//Primary volume descriptor says the filesystem should be bigger than the card we're using.
		//	return;
		//}
		
		//Found the primary volume descriptor - it's in _cdfs_pvd.
		_cdfs_pvd_byteoff = offset;
		break;
	}
	
	//Success - we've got a valid CD filesystem here to use.
	_cdfs_present = true;
}

uint64_t _cdfs_fslen(void)
{
	if(!_cdfs_present)
		return 0;
	
	return _cdfs_pvd.volume_space_size * _cdfs_pvd.logical_block_size;
}

uint32_t _cdfs_rootino(void)
{
	if(!_cdfs_present)
		return 0;
	
	//Identify files by the byte offset of their directory entry.
	//In this case, the directory entry for the root directory is in the primary volume descriptor.
	return _cdfs_pvd_byteoff + offsetof(_cdfs_pvd_t, root_dir_entry);
}

static int _cdfs_extent_locate(uint64_t ino, uint64_t fileoff, uint64_t *loc_out, uint64_t *avail_out, uint64_t *size_out);

//Directory record flag for a file that continues in the next record
#define CDFS_MULTI_EXTENT 0x80

int _cdfs_stat(uint32_t ino, struct stat *buf)
{
	if(!_cdfs_present)
		return -_SC_ENXIO;
	
	int translate_result = _cdfs_translate_dirent(ino, sizeof(_cdfs_dirent_t), NULL, buf, NULL, NULL);
	if(translate_result < 0)
		return translate_result;

	//A multi-extent file's size is the sum of its chain, not the length of the first record
	if( (buf->st_mode & S_IFMT) == S_IFREG )
	{
		uint8_t hdr[33];
		if(_blk_read(ino, hdr, 33) < 0)
			return -_SC_EIO;
		if(hdr[25] & CDFS_MULTI_EXTENT)
		{
			uint64_t totalsize = 0;
			int locate_result = _cdfs_extent_locate(ino, 0, NULL, NULL, &totalsize);
			if(locate_result < 0)
				return locate_result;
			buf->st_size = totalsize;
			buf->st_blocks = totalsize / 512;
		}
	}

	if(buf->st_dev == 0)
		return -_SC_EIO;
	
	return sizeof(*buf);
}

int _cdfs_statvfs(struct statvfs *buf)
{
	if(!_cdfs_present)
		return -_SC_ENXIO;
	
	memset(buf, 0, sizeof(*buf));
	//buf->part = 1;
	//buf->blksize = _cdfs_pvd.logical_block_size;
	//buf->blktotal = _cdfs_pvd.volume_space_size;
	//buf->blkfree = 0; //ISO9660 filesystems are read-only
	
	buf->f_fsid = 1;
	buf->f_namemax = 250;
	buf->f_bsize = _cdfs_pvd.logical_block_size;
	buf->f_frsize = _cdfs_pvd.logical_block_size;
	buf->f_blocks = _cdfs_pvd.volume_space_size;
	buf->f_bavail = 0;
	buf->f_bfree = 0;
	
	return sizeof(*buf);
}

//Finds the Nth visible entry in a directory, counting an ISO9660 multi-extent chain as one.
//Returns 0 and sets the record's location and length on success, 1 when the index is past the end.
static int _cdfs_dir_record(uint64_t dir_loc, uint64_t dir_len, uint32_t want, uint64_t *rec_loc_out, uint64_t *rec_len_out)
{
	uint64_t loc = dir_loc;
	uint64_t left = dir_len;
	uint32_t visible = 0;
	int prev_multi = 0;
	char prev_name[256];
	int prev_namelen = 0;

	while(left > 0)
	{
		uint8_t hdr[33];
		if(_blk_read(loc, hdr, 33) < 0)
			return -_SC_EIO;

		int reclen = hdr[0];
		if(reclen == 0)
		{
			//Ran into the padding at the end of a sector; the rest of it holds no entries
			uint64_t nextsector = (loc / _cdfs_pvd.logical_block_size) + 1;
			uint64_t nextloc = nextsector * _cdfs_pvd.logical_block_size;
			if(nextloc <= loc)
				return 1;

			left -= (nextloc - loc);
			loc = nextloc;
			prev_multi = 0;
			continue;
		}

		if(reclen < 33 || (uint64_t)reclen > left)
			return -_SC_EIO;

		int namelen = hdr[32];
		char name[256];
		if(namelen > 0 && namelen < (int)sizeof(name))
		{
			if(_blk_read(loc + 33, name, namelen) < 0)
				return -_SC_EIO;
			name[namelen] = '\0';
		}
		else
		{
			namelen = 0;
			name[0] = '\0';
		}

		//A record that continues the previous one is not a separate entry
		int continuation = 0;
		if(prev_multi && namelen == prev_namelen && !memcmp(name, prev_name, namelen))
			continuation = 1;

		if(!continuation)
		{
			if(visible == want)
			{
				if(rec_loc_out != NULL)
					*rec_loc_out = loc;
				if(rec_len_out != NULL)
					*rec_len_out = reclen;
				return 0;
			}
			visible++;
		}

		prev_multi = (hdr[25] & CDFS_MULTI_EXTENT) ? 1 : 0;
		memcpy(prev_name, name, namelen);
		prev_name[namelen] = '\0';
		prev_namelen = namelen;

		loc += reclen;
		left -= reclen;
	}

	return 1;
}

//Walks the extent chain of a file, locating the extent holding the given file offset.
//Sets the byte offset on the disc of the data wanted, how many bytes remain in that extent,
//and the total size of the file.
static int _cdfs_extent_locate(uint64_t ino, uint64_t fileoff, uint64_t *loc_out, uint64_t *avail_out, uint64_t *size_out)
{
	uint64_t cur = ino;
	uint64_t pos = 0;
	uint64_t found_loc = 0;
	uint64_t found_avail = 0;
	int found = 0;

	while(1)
	{
		struct stat st = {0};
		uint64_t dloc = 0;
		uint64_t dlen = 0;
		int reclen = _cdfs_translate_dirent(cur, sizeof(_cdfs_dirent_t), NULL, &st, &dloc, &dlen);
		if(reclen < 0)
			return reclen;

		if(fileoff >= pos && fileoff < pos + dlen)
		{
			found_loc = dloc + (fileoff - pos);
			found_avail = (pos + dlen) - fileoff;
			found = 1;
		}

		pos += dlen;

		uint8_t hdr[33];
		if(_blk_read(cur, hdr, 33) < 0)
			return -_SC_EIO;
		if(!(hdr[25] & CDFS_MULTI_EXTENT))
			break;

		//A directory record may not cross a logical block boundary, so a chain that runs past
		//one is padded out to the start of the next block. Step over that padding the same way
		//the directory reader does, or the walk reads a zero-length record as the end of the
		//chain and a file spanning a block boundary reports only the part before it.
		cur += reclen;
		uint8_t padlen = 0;
		if(_blk_read(cur, &padlen, 1) < 0)
			return -_SC_EIO;
		if(padlen == 0)
		{
			uint64_t nextloc = ((cur / _cdfs_pvd.logical_block_size) + 1) * _cdfs_pvd.logical_block_size;
			if(nextloc <= cur)
				break;
			cur = nextloc;
		}

		uint8_t hdrnext[33];
		if(_blk_read(cur, hdrnext, 33) < 0)
			return -_SC_EIO;
		if(hdrnext[0] < 33)
			break; //Nothing usable here, so this was the end of the chain
	}

	if(loc_out != NULL)
		*loc_out = found_loc;
	if(avail_out != NULL)
		*avail_out = found_avail;
	if(size_out != NULL)
		*size_out = pos;

	return found ? 0 : -_SC_EIO;
}

int _cdfs_read(uint32_t ino, uint32_t off, void *buf, int len)
{
	//Compute information about the file being read
	struct stat st = {0};
	uint64_t data_loc = 0;
	uint64_t data_len = 0;
	int translate_result = _cdfs_translate_dirent(ino, sizeof(_cdfs_dirent_t), NULL, &st, &data_loc, &data_len);
	if(translate_result < 0)
		return translate_result;
	
	if( (st.st_mode & S_IFMT) == S_IFDIR )
	{
		//File on the CD is a directory.
		//Translate CD directory entries to PVMK directory entries.
		
		//Our directory entries are all uniformly-sized, as the user sees them.
		//Offset therefore tells us which directory entry we're reading, and a byte offset within.
		uint32_t which_dirent = off / sizeof(struct _dirent_storage);
		uint32_t fraction = off % sizeof(struct _dirent_storage);

		//Generate as many directory entries as they asked for
		char *bufb = (char*)buf;
		int ncopied = 0;
		while(len > 0)
		{
			//Find the record for the entry we want, counting an extent chain as one entry
			uint64_t rec_loc = 0;
			uint64_t rec_len = 0;
			int found = _cdfs_dir_record(data_loc, data_len, which_dirent, &rec_loc, &rec_len);
			if(found < 0)
				return found;
			if(found > 0)
				break; //No more directory entries in the directory

			//Parse that CD directory entry
			struct _dirent_storage de = {0};
			int parsed_cd_dirent = _cdfs_translate_dirent(rec_loc, rec_len, &de, NULL, NULL, NULL);
			if(parsed_cd_dirent < 0)
				return parsed_cd_dirent;

			//Copy the resulting PVMK directory entry out to the user
			int to_copy = sizeof(de) - fraction;
			if(to_copy > len)
				to_copy = len;

			memcpy(bufb, ((char*)(&de)) + fraction, to_copy);

			ncopied += to_copy;
			bufb += to_copy;
			len -= to_copy;
			fraction = 0;
			which_dirent++;
		}

		return ncopied;
	}
	
	//Regular file on CD. Return data directly.

	uint64_t filesize = data_len;
	{
		uint8_t hdr[33];
		if(_blk_read(ino, hdr, 33) < 0)
			return -_SC_EIO;
		if(hdr[25] & CDFS_MULTI_EXTENT)
		{
			int locate_result = _cdfs_extent_locate(ino, 0, NULL, NULL, &filesize);
			if(locate_result < 0)
				return locate_result;
		}
	}

	if(off >= filesize)
	{
		//Entirely off the end of the file
		return 0;
	}

	if(off + len > filesize)
	{
		//Need to truncate so we don't run off the end
		len = filesize - off;
	}

	//Read across the extent chain, handing each piece to the hook with the location it came from
	char *bufb = (char*)buf;
	int nread_total = 0;
	uint64_t fileoff = off;
	while(nread_total < len)
	{
		uint64_t extent_loc = 0;
		uint64_t extent_avail = 0;
		int locate_result = _cdfs_extent_locate(ino, fileoff, &extent_loc, &extent_avail, NULL);
		if(locate_result < 0)
			return locate_result;
		if(extent_avail == 0)
			break;

		int chunk = len - nread_total;
		if((uint64_t)chunk > extent_avail)
			chunk = (int)extent_avail;

		_blk_read(extent_loc, bufb, chunk);

		bufb += chunk;
		fileoff += chunk;
		nread_total += chunk;
	}

	return nread_total;
}

int _cdfs_write(uint32_t ino, uint32_t off, const void *buf, int len)
{
	//Compute information about the file being read
	struct stat st = {0};
	uint64_t data_loc = 0;
	uint64_t data_len = 0;
	int translate_result = _cdfs_translate_dirent(ino, sizeof(_cdfs_dirent_t), NULL, &st, &data_loc, &data_len);
	if(translate_result < 0)
		return translate_result;
	
	if( (st.st_mode & S_IFMT) == S_IFDIR )
	{
		//File on the CD is a directory.
		return -_SC_EISDIR;
	}
		
	//Regular file on CD.
	if(off >= data_len)
	{
		//Entirely off the end of the file
		return -_SC_ENOSPC;
	}
	
	if(off + len > data_len)
	{
		//Need to truncate so we don't run off the end
		len = data_len - off;
	}
	
	_blk_write(data_loc + off, buf, len);
	return len;
}



int _cdfs_search(uint32_t ino, const char *filename)
{
	//Compute information about the file being read
	struct stat directory_st = {0};
	uint64_t data_loc = 0;
	uint64_t data_len = 0;
	int translate_result = _cdfs_translate_dirent(ino, sizeof(_cdfs_dirent_t), NULL, &directory_st, &data_loc, &data_len);
	if(translate_result < 0)
		return translate_result;

	//The directory's directory entry should say it's a directory.
	if( (directory_st.st_mode & S_IFMT) != S_IFDIR )
		return -_SC_ENOTDIR;
	
	//Hack to quickly find current directory
	if(filename[0] == '.' && filename[1] == '\0')
		return ino;
	
	//The directory's directory entry tells us where its contents are.
	while(data_len > 0)
	{
		//Interpret the next directory entry from the directory's contents
		struct _dirent_storage search_de = {0};
		struct stat search_st = {0};
		int translate_result = _cdfs_translate_dirent(data_loc, data_len, &search_de, &search_st, NULL, NULL);
		if(translate_result < 0)
			return translate_result;
		
		//Move past it.
		//PANIC_ASSERT((unsigned)translate_result <= data_len);
		data_loc += translate_result;
		data_len -= translate_result;	
		
		//See if it's what we're looking for
		//if(!strcmp(filename, search_de.d_name))
		//	return search_de.d_ino;
		if(!strcasecmp(filename, search_de.d_name)) //Case-insensitive because ISO9660 utilities mangle case sometimes
			return search_de.d_ino;
	}
	
	//Didn't find it
	return -_SC_ENOENT;
}
