#ifndef _NAND_H__
#define _NAND_H__

struct nand_controller;

struct nand_chip {
	struct nand_controller *master;
	int			chip_id;

	uint32_t	device_id;
	uint32_t	vendor_id;

	uint32_t	chipsize;
	uint32_t	erasesize;
	uint32_t	pagesize;
	uint32_t	oobsize;

	uint32_t	options;

	uint32_t	pagemask;
	uint32_t	page_shift;
	uint32_t	erase_shift;
	uint32_t	chip_shift;
	uint32_t	bbt_erase_shift;
	uint32_t	phys_erase_shift;

	uint32_t	badblockpos;	// bad block position
};

struct nand_controller {
	int		maxchips;
	struct	nand_chip	*chip;

	void		(*select_chip)(struct nand_chip* chip, int isCE);
	uint8_t		(*read_byte)(struct nand_controller *nand_ctrl);
	void		(*cmdfunc)(struct nand_chip* chip, unsigned command, int column, int page_addr);
};

#define NAND_SMALL_BADBLOCK_POS		(5)
#define NAND_LARGE_BADBLOCK_POS		(5)

/* Option constants */
#define NAND_NO_AUTOINCR	0x00000001
#define NAND_BUSWIDTH_16	0x00000002
#define NAND_NO_PADDING		0x00000004
#define NAND_CACHEPRG		0x00000008
#define NAND_COPYBACK		0x00000010
#define NAND_IS_AND		    0x00000020
#define NAND_4PAGE_ARRAY	0x00000040
#define BBT_AUTO_REFRESH	0x00000080
#define NAND_NO_READRDY		0x00000100
#define NAND_NO_SUBPAGE_WRITE	0x00000200

#define NAND_SAMSUNG_LP_OPTIONS \
	(NAND_NO_PADDING | NAND_CACHEPRG | NAND_COPYBACK)

// standard nand commands
#define NAND_CMMD_READ0			0
#define NAND_CMMD_READ1			1
#define NAND_CMMD_RNDOUT		5
#define NAND_CMMD_PAGEPROG		0x10
#define NAND_CMMD_READOOB		0x50
#define NAND_CMMD_ERASE1		0x60
#define NAND_CMMD_STATUS		0x70
#define NAND_CMMD_STATUS_MULTI	0x71
#define NAND_CMMD_SEQIN			0x80
#define NAND_CMMD_RNDIN			0x85
#define NAND_CMMD_READID		0x90
#define NAND_CMMD_ERASE2		0xd0
#define NAND_CMMD_RESET			0xff

/*  
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_TOSHIBA    0x98
#define NAND_MFR_SAMSUNG    0xec
#define NAND_MFR_FUJITSU    0x04
#define NAND_MFR_NATIONAL   0x8f
#define NAND_MFR_RENESAS    0x07
#define NAND_MFR_STMICRO    0x20
#define NAND_MFR_HYNIX      0xad
#define NAND_MFR_MICRON     0x2c
#define NAND_MFR_AMD        0x01

/*
 * NAND Flash Device ID Structure
 */
struct nand_flash_dev {
    char *name;
	int id;
	unsigned long pagesize;
	unsigned long chipsize;
	unsigned long erasesize;
	unsigned long options;
};

/*
 * NAND Flash Manufacturer ID Structure
 */
struct nand_manufacturers {
    int id;
	char * name;
};

extern struct nand_flash_dev nand_flash_ids[];
extern struct nand_manufacturers nand_manuf_ids[];
#endif	//_NAND_H__
