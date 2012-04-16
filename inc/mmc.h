#ifndef _MMC_H
#define _MMC_H
#include <blkdev.h>

/* Command Index */
#define MMC_CMD_GO_IDLE_STATE       0
#define MMC_CMD_SEND_OP_COND        1
#define MMC_CMD_ALL_SEND_CID        2
#define MMC_CMD_SET_RELATIVE_ADDR   3
#define MMC_CMD_SET_DSR				4
#define MMC_CMD_SWITCH				6
#define MMC_CMD_SELECT_CARD			7 
#define MMC_CMD_SEND_EXT_CSD        8
#define MMC_CMD_SEND_CSD			9
#define MMC_CMD_SEND_CID			10
#define MMC_CMD_STOP_TRANSMISSION   12
#define MMC_CMD_SEND_STATUS			13
#define MMC_CMD_SET_BLOCKLEN        16
#define MMC_CMD_READ_SINGLE_BLOCK   17
#define MMC_CMD_READ_MULTIPLE_BLOCK 18
#define MMC_CMD_WRITE_SINGLE_BLOCK  24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define MMC_CMD_APP_CMD				55
    
#define SD_CMD_SEND_RELATIVE_ADDR   3
#define SD_CMD_SWITCH_FUNC			6
#define SD_CMD_SEND_IF_COND			8

#define SD_CMD_APP_SET_BUS_WIDTH    6
#define SD_CMD_APP_SEND_OP_COND     41
#define SD_CMD_APP_SEND_SCR			51


/* Response Type */
#define MMC_RSP_PRESENT 	(1 << 0)
#define MMC_RSP_136     	(1 << 1)	/* 136 bit response */
#define MMC_RSP_CRC     	(1 << 2)	/* expect valid crc */
#define MMC_RSP_BUSY    	(1 << 3)	/* card may send busy */
#define MMC_RSP_OPCODE  	(1 << 4)	/* response contains opcode */
        
#define MMC_RSP_NONE    (0)
#define MMC_RSP_R1      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b 	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| MMC_RSP_BUSY)
#define MMC_RSP_R2      (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3      (MMC_RSP_PRESENT)
#define MMC_RSP_R4      (MMC_RSP_PRESENT)
#define MMC_RSP_R5      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

            

/* Error Code */
#define NO_CARD_ERR     -16 /* No SD/MMC card inserted */
#define UNUSABLE_ERR        -17 /* Unusable Card */
#define COMM_ERR        -18 /* Communications Error */
#define TIMEOUT         -19

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY   0x00020000
#define SD_HIGHSPEED_SUPPORTED  0x00020000

#define MMC_HS_TIMING       0x00000100
#define MMC_HS_52MHZ        0x2





#define MMC_SWITCH_MODE_CMD_SET     0x00 /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS    0x01 /* Set bits in EXT_CSD byte
                        addressed by index which are
                        1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS  0x02 /* Clear bits in EXT_CSD byte
                        addressed by index, which are
                        1 in value field */
#define MMC_SWITCH_MODE_WRITE_BYTE  0x03 /* Set target byte to value */



/*
 * EXT_CSD fields
 */

#define EXT_CSD_BUS_WIDTH   183 /* R/W */
#define EXT_CSD_HS_TIMING   185 /* R/W */
#define EXT_CSD_CARD_TYPE   196 /* RO */
#define EXT_CSD_REV     192 /* RO */
#define EXT_CSD_SEC_CNT     212 /* RO, 4 bytes */

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_CMD_SET_NORMAL      (1<<0)
#define EXT_CSD_CMD_SET_SECURE      (1<<1)
#define EXT_CSD_CMD_SET_CPSECURE    (1<<2)

#define EXT_CSD_CARD_TYPE_26    (1<<0)  /* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52    (1<<1)  /* Card can run at 52MHz */

#define EXT_CSD_BUS_WIDTH_1 0   /* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4 1   /* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8 2   /* Card is in 8 bit mode */

#define R1_ILLEGAL_COMMAND      (1 << 22)
#define R1_APP_CMD          (1 << 5)

struct mmc_cmd {
    uint16_t cmdidx;
    uint32_t resp_type;
    uint32_t cmdarg;
    uint32_t response[4];
    uint32_t flags;
};

struct mmc_data {
    union {
        char *dest;
        const char *src; /* src buffers don't get written to */
    }buff;
    
    uint32_t flags;
#define MMC_DATA_READ       1
#define MMC_DATA_WRITE      2
    
    uint32_t blocks;
    uint32_t blocksize;
};

struct mmc {
//	void *priv;
    uint32_t voltages;
#define MMC_VDD_165_195     0x00000080  /* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21       0x00000100  /* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22       0x00000200  /* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23       0x00000400  /* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24       0x00000800  /* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25       0x00001000  /* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26       0x00002000  /* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27       0x00004000  /* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28       0x00008000  /* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29       0x00010000  /* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30       0x00020000  /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31       0x00040000  /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32       0x00080000  /* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33       0x00100000  /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34       0x00200000  /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35       0x00400000  /* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36       0x00800000  /* VDD voltage 3.5 ~ 3.6 */
    
    uint32_t version;
#define SD_VERSION_SD   	0x20000
#define SD_VERSION_2    	(SD_VERSION_SD | 0x20)
#define SD_VERSION_1_0  	(SD_VERSION_SD | 0x10)
#define SD_VERSION_1_10 	(SD_VERSION_SD | 0x1a)
#define MMC_VERSION_MMC     0x10000
#define MMC_VERSION_UNKNOWN (MMC_VERSION_MMC)
#define MMC_VERSION_1_2     (MMC_VERSION_MMC | 0x12)
#define MMC_VERSION_1_4     (MMC_VERSION_MMC | 0x14)
#define MMC_VERSION_2_2     (MMC_VERSION_MMC | 0x22)
#define MMC_VERSION_3       (MMC_VERSION_MMC | 0x30)
#define MMC_VERSION_4       (MMC_VERSION_MMC | 0x40)

#define IS_SD(x) (x->version & SD_VERSION_SD)
    
    uint32_t f_min;
    uint32_t f_max;
    int high_capacity;
    
    uint32_t bus_width;
    uint32_t clock;
    
    uint32_t card_caps;
#define MMC_MODE_HS     0x001
#define MMC_MODE_HS_52MHz   0x010
#define MMC_MODE_4BIT       0x100 
#define MMC_MODE_8BIT       0x200
    
    uint32_t host_caps;
    uint32_t ocr;
#define OCR_BUSY    0x80000000
#define OCR_HCS     0x40000000
    
    uint32_t scr[2];    
    uint32_t csd[4];
    uint32_t cid[4];
    uint16_t rca;
    uint32_t tran_speed;
    uint32_t read_bl_len;
    uint32_t write_bl_len;
    uint64_t capacity;
    
    block_dev_desc_t block_dev;
    
    int (*send_cmd)(struct mmc *mmc,
            struct mmc_cmd *cmd, struct mmc_data *data);
    void (*set_ios)(struct mmc *mmc);
    int (*init)(struct mmc *mmc);
};

/*
 * MMC Function Protocols
 */
int mmc_initialize(void);
void mmc_deinitialize(void);
struct mmc* get_active_mmc(void);
int mmc_init(struct mmc *mmc);

/* implement in specific sd/mmc controller driver */
int cpu_mmc_init(void);
void cpu_mmc_exit(void);

#endif	/* _MMC_H */
