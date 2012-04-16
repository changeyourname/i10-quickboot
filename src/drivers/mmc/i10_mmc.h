#ifndef _I10_MMC_H_
#define _I10_MMC_H_

/* 
 * SD Controller registers 
 */
#define MMC_CTL_REG			0x00
#define		SDIO_READ_WAIT_EN		(1<<10)
#define		SDIO_INT_EN				(1<<9)
#define		SDIO_MODE_EN			(1<<8)
#define		DATA_WIDTH4				(1<<7)
#define		HIGH_SPEED				(1<<6)


#define	MMC_IO_REG			0x04
#define 	MMC_IO_AUTO_8NULLCLK_EN	(1<<7)
#define 	MMC_IO_AUTO_RCVRSP		(1<<6)
#define 	MMC_IO_SND_8NULLCLK		(1<<5)
#define 	MMC_IO_RSP_136BIT		(1<<4)
#define 	MMC_IO_SNDCMD			(0<<3)
#define 	MMC_IO_RCVRSP			(1<<3)
#define 	MMC_IO_AUTO_SND_EN      (1<<2)
#define 	MMC_IO_AUTO_SND_DIS		(0<<2)
#define 	MMC_IO_DATA_READ		(1<<1)
#define 	MMC_IO_DATA_WRITE		(0<<1)
#define 	MMC_IO_SET_AUTO_TRANS	(1<<0)

#define MMC_BYTECNT_REG		0x08
#define MMC_TRBLOCKCNT_REG	0x0c	//[RO]
#define MMC_CRCCTL_REG		0x10
#define		CMD_CRC_EN				(1<<7)
#define		DAT_CRC_EN				(1<<6)
#define		AUTOCHECK_CRCST_EN		(1<<5)

#define MMC_CMDCRC_REG		0x14		//[RO]
#define MMC_DATCRCL_REG		0x18		//[RO]
#define MMC_DATCRCH_REG		0x1c		//[RO]
#define MMC_PORT_REG		0x20
#define MMC_INTMASK_REG		0x24
#define MMC_INTCLR_REG		0x28
#define		MMC_INTCLR_CMD_DONE			(1<<0)
#define		MMC_INTCLR_DAT_DONE			(1<<1)
#define		MMC_INTCLR_DAT_CRC_ERR		(1<<2)
#define		MMC_INTCLR_CMD_CRC_ERR		(1<<3)
#define		MMC_INTCLR_MBLOCK_DONE		(1<<4)
#define		MMC_INTCLR_MBLOCK_TIMEOUT	(1<<5)
#define		MMC_INTCLR_CMD_RESP_NCR_TIMEOUT	(1<<6)
#define		MMC_INTCLR_CRC_STATUS_TOKEN_ERR	(1<<7)
#define		MMC_INTCLR_SDIO				(1<<8)

#define MMC_CARDSEL_REG		0x2c
#define MMC_SIG_REG			0x30

#define MMC_IOMBCTL_REG		0x34	
#define		MMC_IOMBCTL_NAC_TIMEOUT_SCALE1us		(0<<6)
#define		MMC_IOMBCTL_NAC_TIMEOUT_SCALE100us		(1<<6)
#define		MMC_IOMBCTL_NAC_TIMEOUT_SCALE10ms		(2<<6)
#define		MMC_IOMBCTL_NAC_TIMEOUT_SCALE1s			(3<<6)
#define		MMC_IOMBCTL_BUSY_TIMEOUT_SCALE1us		(0<<4)
#define		MMC_IOMBCTL_BUSY_TIMEOUT_SCALE100us		(1<<4)
#define		MMC_IOMBCTL_BUSY_TIMEOUT_SCALE10ms		(2<<4)
#define		MMC_IOMBCTL_BUSY_TIMEOUT_SCALE1s		(3<<4)
#define		MMC_IOMBCTL_CLK_PUSH_FALLING			(1<<3)
#define		MMC_IOMBCTL_CLK_PUSH_RISING				(0<<3)
#define		MMC_IOMBCTL_PORT_AUTO					(1<<2)
#define		MMC_IOMBCTL_DATA_READ					(1<<1)
#define		MMC_IOMBCTL_DATA_WRITE					(0<<1)
#define		MMC_IOMBCTL_AUTO_MULTI_BLOCK			(1<<0)

#define MMC_BLOCKCNT_REG	0x38
#define MMC_TIMEOUTCNT_REG	0x3c
#define MMC_CMDBUF0_REG		0x40
#define MMC_CMDBUF1_REG		0x44
#define MMC_CMDBUF2_REG		0x48
#define MMC_CMDBUF3_REG		0x4c
#define MMC_CMDBUF4_REG		0x50
#define MMC_CMDBUF5_REG		0x54
#define MMC_CMDBUF6_REG		0x58
#define MMC_CMDBUF7_REG		0x5c
#define MMC_CMDBUF8_REG		0x60
#define MMC_CMDBUF9_REG		0x64
#define MMC_CMDBUF10_REG	0x68
#define MMC_CMDBUF11_REG	0x6c
#define MMC_CMDBUF12_REG	0x70
#define MMC_CMDBUF13_REG	0x74
#define MMC_CMDBUF14_REG	0x78
#define MMC_CMDBUF15_REG	0x7c
#define MMC_CMDBUF_REG(x)		(0x40 + (i)<<2)

#define MMC_BUFCTL_REG		0x80
#define		MMC_BUFCTL_FIFO_FLUSH_EN		(1<<15)
#define		MMC_BUFCTL_DMA_REQ_MASK			(1<<14)
#define		MMC_BUFCTL_FIFO_STATUS_INI_EN	(1<<12)
#define		MMC_BUFCTL_DIR_WRITE			(1<<11)
#define		MMC_BUFCTL_DIR_READ				(0<<11)
#define		MMC_BUFCTL_DMA_EN				(1<<10)
#define		MMC_BUFCTL_DMA_DIS				(0<<10)
#define		MMC_BUFCTL_WATERMARK(x)			((x)<<2)
#define		MMC_BUFCTL_DATA_EMPTY			(1<<1)
#define		MMC_BUFCTL_DATA_FULL			(1<<0)

#define MMC_DATBUF_REG		0x100

#endif /*_I10_MMC_H_*/
