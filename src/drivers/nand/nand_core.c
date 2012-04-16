/**
 * @file
 * @brief	NAND Driver
 * @date	2011/04/06
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <io.h>
#include <bitops.h>
#include "nand.h"

static int probe_nand_chip(struct nand_chip* chip)
{
	int i;
	struct nand_controller* nand_ctrl = chip->master;
	struct nand_flash_dev* type = NULL;
	int maf_idx;

	nand_ctrl->select_chip(chip, 1);

	nand_ctrl->cmdfunc(chip, NAND_CMMD_READID, 0x00, -1);

	chip->device_id = nand_ctrl->read_byte(nand_ctrl);
	chip->vendor_id = nand_ctrl->read_byte(nand_ctrl);

	for (i=0; nand_flash_ids[i].name != NULL; i++) {
		if (chip->device_id == nand_flash_ids[i].id) {
			type = &nand_flash_ids[i];
			break;
		}
	}
	
	if (!type)
		return -1;
	
	chip->chipsize = type->chipsize << 20;
	chip->options = type->options;

	// newer chip have all the information in additional id bytes
	if (!type->pagesize) {
		int extid;
		// The 3rd id byte holds MLC / multichip data
		extid = nand_ctrl->read_byte(nand_ctrl);
		// The 4th id byte is the important one
		extid = nand_ctrl->read_byte(nand_ctrl);

		// calc pagesize
		chip->pagesize = 1024 << (extid & 0x3);
		extid >>= 2;

		// calc oobsize
		chip->oobsize = (8 << (extid & 0x1)) * (chip->pagesize >> 9);
		extid >>= 2;

		// calc blocksize
		chip->erasesize = (64 * 1024) << (extid & 0x3);
		extid >>= 2;

		// buswidth
		chip->options &= ~ NAND_BUSWIDTH_16;
		chip->options |= (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
	} else {
		chip->erasesize = type->erasesize;
		chip->pagesize = type->pagesize;
		chip->oobsize = type->pagesize >> 5;	// div 32
	}
	
	nand_ctrl->select_chip(chip, 0);

	// Try to identify manfacturer
	for (maf_idx=0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == chip->vendor_id)
			break;
	}

	PRINT("NAND device is detected :  "
			"Vendor ID = 0x%02x, Device ID = 0x%02x "
			"(%s %s)\n",
			chip->vendor_id, chip->device_id,
			nand_manuf_ids[maf_idx].name, type->name);
	
	return 0;
}

int nand_controller_register(struct nand_controller* nand_ctrl)
{
	struct nand_chip* chip;
	int i;
	int ret;

	for (i=0; i<nand_ctrl->maxchips; i++) {
		chip = &nand_ctrl->chip[i];
		if (chip == NULL)
			break;
		
		chip->master = nand_ctrl;
		chip->chip_id = i;

		ret = probe_nand_chip(chip);
		if (ret < 0) {
			break;
		}

		chip->page_shift = ffs(chip->pagesize) - 1;
		chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;
		chip->erase_shift = ffs(chip->erasesize) - 1;
		chip->chip_shift = ffs(chip->chipsize) - 1;
		chip->bbt_erase_shift = chip->phys_erase_shift = ffs(chip->erasesize) - 1;
		chip->badblockpos = chip->pagesize > 512 ? NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;
	}
	return 0;
}

extern int cpu_nand_init(void) __attribute__  ((weak));
extern void cpu_nand_exit(void) __attribute__  ((weak));

int nand_initialize(void)
{
	cpu_nand_init();
}

void nand_deinitialize(void)
{
	cpu_nand_exit();
}
