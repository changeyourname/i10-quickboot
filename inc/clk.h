
#ifndef __ASM_ARM_ARCH_CLK_H_
#define __ASM_ARM_ARCH_CLK_H_

unsigned long get_AHB_clk(void);
unsigned long get_APB_clk(void);

unsigned long get_uart_clk(int idx);
unsigned long get_timer_clk(void);
unsigned long get_sdio_clk(int idx);
unsigned long get_nand_clk(void);

enum clock_id {
	CLK_TIMER = 0,
	CLK_GPIO2,
	CLK_GPIO1,
	CLK_GPIO0,
	CLK_I2C2,
	CLK_I2C1,
	CLK_I2C0,
	CLK_EMIS,
	CLK_EMIM,
	CLK_SPIM0,
	CLK_SPIS,
	CLK_SDIO1,
	CLK_SDIO0,
	CLK_UART3,
	CLK_UART2,
	CLK_UART1,
	CLK_UART0,
	CLK_NAND,
	CLK_7816,
	CLK_AIC,
	CLK_SRAM,
	CLK_ETH,
	CLK_USBD,
	CLK_USBH1,
	CLK_USBH0,
	CLK_DMAC,
	CLK_GPU,
	CLK_8290,
	CLK_G1,
	CLK_DDRC,
	CLK_ZSP = 30,
	CLK_EFUSE = 32 + 12,
	CLK_OSC,
	CLK_GPIO5,
	CLK_GPIO4,
	CLK_GPIO3,
	CLK_I2C3,
	CLK_SPIM2,
	CLK_SPIM1,
	CLK_SDIO2,
	CLK_RTC,
	CLK_CIPHER,
	CLK_TS,
	CLK_AUDIO,
	CLK_DI,
	CLK_VOUT,
	CLK_VIN,
	CLK_SARADC,
	CLK_WDT,
	CLK_PWM1,
	CLK_PWM0
};

void clock_switch(enum clock_id clk, int on);


enum power_domain {
	// Core
    PWR_PD5 = 0,	// ZSP
    PWR_PD6,		//G1
    PWR_PD7,
    PWR_PD8,		//GPU
    PWR_PD9,		// VOUT
    PWR_PD10,		// VIN, 8290
    PWR_PD11,	// USB, ETH
    PWR_PD12,	//NFC, EMIM, EMIS, SDIO 

    // Pad
    PWR_PD_IOUSB,
    PWR_PD_IOADC,
    PWR_PD_IOLVDS,
};

void power_domain_switch(enum power_domain pd, int on);

#endif
