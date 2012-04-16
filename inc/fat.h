#ifndef _FAT_H
#define _FAT_H
#include <common.h>

#define GET_BYTE(_buff, _off)	( *(((uint8_t*)_buff) + _off) )

#define GET_16BIT_WORD(_buff, _off)   ( *(((uint8_t *)_buff) + _off) |			\
								 *(((uint8_t *)_buff) + _off + 1) << 8 )
								 
#define GET_32BIT_WORD(_buff, _off)	(*(((uint8_t *)_buff) + _off)           |		\
                				 *(((uint8_t *)_buff) + _off + 1) << 8  |        \
                				 *(((uint8_t *)_buff) + _off + 2) << 16 |        \
                				 *(((uint8_t *)_buff) + _off +3) << 24)

typedef enum {
	TYPE_UNKNOWN = 0,
	TYPE_FAT12,
	TYPE_FAT16,
	TYPE_FAT32
}fat_type_t;

/*************************************/
typedef struct
{
	uint8_t name[11];
	uint8_t attr;
	uint8_t reserved;
	uint8_t crt_time_tenth;
	uint8_t crt_time[2];
	uint8_t crt_date[2];
	uint8_t lst_acc_date[2];
	uint16_t fst_clus_hi;
	uint8_t wrt_time[2];
	uint8_t wrt_date[2];
	uint16_t fst_clus_lo;
	uint32_t file_size;
} __attribute__((__packed__)) dir_entry_t;

// FAT filesystem dir entry attributes
#define S_FATFS_RDONLY  (0x01) // Read only
#define S_FATFS_HIDDEN  (0x02) // Hidden
#define S_FATFS_SYSTEM  (0x04) // System
#define S_FATFS_VOLUME  (0x08) // Volume label
#define S_FATFS_DIR     (0x10) // Subdirectory
#define S_FATFS_ARCHIVE (0x20) // Needs archiving

#define DIRENTRY_IS_RDONLY(_dentry)		((_dentry)->attr & S_FATFS_RDONLY)
#define DIRENTRY_IS_HIDDEN(_dentry)		((_dentry)->attr & S_FATFS_HIDDEN)
#define DIRENTRY_IS_SYSTEM(_dentry)		((_dentry)->attr & S_FATFS_SYSTEM)
#define DIRENTRY_IS_VOLUME(_dentry)		((_dentry)->attr & S_FATFS_VOLUME)
#define DIRETNRY_IS_DIR(_dentry)			((_dentry)->attr & S_FATFS_DIR)
#define DIRETNRY_IS_ARCHIVE(_dentry)	((_dentry)->attr & S_FATFS_ARCHIVE)
#define DIRETNRY_IS_LFN(_dentry)			((_dentry)->attr & 0xF)

#define DIRENTRY_IS_DELETED(_dentry)	\
	(0xE5 == (uint8_t)(_dentry)->name[0])
	
#define DIRENTRY_IS_ZERO(_dentry)	\
	(0x00 == (uint8_t)(_dentry)->name[0])
	
/* direcotry entry size */
#define DIRENTRY_SIZE			32

// Gets the filename from given dir entry. 
static __INLINE__ void get_dentry_filename(dir_entry_t *dentry, char *name)
{
    int   i     = 0;
    char *cptr  = &(dentry->name[0]);
    char *cname = name;

    while (*cptr != ' ' && i < 8) {
        *cname++ = *cptr++; i++;
    }
    cptr = &(dentry->name[8]);

    if (*cptr != ' ') {
        *cname++ = '.'; 
        i = 0;
        while (*cptr != ' ' && i < 3) {
            *cname++ = *cptr++;
            i++;
        }
    }
    *cname = '\0';
}

static __INLINE__ int fatfs_toupper(int c)
{
    return (('a' <= c) && (c <= 'z')) ? c - 'a' + 'A' : c ;
}

static __INLINE__ int fatfs_strncasecmp( const char *s1, const char *s2, int n )
{
    int ret;

    if (n == 0)
        return 0;

    while (n-- != 0 && fatfs_toupper(*s1) == fatfs_toupper(*s2)) {
        if (n == 0 || *s1 == '\0' || *s2 == '\0')
            break;
        s1++;
        s2++;
    }

    ret = fatfs_toupper(*(unsigned char *) s1) - fatfs_toupper(*(unsigned char *) s2);
    
    return ret;
}

typedef struct {
	block_dev_desc_t *dev_desc;
	int			curr_part;
	uint32_t	part_lbabegin;
	fat_type_t	fattype;
	
	uint16_t	bytes_per_sector;
	uint8_t		sectors_per_cluster;
	
	uint32_t	total_sectors;
	uint16_t	reserved_sectors;
	uint32_t	rootdir_sectors;
	uint32_t	data_sectors;
	
	uint8_t		num_FATs;
	uint32_t	sectors_per_FAT;
	
	uint32_t	first_FAT_sectnum;
	uint32_t	first_data_sectnum;
	uint32_t	rootdir_cluster;
	uint32_t	first_rootdir_sectnum;
}FAT_CONTEXT;

typedef struct {
	FAT_CONTEXT	*context;
	
//#define MAX_LONG_FILENAME	32	
//	char		path[MAX_LONG_FILENAME];
//	char		filename[MAX_LONG_FILENAME];
	uint32_t	filesize;
	uint32_t	offset;
	
//	uint32_t	parentcluster;
	uint32_t	startcluster;
}FAT_FILE;

#define SEEK_SET	0
#define SEEK_END	1
#define SEEK_CUR	2


int fatfs_fopen(FAT_CONTEXT *context, const char* filename, FAT_FILE * f);
void fatfs_fclose(FAT_FILE* f);
int fatfs_fread(FAT_FILE *f, void *dst, uint32_t count);
int fatfs_fseek(FAT_FILE *f, int offset, int origin);


int fatfs_bind_device(block_dev_desc_t *dev_desc, int part_no, FAT_CONTEXT* context);

#endif	/* _FAT_H */
