/**
 * @file
 * @brief	load file from SD/MMC card
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>
#include <blkdev.h>
#include <mmc.h>
#include <fat.h>

static int do_load_file(block_dev_desc_t* blkdev, const char* filename, unsigned long destaddr)
{
	FAT_CONTEXT	CONTEXT;
	FAT_FILE	FILE;
	int ret;
	int filesize;

	ret = fatfs_bind_device(blkdev, 1, &CONTEXT);
	if (ret) {
		PRINT("fatfs_bind_device failed\n");
		return -1;
	}
	ret = fatfs_fopen(&CONTEXT, filename, &FILE);
	if (ret) {
		PRINT("fatfs_fopen failed\n");
		return -1;
	}

	filesize = fatfs_fseek(&FILE, 0, SEEK_END);
	PRINT("filesize = %d\n", filesize);
	fatfs_fseek(&FILE, 0, SEEK_SET);

	ret = fatfs_fread(&FILE, (void*)destaddr, filesize);
	if (ret != filesize) {
		PRINT("fatfs_fread failed, ret = %d\n", ret);
		ret = -1;
		goto out;
	}
	ret = 0;

out:
	fatfs_fclose(&FILE);

	return ret;
}

static void show_help(void)
{
	PRINT("sdload filename destaddr\n");
}

static int cmd_sdload(int argc, char** argv)
{
	struct mmc* mmc;
	int ret;
	char* filename;
	unsigned long destaddr;

	if (argc != 3) {
		show_help();
		return 0;
	}

	filename = argv[1];
	destaddr = simple_strtoul(argv[2], NULL, 0); 

	ret = mmc_initialize();
	if (ret) {
		PRINT("mmc_initialize failed\n");	
		goto out;
	}
	mmc = get_active_mmc();
	if (!mmc) {	// no mmc card registered
		ret = -1;
		PRINT("get_active_mmc failed\n");	
		goto out;
	}

	ret = mmc_init(mmc);
	if (ret) {
		PRINT("mmc_init failed\n");	
		goto out;
	}
	ret = do_load_file(&mmc->block_dev, filename, destaddr);
out:
	mmc_deinitialize();
	return  (ret ? -1 : 0);
}

INSTALL_CMD(
	sdload, 
	cmd_sdload, 
	"load file from SD/MMC card"
	);

