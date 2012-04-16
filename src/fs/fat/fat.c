/**
 * @file
 * @brief	FAT file operate interface.
 * @author	jimmy.li<lizhengming@innofidei.com>
 * @date	2010/07/04
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <blkdev.h>
#include <fat.h>

#define fat_err		PRINT

static bool_t ReadSector(FAT_CONTEXT *context, uint8_t* buff, uint32_t start_sector, uint32_t sectors)
{
	block_dev_desc_t* dev_desc = context->dev_desc;
	
	if (dev_desc->block_read(dev_desc->dev, start_sector, sectors, buff) != sectors)
		return false;
	
	return true;
}

static bool_t IsValidBootSector(uint8_t *buffer)
{
	if (buffer[0x1fe] != 0x55 || buffer[0x1ff] != 0xaa)
		return false;
	
	if(strncmp((char *)&buffer[54],"FAT",3)==0 /* FAT12/16 */||
			strncmp((char*)&buffer[82], "FAT", 3)==0 /* FAT32 */)
		return true;
	
	return false;
}

static bool_t IsValidMBR(uint8_t* buffer)
{
	if (buffer[0x1fe] != 0x55 || buffer[0x1ff] != 0xaa)
		return false;
	
	if(strncmp((char *)&buffer[54],"FAT",3)==0 /* FAT12/16 */||
			strncmp((char*)&buffer[82], "FAT", 3)==0 /* FAT32 */)
		return false; /* is bootsector */
	
	return true;
}



/*
 * give a cluster number, the sector number of the first sector of that cluster is return.
 */
static int FirstSectorofCluster(FAT_CONTEXT *context, int clusterN)
{
	int sector;
	
	sector = (clusterN - 2) * context->sectors_per_cluster + context->first_data_sectnum;

	return sector;
}

#define FAT_CACHE
static int ClusterContentInFAT(FAT_CONTEXT *context, int clusterN)
{
#ifdef FAT_CACHE
	static int FATSecNum = -1;
	static char SecBuff[512];
#else
	char SecBuff[512];
#endif
	int FATOffset, ThisFATSecNum, ThisFATEntOffset;
	int ret;

	if(context->fattype == TYPE_FAT16){
		FATOffset = clusterN * 2;
	}else if(context->fattype == TYPE_FAT32){
		FATOffset = clusterN * 4;
	}else{
		return -1;
	}
	

	ThisFATSecNum = context->part_lbabegin + context->reserved_sectors + (FATOffset >> 9);
	ThisFATEntOffset = FATOffset & 0x1ff;
/*
	ThisFATSecNum = context->part_lbabegin + context->reserved_sectors + (FATOffset/context->bytes_per_sector);
	ThisFATEntOffset = FATOffset % context->bytes_per_sector;
*/
	
#ifdef FAT_CACHE	
	if(ThisFATSecNum != FATSecNum){
		ReadSector(context, SecBuff, ThisFATSecNum, 1);

		FATSecNum = ThisFATSecNum;
	}
#else
	ReadSector(context, SecBuff, ThisFATSecNum, 1);
#endif
	
	if(context->fattype == TYPE_FAT16){
		return *((uint16_t *) &SecBuff[ThisFATEntOffset]);
	}else if(context->fattype == TYPE_FAT32){
		return (*((uint32_t *) &SecBuff[ThisFATEntOffset])) & 0x0FFFFFFF;
	}
	
	//there is somethin wrong here.
	return -1;
}

static int IsClusterLinkEof(FAT_CONTEXT * context, int FATContent)
{
		int isEOF = 0;
		
		if(context->fattype == TYPE_FAT12){
			if(FATContent>= 0x0FF8)
				isEOF = 1;
		}else if(context->fattype == TYPE_FAT16){
			if(FATContent >= 0xFFF8)
				isEOF = 1;
		}else if(context->fattype == TYPE_FAT32){
			if(FATContent >= 0x0FFFFFF8)
				isEOF = 1;
		}
		
		return isEOF;
}

/*
 * read a sector follow the cluster link
 */
static bool_t ReadSectorFollowCluster(FAT_CONTEXT* context, uint32_t start_cluster, uint32_t sector_idx, uint8_t *buffer)
{
	int cluster = start_cluster;
	int sector;
	int cluster_idx = sector_idx / context->sectors_per_cluster;
	int sector_idx_in_cluster = sector_idx % context->sectors_per_cluster;
	int i;
	
	for (i = 0; i < cluster_idx; i++){
		cluster = ClusterContentInFAT(context, cluster);	//get next cluster
		if (IsClusterLinkEof(context, cluster)){
			return false;
		}
	}
	sector = FirstSectorofCluster(context, cluster);
	sector += sector_idx_in_cluster;
	
	if (ReadSector(context, buffer, sector, 1) == false)
		return false;

	return true;
}

/*
 *if found the specific dir_entry, then return zero, otherwise return -1.
 */
static int SearchRootDirEntry(FAT_CONTEXT *context, const char* filename, dir_entry_t *dirent)
{
	uint8_t buff[512];
	int found = 0;
	dir_entry_t *p;
	
	if(context->fattype == TYPE_FAT32){
		int cluster = context->rootdir_cluster;
		
		do {
			int i, j;
			int sec = FirstSectorofCluster(context, cluster);
			
			for(i=0; i<context->sectors_per_cluster; i++, sec++){
				ReadSector(context, buff, sec, 1);
				for(j=0; j<context->bytes_per_sector/DIRENTRY_SIZE; j++){
					p = &((dir_entry_t*)buff)[j];
					
					if(DIRENTRY_IS_DELETED(p)){
						continue;
					}
				
					if(DIRENTRY_IS_ZERO(p)){
						goto out;
					}
				
					if(!DIRETNRY_IS_LFN(p) && !DIRETNRY_IS_DIR(p) && !DIRENTRY_IS_VOLUME(p)){
						char short_name[12+1];
						int res;
						get_dentry_filename(p, short_name);
						res = fatfs_strncasecmp(filename, short_name, 12);
						if(res == 0){
							found = 1;
							memcpy(dirent, p, sizeof(dir_entry_t));
							goto out;
						}
						continue;
					}
				}//end for(j=...
				
			}	// end for(i=..
			
			if( IsClusterLinkEof(context, ClusterContentInFAT(context, cluster)) )
				goto out;
	
			cluster = ClusterContentInFAT(context, cluster);	//get next cluster
		}while(1);
			
	}else{	//FAT16
		uint32_t rootdir_sec_last = context->first_rootdir_sectnum + context->rootdir_sectors - 1;
		uint32_t sec = context->first_rootdir_sectnum;
		
		while(sec < rootdir_sec_last){
			int i;
			ReadSector(context, buff, sec++, 1);
			
			for(i=0; i<context->bytes_per_sector/DIRENTRY_SIZE; i++){
				p = &((dir_entry_t*)buff)[i];
				
				if(DIRENTRY_IS_DELETED(p)){
					continue;
				}
				
				if(DIRENTRY_IS_ZERO(p)){
					goto out;
				}
				
				if(!DIRETNRY_IS_LFN(p) && !DIRETNRY_IS_DIR(p) && !DIRENTRY_IS_VOLUME(p)){
					char short_name[12+1];
					int res;
					get_dentry_filename(p, short_name);
					res = fatfs_strncasecmp(filename, short_name, 12);
					if(res==0){
						found = 1;
						memcpy(dirent, p, sizeof(dir_entry_t));
						goto out;
					}
					continue;
					
				}
			}	// end for
			
		}//end while
	}
	
out:
	return (found ? 0 : -1);
}


/** 
 * @brief bind a block device with FAT.
 *
 * @param[in] dev_desc - block device descriptor 
 * @param[in] part_no - partition number to be operate (currently only operate the first partition of the disk). 
 * @param[in, out] context	- the FAT information of bind disk partition.
 * 
 * @return	- if success return 0, otherwize return -1
 */
int fatfs_bind_device(block_dev_desc_t *dev_desc, int part_no, FAT_CONTEXT *context)
{
	uint8_t	buffer[512];
	
	context->dev_desc = dev_desc;
	
	if (ReadSector(context, buffer, 0, 1) == false)
		return -1;

	
	if (buffer[0x1fe] != 0x55 || buffer[0x1ff] != 0xaa)
		return -1;
	
	// check if MBR exists
	if (IsValidMBR(buffer)){
		//TODO
		context->part_lbabegin = GET_32BIT_WORD(buffer, 0x1c6);
		context->curr_part = 1;	//part_no;	//no used, we always bind the partition 1.
		
		// read boot sector
		ReadSector(context, buffer, context->part_lbabegin, 1);
		
	}else {
		context->part_lbabegin = 0;
		context->curr_part = 1;
	}
	
	// now we are in boot sector
	if (IsValidBootSector(buffer)) {
		uint32_t BPB_FATSz32, BPB_TotSec32;
		uint16_t BPB_FATSz16, BPB_TotSec16;
		uint32_t CountofClusters;
		
		context->bytes_per_sector = GET_16BIT_WORD(buffer, 11);
		//make sure there are 512bytes per sector
		if (context->bytes_per_sector != 0x200){
			fat_err("only support 512 bytes sector.\n");
			return -1;
		}
		context->sectors_per_cluster = GET_BYTE(buffer, 13);
		
		context->reserved_sectors = GET_16BIT_WORD(buffer, 14);
		context->rootdir_sectors = GET_16BIT_WORD(buffer, 17) >> 4;	 // ((x) * 32 / 512)
		context->num_FATs = GET_BYTE(buffer, 16);
		BPB_TotSec16 = GET_16BIT_WORD(buffer, 19);
		BPB_TotSec32 = GET_32BIT_WORD(buffer, 32);
		context->total_sectors = (BPB_TotSec16 == 0) ? BPB_TotSec32 : BPB_TotSec16;
		BPB_FATSz16 = GET_16BIT_WORD(buffer, 22);
		BPB_FATSz32 = GET_32BIT_WORD(buffer, 36);
		context->sectors_per_FAT = (BPB_FATSz16 == 0) ? BPB_FATSz32 : BPB_FATSz16;
		context->data_sectors = context->total_sectors - context->reserved_sectors - 
				(context->num_FATs * context->sectors_per_FAT) - context->rootdir_sectors;
		
		context->first_FAT_sectnum = context->part_lbabegin + context->reserved_sectors;
		context->first_data_sectnum = context->part_lbabegin + context->reserved_sectors + 
				(context->num_FATs * context->sectors_per_FAT) +
				context->rootdir_sectors;

		// FAT type determination
		CountofClusters = context->data_sectors / context->sectors_per_cluster;
		if (CountofClusters < 4095) {
			// partition is FAT12
			context->fattype = TYPE_FAT12;
			//currently we not support FAT12
			fat_err("not support FAT12\n");
			return -1;
			
		} else if (CountofClusters < 65525) {
			// partition is FAT16
			context->fattype = TYPE_FAT16;
			
			context->rootdir_cluster = 0;		//no used
			context->first_rootdir_sectnum = context->part_lbabegin + context->reserved_sectors + 
					(context->num_FATs * context->sectors_per_FAT);
		} else {
			// partition is FAT32
			context->fattype = TYPE_FAT32;
			
			context->rootdir_cluster = GET_32BIT_WORD(buffer, 44);
			context->first_rootdir_sectnum = FirstSectorofCluster(context, context->rootdir_cluster);
		}
		
	}else {
		// is not a valid boot sector
		return -1;
	}

	return 0;	
}


/** 
 * @brief open a specific file
 *
 * @param[in] context - the context return by fatfs_bind_device.
 * @param[in] filename	- Pointer to a null-terminated string that specifies the name of the file.
 * @param[out] f - the handle to an open file.
 * 
 * @return	- if success return 0, otherwize return a negative number.
 */
int fatfs_fopen(FAT_CONTEXT *context, const char* filename, FAT_FILE* f)
{
	dir_entry_t dirent;
	
	//currently, we only support open file under root directory
	//TODO
	if (SearchRootDirEntry(context, filename, &dirent) != 0) {
		return -1;
	}
	 
	f->filesize = dirent.file_size;
	f->offset = 0;
	f->startcluster = (((uint32_t)dirent.fst_clus_hi)<<16) + dirent.fst_clus_lo;
	
	f->context = context;
	return 0;
}

/** 
 * @brief close an open file
 *
 * @param[in] f - the handle to an open file.
 * 
 * @return none.
 */
void fatfs_fclose(FAT_FILE* f)
{
}

/** 
 * @brief reads data from a file.
 *
 * @param[in] f - the handle to an open file. 
 * @param[out] dst - pointer to the buffer that receives the data read from the file. 
 * @param[in] count	- number of bytes to be read from the file.
 * 
 * @return	- if success return actual read bytes.otherwize return -1
 */
int fatfs_fread(FAT_FILE *f, void *dst, uint32_t count)
{
	FAT_CONTEXT* context = f->context;
	uint32_t sector, offset;
	uint32_t totsectors, i;
	uint32_t bytesRead;
	uint32_t thisReadCount;
	
	// limit to file size
	if (f->offset + count > f->filesize)
		count = f->filesize - f->offset;
	
	// Calculations for file position
	sector = f->offset >> 9;
	offset = f->offset - (sector << 9);
	
	// Calculate how many sectors this is
	totsectors = (count + offset) >> 9;

	// Take into account partial sector read
	if ((count+offset) & 0x1ff)
		totsectors++;

	bytesRead = 0;
	for (i=0; i<totsectors; i++) {
		if (i==0 && offset) {	// first sector, partial
			uint8_t buff[512];
			if (ReadSectorFollowCluster(context, f->startcluster, (sector+i), buff)) {
				thisReadCount = MIN(512 - offset, count);
				memcpy(dst, buff+offset, thisReadCount);
			}else
				return bytesRead;
			
		} else if(bytesRead + 512 > count){	//last sector, remainder
			uint8_t buff[512];
			if (ReadSectorFollowCluster(context, f->startcluster, (sector+i), buff)) {
				thisReadCount = count - bytesRead;
				memcpy(dst, buff, thisReadCount);
			}else
				return bytesRead;
			
		} else {	//full sector
			if (ReadSectorFollowCluster(context, f->startcluster, (sector+i), dst)) {
				thisReadCount = 512; 
			}else
				return bytesRead;
		}
		
		dst = (uint8_t*)dst + thisReadCount;
		bytesRead += thisReadCount;
		f->offset += thisReadCount;		//update file pointer
	}
	
	return bytesRead;
}

/**
 * @brief set file point position
 * 
 * @param[in] f	- the handle to an open file.
 * @param[in] offset	- specifies the number of bytes to move the file pointer.
 * @param[in] origin	- starting point for the file pointer move, the following table shows possible values for this parameter.
 *			SEEK_SET	0	indicates that the starting point is zero or the beginning of the file
 *			SEEK_END	1	indicates that the starting point is the current EOF position.
 *			SEEK_CUR	2	indicates that the starting point is the current value of the file pointer.	
 * 
 * @return		-	if success return the new file pointer,otherwize return -1.
 */
int fatfs_fseek(FAT_FILE *f, int offset, int origin)
{
	int pos;
	
	if (origin == SEEK_SET) {
		pos = 0;		
	} else if (origin == SEEK_CUR) {
		pos = f->offset;
	} else if (origin == SEEK_END) {
		pos = f->filesize;
	}
	
	if (pos + offset < 0)
		f->offset = 0;
	else if (pos + offset > f->filesize)
		f->offset = f->filesize;
	else
		f->offset = pos + offset;

	
	return f->offset;
}

