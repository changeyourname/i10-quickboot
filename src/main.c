/**
 * @file
 * @brief	bootloader main routine
 * @author	jimmy.li<lizhengming@innofidei.com>
 * @date	2010/07/01
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <hardware.h>
#include <iomux.h>

#include <io.h>

extern void start_console(void);

static mux_pin_t muxpins_cfg[] = {
	/* UART 0 */
	MP_UTXD0, MP_URXD0,

	/* UART 1 */
	MP_UTXD1, MP_URXD1,
	
	/* UART 2 */
	MP_UTXD2, MP_URXD2,

	/* UART 3 */
	MP_UTXD3, MP_URXD3,

	/* SDIO 0 */
	MP_SD0_CLK, MP_SD0_CMD, MP_SD0_DAT3, MP_SD0_DAT2, MP_SD0_DAT1, MP_SD0_DAT0,

	/* SDIO 1 */
	MP_SD1_CLK, MP_SD1_CMD, MP_SD1_DAT3, MP_SD1_DAT2, MP_SD1_DAT1, MP_SD1_DAT0,

	/* SDIO 2 */
	MP_SD2_CLK, MP_SD2_CMD, MP_SD2_DAT3, MP_SD2_DAT2, MP_SD2_DAT1, MP_SD2_DAT0,

	/* NAND */
	MP_NAND_CEN0, MP_NAND_CLE, MP_NAND_ALE, MP_NAND_REN, MP_NAND_WEN, MP_NAND_RBN,
	MP_NAND_WPN, MP_NAND_IO0, MP_NAND_IO1, MP_NAND_IO2, MP_NAND_IO3, MP_NAND_IO4,
	MP_NAND_IO5, MP_NAND_IO6, MP_NAND_IO7, MP_NAND_IO8, MP_NAND_IO9, MP_NAND_IO10,
	MP_NAND_IO11, MP_NAND_IO12, MP_NAND_IO13, MP_NAND_IO14, MP_NAND_IO15,

	/* Ether */
	MP_EMI_OEN_ETH_MDC, MP_EMI_WEN_ETH_MDIO, MP_EMI_WAIT_ETH_RX_ER, MP_EMI_EQ15_ETH_RXD3,
	MP_EMI_EQ14_ETH_RXD2, MP_EMI_EQ13_ETH_RXD1, MP_EMI_EQ12_ETH_RXD0, MP_EMI_EQ11_ETH_RX_DV,
	MP_EMI_EQ10_ETH_RX_CLK, MP_EMI_EQ9_ETH_TX_ER, MP_EMI_EQ8_ETH_TXD3, MP_EMI_EQ7_ETH_TXD2,
	MP_EMI_EQ6_ETH_TXD1, MP_EMI_EQ5_ETH_TXD0, MP_EMI_EQ4_ETH_TX_EN, MP_EMI_EQ3_ETH_TX_CLK,
	MP_EMI_EQ2_ETH_CRS, MP_EMI_EQ1_ETH_COL,
};


static int i10_init(void)
{
	unsigned long val;

	val = __raw_readl(PMU_BASE_ADDR + 0x400);
	val &= ~0x7;	// set power on state
	val |= (1<<26);		// GPIO0~31 output
	__raw_writel(val, PMU_BASE_ADDR + 0x400);

	i10_iomux_config(muxpins_cfg, ARRAY_SIZE(muxpins_cfg));

	timer_init();
	serial_init();

	return 0;
}

static void dead_loop(void)
{
	while(1)
		;
}

int main(void)
{

	i10_init();

	PRINT("welcome to bootloader!\n");

	start_console();

	dead_loop();
	
	return 0;
}
