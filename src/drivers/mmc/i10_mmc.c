/**
 * @file
 * @brief	I10 sd/mmc/sdio host controller driver
 * @date	2010/07/01
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <io.h>
#include <mmc.h>
#include <hardware.h>
#include "i10_mmc.h"

#define sdi_info		//PRINT
#define sdi_err			PRINT
#define sdi_dbg			PRINT

typedef struct {
	struct mmc	mmc;		//first element
	void __iomem* reg_base;

	unsigned int	blocks;	//remaining PIO blocks
	void*			buffer;
}sdi_host_t;

static sdi_host_t sdi_host;

static __INLINE__ uint32_t sdi_rd_regl(sdi_host_t* host, unsigned offset)
{
	return __raw_readl(host->reg_base + offset);
}

static __INLINE__ void sdi_wr_regl(sdi_host_t *host, unsigned offset, uint32_t val)
{
	__raw_writel(val, host->reg_base + offset);
}

static void sdi_prepare_data(struct mmc *mmc, struct mmc_data *data)
{
	sdi_host_t *host = (sdi_host_t*)mmc;
	uint32_t val = 0;

    if (!data) {
		sdi_wr_regl(host, MMC_BYTECNT_REG, 0);
		sdi_wr_regl(host, MMC_BLOCKCNT_REG, 0);
		sdi_wr_regl(host, MMC_BUFCTL_REG, 0);
        return;
    }

	host->blocks = data->blocks;
	host->buffer = data->buff.dest;

	sdi_wr_regl(host, MMC_BYTECNT_REG, data->blocksize);
	sdi_wr_regl(host, MMC_BLOCKCNT_REG, data->blocks);

	if (data->flags & MMC_DATA_READ) {
		val = MMC_BUFCTL_DIR_READ | MMC_BUFCTL_DMA_DIS;
		sdi_wr_regl(host, MMC_IOMBCTL_REG, MMC_IOMBCTL_NAC_TIMEOUT_SCALE1s | MMC_IOMBCTL_BUSY_TIMEOUT_SCALE1s | MMC_IOMBCTL_DATA_READ);
	}else{
		val = MMC_BUFCTL_DIR_WRITE | MMC_BUFCTL_DMA_DIS;
		sdi_wr_regl(host, MMC_IOMBCTL_REG, MMC_IOMBCTL_NAC_TIMEOUT_SCALE1s | MMC_IOMBCTL_BUSY_TIMEOUT_SCALE1s | MMC_IOMBCTL_DATA_WRITE);
	}

	sdi_wr_regl(host, MMC_BUFCTL_REG, val);
} 


static void sdi_receive_response(struct mmc *mmc, struct mmc_cmd *cmd)
{
    int i;
    sdi_host_t* host = (sdi_host_t*)mmc;

    // get response data
    if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			unsigned int response[16];
			for (i=0; i<16; i++)
				response[i] = sdi_rd_regl(host, MMC_CMDBUF_REG(i));
			
			cmd->response[0] = (response[14]<<24) | (response[13]<<16) | (response[12]<<8) | (response[11]);	/* bit127 - 96 */

			cmd->response[1] = (response[10]<<24) | (response[9]<<16) | (response[8]<<8) | (response[7]);	/* bit95 - 64 */

			cmd->response[2] = (response[6]<<24) | (response[5]<<16) | (response[4]<<8) | (response[3]);	/* bit63 - 32 */

			cmd->response[3] = (response[2]<<24) | (response[1]<<16) | (response[0]<<8) | 
								( ((sdi_rd_regl(host, MMC_CMDCRC_REG) & 0x7f)<<1) | 0x01 );				/* bit31 - 0 */

		} else {
			unsigned int response[6];
			for (i=0; i<5; i++)
				response[i] = sdi_rd_regl(host, MMC_CMDBUF_REG(i));

			cmd->response[0] = (response[3]<<24 | response[2]<<16 | response[1]<<8 | response[0]);	/* bit39 - 8 */
			cmd->response[1] = 0;
			cmd->response[2] = 0;
			cmd->response[3] = 0;
		}
	}

}

static int sdi_finish_cmd(struct mmc* mmc, struct mmc_cmd *cmd) 
{
	uint32_t intmask;
	unsigned long timeout = 1000;
	sdi_host_t *host = (sdi_host_t*)mmc;
	int ret;
	uint32_t intclr;

	while (timeout--) {
		intclr = sdi_rd_regl(host, MMC_INTCLR_REG);
		if (intclr & MMC_INTCLR_CMD_DONE)
			return 0;
	}

	return TIMEOUT;
}

static void sdi_read_block_pio(struct mmc *mmc, struct mmc_data *data)
{
    sdi_host_t *host = (sdi_host_t*)mmc;
	int totsize = data->blocksize;

	if (!((uint32_t)(host->buffer) & 0x3)) {	// dest buffer address 4-byte align
		int m = (totsize >> 2);
		int i;
    	uint32_t *buffer = host->buffer;
		for (i=0; i<m; i++)
			*(buffer++) = sdi_rd_regl(host, MMC_DATBUF_REG);

		totsize -= (m<<2);
		host->buffer = (void*)buffer;
	}

	while (totsize) {
		uint32_t tmp = sdi_rd_regl(host, MMC_DATBUF_REG);
		int size = MIN(4, totsize);
		totsize -= size;
		while (size){
			*((char*)(host->buffer)++) = tmp & 0xff;
			tmp >>= 8;
			size--;
		}
	}
}


static void sdi_transfer_pio(struct mmc *mmc, struct mmc_data *data)
{
    uint32_t mask;
    sdi_host_t* host = (sdi_host_t*)mmc;
    
	if (host->blocks == 0)
		return;
	
	if (data->flags & MMC_DATA_READ)
		sdi_read_block_pio(mmc, data);
	else {
		//sdi_err("block write not implemention!\n");
	}

	host->blocks--;
	if (host->blocks == 0)
		return;
}

static int sdi_finish_data(struct mmc* mmc, struct mmc_data *data)
{
    uint32_t intmask;
    unsigned long timeout = 1000;
    sdi_host_t* host = (sdi_host_t*)mmc;
   	uint32_t intclr;
	uint32_t tmpval;

	if (data->flags & MMC_DATA_WRITE) {
		sdi_err("block write not implemention!\n");
		//TODO fill write FIFO
		return UNUSABLE_ERR; 
	}

	tmpval = sdi_rd_regl(host, MMC_IOMBCTL_REG);	
	tmpval |= MMC_IOMBCTL_AUTO_MULTI_BLOCK;
	sdi_wr_regl(host, MMC_IOMBCTL_REG, tmpval);

    while (timeout--) {
		intclr = sdi_rd_regl(host, MMC_INTCLR_REG);
		sdi_wr_regl(host, MMC_INTCLR_REG, intclr);	//clear interrupt

		if (intclr & MMC_INTCLR_DAT_DONE) {
			sdi_transfer_pio(mmc, data);
		}
	
		if (intclr & MMC_INTCLR_MBLOCK_DONE) {
			return 0;
		}
    }
   
    return TIMEOUT;
}

static int sdi_send_request(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    unsigned long flags;
    uint32_t mask=0;
    unsigned long timeout;
    int ret;
    sdi_host_t* host = (sdi_host_t*)mmc;
	uint32_t val = 0;

    /* Wait max 10 ms */
    timeout = 10;

    sdi_prepare_data(mmc, data);

    switch (cmd->resp_type) {
    case MMC_RSP_NONE:
		val = (MMC_IO_AUTO_8NULLCLK_EN | MMC_IO_SNDCMD | MMC_IO_AUTO_SND_EN);
        break;
    case MMC_RSP_R1:
//    case MMC_RSP_R5:
//    case MMC_RSP_R6:
//    case MMC_RSP_R7:
		val = (MMC_IO_AUTO_8NULLCLK_EN | MMC_IO_AUTO_RCVRSP | MMC_IO_SNDCMD | MMC_IO_AUTO_SND_EN);
        break;
    case MMC_RSP_R1b:
//    case MMC_RSP_R5b:
		val = (MMC_IO_AUTO_8NULLCLK_EN | MMC_IO_AUTO_RCVRSP | MMC_IO_SNDCMD | MMC_IO_AUTO_SND_EN);
        break;
    case MMC_RSP_R2:
		val = (MMC_IO_AUTO_8NULLCLK_EN | MMC_IO_AUTO_RCVRSP | MMC_IO_RSP_136BIT | MMC_IO_SNDCMD | MMC_IO_AUTO_SND_EN);
        break;
    case MMC_RSP_R3:
//    case MMC_RSP_R4:
		val = (MMC_IO_AUTO_8NULLCLK_EN | MMC_IO_AUTO_RCVRSP | MMC_IO_SNDCMD | MMC_IO_AUTO_SND_EN);
        break;
    default:
		val = (MMC_IO_AUTO_8NULLCLK_EN | MMC_IO_SNDCMD | MMC_IO_AUTO_SND_EN);
        break;
   }

	//set argument
	sdi_wr_regl(host, MMC_CMDBUF0_REG, (cmd->cmdarg & 0xff));		// bit 15:8
	sdi_wr_regl(host, MMC_CMDBUF1_REG, ((cmd->cmdarg>>8) & 0xff));	// bit 23:16
	sdi_wr_regl(host, MMC_CMDBUF2_REG, ((cmd->cmdarg>>16) & 0xff));	// bit 31:24
	sdi_wr_regl(host, MMC_CMDBUF3_REG, ((cmd->cmdarg>>24) & 0xff));	// bit 39:32

	// set command index
	sdi_wr_regl(host, MMC_CMDBUF4_REG, (0x40 | cmd->cmdidx));	// bit 47: 40, including start bit ('0'), transmission bit('1'), command index.

	// set command timeout ??

	// send command
	sdi_wr_regl(host, MMC_IO_REG, val);

	while(sdi_rd_regl(host, MMC_IO_REG) & MMC_IO_AUTO_SND_EN);	// wait clear

    ret = sdi_finish_cmd(mmc,cmd);
    if (ret)
        return ret;

    if (data) {
        ret = sdi_finish_data(mmc,data);
    }

    return ret;
}

static void set_buswidth(struct mmc* mmc, int buswidth)
{
	sdi_host_t *host = (sdi_host_t*)mmc;
	//TODO
}

static int set_clock(struct mmc* mmc, unsigned int clock)
{
	return 0;    
}

static void sdi_set_ios(struct mmc *mmc)
{
	set_buswidth(mmc, mmc->bus_width);
	set_clock(mmc, mmc->clock);
}

static int sdi_init(struct mmc *mmc)
{
    uint32_t intmask;
    sdi_host_t *host = (sdi_host_t*)mmc;
   

	return 0;
}


static void sdi_startup(void)
{
	//TODO
	// turn on controller
}

static void sdi_shutdown(void)
{
	//TODO
	// turn off controller
}

int cpu_mmc_init(void)
{
	struct mmc *mmc = &sdi_host.mmc;
	void __iomem* reg_base;
	
	reg_base = sdi_host.reg_base = (void __iomem *)SDIO0_BASE_ADDR;
	mmc->send_cmd = sdi_send_request;
	mmc->set_ios = sdi_set_ios;
	mmc->init = sdi_init;
	
	sdi_startup();
	
	mmc->f_max = 24000000;
	mmc->f_min = 400000;
	
	mmc->voltages |= MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 |MMC_VDD_32_33;
	
	mmc->host_caps = MMC_MODE_4BIT;
	mmc->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
	
	mmc_register(mmc);
	
	return 0;
}

void cpu_mmc_exit(void)
{
	sdi_shutdown();
}
