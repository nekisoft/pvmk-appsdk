//cdfs.c
//Access to ISO9660 filesystem on card
//Bryan E. Topp <betopp@betopp.com> 2024

#include "cdfs.h"
#include "blk.h"
#include "panic.h"

//Whether we found a working ISO9660 filesystem.
static bool cdfs_present;

//Date-time value used in ISO9660
typedef struct cdfs_datetime_s
{
	char whatever[17]; //Don't care
} cdfs_datetime_t;

//ISO9660 directory entry (may be shorter than this)
typedef struct cdfs_dirent_s
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
} __attribute__((packed)) cdfs_dirent_t;

//Structure of a ISO9660 primary volume descriptor
typedef struct cdfs_pvd_s
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
	cdfs_datetime_t creation_date;
	cdfs_datetime_t modification_date;
	cdfs_datetime_t expiration_date;
	cdfs_datetime_t effective_date;
	char file_structure_version;
	char unused882;
	char application_used[512];
	char reserved[653];
} __attribute__((packed)) cdfs_pvd_t;
static cdfs_pvd_t cdfs_pvd;
static uint64_t cdfs_pvd_byteoff; //Location of the PVD in the block device, in bytes

//Translates an ISO9660 directory entry to PVMK directory entry and file status.
//This forms most of the "hard work" we do, as the ISO9660 directory entries are essentially their file structure.
//We use the location of a file's directory-entry as its "inode number" for our identification.
static int cdfs_translate_dirent_uncached(uint64_t byteoff, uint64_t maxlen, _sc_dirent_t *de_out, _sc_stat_t *st_out, uint64_t *loc_out, uint64_t *len_out)
{
	//Don't even try if we don't have a valid filesystem
	if(!cdfs_present)
		return -_SC_ENXIO;
	
	//Validate that the directory entry fits in the space of the card
	if(byteoff >= blk_size())
		return -_SC_EIO;
	
	if(byteoff + maxlen > blk_size())
		maxlen = blk_size() - byteoff;
	
	//Only read up to the max directory entry size at once
	cdfs_dirent_t de = {0};
	int to_read = maxlen;
	if(to_read > (int)sizeof(de))
		to_read = sizeof(de);
	
	//Read the directory entry from the ISO9660 FS
	blk_read(byteoff, &de, sizeof(de));
	
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
		int lbs = cdfs_pvd.logical_block_size;
		while(lbs > 1)
		{
			lbs_log2++;
			lbs /= 2;
		}
		return cdfs_pvd.logical_block_size - (byteoff & ~(0xFFFFFFFFu << lbs_log2));
	}
	
	//Sanity check the size of the directory entry and its contents	
	if(de.length > maxlen)
		return -_SC_EIO;
	
	if( (de.lba * cdfs_pvd.logical_block_size) >= blk_size() )
		return -_SC_EIO;
	
	if( (de.lba * cdfs_pvd.logical_block_size) + de.len > blk_size() )
		return -_SC_EIO;
	
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
		st_out->part = 1;
		st_out->ino = byteoff;
		st_out->size = de.len;
		st_out->used = de.len;
		st_out->nlinks = 1;
		st_out->rdev = 0;
		
		if(susp_loc[SUSP_RR] != NULL && susp_loc[SUSP_PX] != NULL)
		{
			//Has Rock Ridge mode info
			uint32_t rr_mode = 0;
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][7]);
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][6]);
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][5]);
			rr_mode = (rr_mode << 8) | (uint8_t)(susp_loc[SUSP_PX][4]);
			
			//Permission bits are the same as in Rock Ridge (and everywhere else ever)
			st_out->mode = rr_mode & 0777;
			
			//Translate the file type field
			switch(rr_mode & 0770000)
			{
				case 0140000: st_out->mode |= _SC_S_IFSOCK; break;
				case 0120000: st_out->mode |= _SC_S_IFLNK; break;
				case 0100000: st_out->mode |= _SC_S_IFREG; break;
				case 0060000: st_out->mode |= _SC_S_IFBLK; break;
				case 0020000: st_out->mode |= _SC_S_IFCHR; break;
				case 0040000: st_out->mode |= _SC_S_IFDIR; break;
				case 0010000: st_out->mode |= _SC_S_IFIFO; break;
				default: break;
			}
			
			if( (st_out->mode & _SC_S_IFMT) == 0 )
			{
				//Didn't get a valid mode...?
				st_out->mode = _SC_S_IFREG | 0000;
			}
		}
		else if(de.flags & (1u << 1))
		{
			//Directory
			st_out->mode = _SC_S_IFDIR | 0444;
		}
		else
		{
			//Regular file
			st_out->mode = _SC_S_IFREG | 0444;
			
			//If the filename ends in .nne, force it to be executable.
			//Note that this only looks at the level-1 ISO9660 name field.
			//It's assumed that using Rock Ridge or similar, permissions would be specified explicitly.
			if((de.filename_len > 6) && !memcmp(de.filename_and_system_use + de.filename_len - 6, ".NNE;1", 6))
				st_out->mode |= 0111;
		}
	}
	
	//Output the directory entry
	if(de_out != NULL)
	{
		memset(de_out, 0, sizeof(*de_out));
		de_out->ino = byteoff;
		
		if(de.filename_and_system_use[0] == 0 && de.filename_len == 1)
		{
			//Empty string represents current directory in ISO9660. Report as ".".
			de_out->name[0] = '.';
			de_out->name[1] = '\0';
		}
		if(de.filename_and_system_use[0] == 1 && de.filename_len == 1)
		{
			//String "\x01" represents parent directory in ISO9660. Report as "..".
			de_out->name[0] = '.';
			de_out->name[1] = '.';
			de_out->name[2] = '\0';
		}
		else if(susp_loc[SUSP_RR] != NULL && susp_loc[SUSP_NM] != NULL)
		{
			//Found a Rock Ridge filename
			int to_copy = susp_len[SUSP_NM] - 5;
			if(to_copy > (int)sizeof(de_out->name) - 1)
				to_copy = sizeof(de_out->name) - 1;
			
			if(to_copy > 0)
				strncpy(de_out->name, susp_loc[SUSP_NM] + 5, to_copy);
		}
		else
		{
			//Normal filename
			int to_copy = de.filename_len - 2; //Exclude ";1"
			if(to_copy > (int)sizeof(de_out->name) - 1)
				to_copy = sizeof(de_out->name) - 1;
		
			if(to_copy > 0)
				strncpy(de_out->name, de.filename_and_system_use, to_copy);
		}
	}
	
	//Output the data location/size
	if(loc_out != NULL)
		*loc_out = de.lba * cdfs_pvd.logical_block_size;
	
	if(len_out != NULL)
		*len_out = de.len;
	
	//Return the length of the directory entry consumed from the CD FS
	return de.length;
}

//Cache of translation results, turning ISO9660 directory entries into our status + dirents.
typedef struct cdfs_inode_cache_s
{
	uint64_t ino;    //Inode (byte position of directory entry) being cached here
	
	int entryage;    //How recently this cache entry was used
	
	int delen;       //Length of directory entry originally translated
	_sc_dirent_t de; //Resulting directory entry when translated
	_sc_stat_t st;   //Resulting file status when translated
	uint64_t loc;    //Resulting data/contents location (byte offset) when translated
	uint64_t len;    //Resulting data/contents length (byte count) when translated
} cdfs_inode_cache_t;
#define CDFS_INODE_CACHE_MAX 128
static cdfs_inode_cache_t cdfs_inode_cache[CDFS_INODE_CACHE_MAX];

//Translates an ISO9660 directory entry, looking it up in the cache if possible
static int cdfs_translate_dirent(uint64_t byteoff, uint64_t maxlen, _sc_dirent_t *de_out, _sc_stat_t *st_out, uint64_t *loc_out, uint64_t *len_out)
{
	//See if the inode is in the cache
	int lru_idx = 0;
	int lru_age = 0;
	for(int cc = 0; cc < CDFS_INODE_CACHE_MAX; cc++)
	{
		cdfs_inode_cache_t *cptr = &(cdfs_inode_cache[cc]);
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
	cdfs_inode_cache_t *newentry = &(cdfs_inode_cache[lru_idx]);
	memset(newentry, 0, sizeof(*newentry));
	
	int result = cdfs_translate_dirent_uncached(byteoff, maxlen, &(newentry->de), &(newentry->st), &(newentry->loc), &(newentry->len));
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

void cdfs_init(void)
{
	PANIC_ASSERT(sizeof(cdfs_pvd_t) == 2048);
	PANIC_ASSERT(sizeof(cdfs_dirent_t) == 256);
	
	//Try to examine the ISO9660 filesystem at the beginning of the block device.
	//If it's not valid, we'll just return -_SC_ENXIO for everything.
	cdfs_present = false;
	
	//Look through the volume descriptors to find the Primary Volume Descriptor.
	//Volume descriptors start at sector 16 (+32KByte into the volume).
	int pvd_sector = 16;
	while(1)
	{
		//Read the next 2KByte sector and see what's there
		uint64_t offset = pvd_sector * 2048ull;
		if(offset >= blk_size())
		{
			//Block device ended really soon...? Or maybe not present at all.
			return;
		}
		
		blk_read(offset, &cdfs_pvd, sizeof(cdfs_pvd));
		
		//If the first byte is 0xFF, it's the end of the volume descriptors.
		if((uint8_t)(cdfs_pvd.magic[0]) == 0xFF)
		{
			//Reached end of volume descriptors, didn't find the primary volume descriptor
			return;
		}
		
		//The primary volume descriptor will always start with these 8 bytes.
		//Other volume descriptors may start with other sequences.
		if(memcmp(cdfs_pvd.magic, "\x01" "CD001" "\x01" "\x00", 8) != 0)
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
		if(cdfs_pvd.file_structure_version != 1)
		{
			//Unrecognized file structure version.
			return;
		}
		
		if(cdfs_pvd.volume_space_size < 1 || cdfs_pvd.logical_block_size < 1)
		{
			//Bad size
			return;
		}
		
		uint64_t size_in_bytes = cdfs_pvd.volume_space_size * cdfs_pvd.logical_block_size;
		if(size_in_bytes > blk_size())
		{
			//Primary volume descriptor says the filesystem should be bigger than the card we're using.
			return;
		}
		
		//Found the primary volume descriptor - it's in cdfs_pvd.
		cdfs_pvd_byteoff = offset;
		break;
	}
	
	//Success - we've got a valid CD filesystem here to use.
	cdfs_present = true;
}

uint64_t cdfs_fslen(void)
{
	if(!cdfs_present)
		return 0;
	
	return cdfs_pvd.volume_space_size * cdfs_pvd.logical_block_size;
}

uint32_t cdfs_rootino(void)
{
	if(!cdfs_present)
		return 0;
	
	//Identify files by the byte offset of their directory entry.
	//In this case, the directory entry for the root directory is in the primary volume descriptor.
	return cdfs_pvd_byteoff + offsetof(cdfs_pvd_t, root_dir_entry);
}

int cdfs_stat(uint32_t ino, _sc_stat_t *buf)
{
	if(!cdfs_present)
		return -_SC_ENXIO;
	
	int translate_result = cdfs_translate_dirent(ino, sizeof(cdfs_dirent_t), NULL, buf, NULL, NULL);
	if(translate_result < 0)
		return translate_result;
	
	if(buf->part == 0)
		return -_SC_EIO;
	
	return sizeof(*buf);
}

int cdfs_statvfs(_sc_statvfs_t *buf)
{
	if(!cdfs_present)
		return -_SC_ENXIO;
	
	memset(buf, 0, sizeof(*buf));
	buf->part = 1;
	buf->blksize = cdfs_pvd.logical_block_size;
	buf->blktotal = cdfs_pvd.volume_space_size;
	buf->blkfree = 0; //ISO9660 filesystems are read-only
	return sizeof(*buf);
}

int cdfs_read(uint32_t ino, uint32_t off, void *buf, int len)
{
	//Compute information about the file being read
	_sc_stat_t st = {0};
	uint64_t data_loc = 0;
	uint64_t data_len = 0;
	int translate_result = cdfs_translate_dirent(ino, sizeof(cdfs_dirent_t), NULL, &st, &data_loc, &data_len);
	if(translate_result < 0)
		return translate_result;
	
	if( (st.mode & _SC_S_IFMT) == _SC_S_IFDIR )
	{
		//File on the CD is a directory.
		//Translate CD directory entries to PVMK directory entries.
		
		//Our directory entries are all uniformly-sized, as the user sees them.
		//Offset therefore tells us which directory entry we're reading, and a byte offset within.
		uint32_t which_dirent = off / sizeof(_sc_dirent_t);
		uint32_t fraction = off % sizeof(_sc_dirent_t);
		
		//Skip appropriate number of ISO9660 directory entries before the one we want
		//(Todo - this turns reading a directory into O(n^2) time. Caching a single last-used offset would fix.)
		while(which_dirent > 0)
		{
			int skipped_cd_dirent = cdfs_translate_dirent(data_loc, data_len, NULL, NULL, NULL, NULL);
			if(skipped_cd_dirent < 0)
				return skipped_cd_dirent;
			
			//Skip over that directory entry
			PANIC_ASSERT((unsigned)skipped_cd_dirent <= data_len);
			data_loc += skipped_cd_dirent;
			data_len -= skipped_cd_dirent;
			
			//Have one less to skip now
			which_dirent--;
		}
		
		//Generate as many directory entries as they asked for
		char *bufb = (char*)buf;
		int ncopied = 0;
		while(len > 0)
		{
			if(data_len == 0)
				break; //No more directory entries in the directory
			
			//Parse the next CD directory entry
			_sc_dirent_t de = {0};
			int parsed_cd_dirent = cdfs_translate_dirent(data_loc, data_len, &de, NULL, NULL, NULL);
			if(parsed_cd_dirent < 0)
				return parsed_cd_dirent;
			
			//Move past it
			PANIC_ASSERT((unsigned)parsed_cd_dirent <= data_len);
			data_loc += parsed_cd_dirent;
			data_len -= parsed_cd_dirent;
			
			//Copy the resulting PVMK directory entry out to the user
			int to_copy = sizeof(de) - fraction;
			if(to_copy > len)
				to_copy = len;
			
			memcpy(bufb, ((char*)(&de)) + fraction, to_copy);
			
			ncopied += to_copy;
			bufb += to_copy;
			len -= to_copy;
			fraction = 0;
		}
		
		return ncopied;
	}
	
	//Regular file on CD. Return data directly.
	
	if(off >= data_len)
	{
		//Entirely off the end of the file
		return 0;
	}
	
	if(off + len > data_len)
	{
		//Need to truncate so we don't run off the end
		len = data_len - off;
	}
	
	blk_read(data_loc + off, buf, len);
	return len;
	
}

int cdfs_search(uint32_t ino, const char *filename)
{
	//Compute information about the file being read
	_sc_stat_t directory_st = {0};
	uint64_t data_loc = 0;
	uint64_t data_len = 0;
	int translate_result = cdfs_translate_dirent(ino, sizeof(cdfs_dirent_t), NULL, &directory_st, &data_loc, &data_len);
	if(translate_result < 0)
		return translate_result;

	//The directory's directory entry should say it's a directory.
	if( (directory_st.mode & _SC_S_IFMT) != _SC_S_IFDIR )
		return -_SC_ENOTDIR;
	
	//The directory's directory entry tells us where its contents are.
	while(data_len > 0)
	{
		//Interpret the next directory entry from the directory's contents
		_sc_dirent_t search_de = {0};
		_sc_stat_t search_st = {0};
		int translate_result = cdfs_translate_dirent(data_loc, data_len, &search_de, &search_st, NULL, NULL);
		if(translate_result < 0)
			return translate_result;
		
		//Move past it.
		PANIC_ASSERT((unsigned)translate_result <= data_len);
		data_loc += translate_result;
		data_len -= translate_result;	
		
		//See if it's what we're looking for
		if(!strcmp(filename, search_de.name))
			return search_de.ino;
	}
	
	//Didn't find it
	return -_SC_ENOENT;
}
