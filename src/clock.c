
#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>
#include <clk.h>


/*========== System Controller Registers ===================*/
#define SYSC_PLL1_CTRL_REG          (SYSC_BASE_ADDR + 0x00)
#define SYSC_PLL2_CTRL_REG          (SYSC_BASE_ADDR + 0x04)
#define SYSC_PLL3_CTRL_REG          (SYSC_BASE_ADDR + 0x08)
#define SYSC_PLL4_CTRL_REG          (SYSC_BASE_ADDR + 0x0c)
#define SYSC_PLL5_CTRL_REG          (SYSC_BASE_ADDR + 0x10)
#define SYSC_PLL6_CTRL_REG          (SYSC_BASE_ADDR + 0x14)
#define SYSC_PLLx_CTRL_REG(x)       (SYSC_PLL1_CTRL_REG + (((x)-1)<<2))

#define SYSC_FREQ_DIV1_REG          (SYSC_BASE_ADDR + 0x18)
#define SYSC_FREQ_DIV2_REG          (SYSC_BASE_ADDR + 0x1c)
#define SYSC_FREQ_DIV3_REG          (SYSC_BASE_ADDR + 0x20)
#define SYSC_CLKSRC_SEL_REG         (SYSC_BASE_ADDR + 0x24)

#define CLK_GATING_CONTROL1_REG 	(SYSC_BASE_ADDR + 0x28)
#define CLK_GATING_STATUS1_REG  	(SYSC_BASE_ADDR + 0x2c)
#define CLK_GATING_CONTROL2_REG 	(SYSC_BASE_ADDR + 0x30)
#define CLK_GATING_STATUS2_REG  	(SYSC_BASE_ADDR + 0x34)
#define CLK_GATING_CONTROL_REG(x)   (SYSC_BASE_ADDR + 0x28 + ((x)<<3))
#define CLK_GATING_STATUS_REG(x)    (SYSC_BASE_ADDR + 0x2c + ((x)<<3))

/*-------------------------Power Control ----------------------*/
#define PMU_SCRATCH_SRAM_ADDR		(PMU_BASE_ADDR + 0x00)

#define PMU_SYSPWR_STATE_REG		(PMU_BASE_ADDR + 0x400)
#define PMU_FUNCOREPWR_CTRL_REG		(PMU_BASE_ADDR + 0x404)
#define PMU_PADPWR_CTRL_REG			(PMU_BASE_ADDR + 0x408)
#define PMU_FUNCORE_PAD_TCNT_REG	(PMU_BASE_ADDR + 0x40c)
#define PMU_SCRATCH_REG				(PMU_BASE_ADDR + 0x420)


#define EXT_OSC_HZ			24000000


static void wait_bits_clear(unsigned reg, int bitmask)
{
	unsigned long val;
	
	while (1) {
		val = GET_REG(reg);
		
		if ((val & bitmask) == 0)
			break;
	}
}

static void wait_bits_set(unsigned reg, int bitmask)
{
	unsigned long val;
	
	while (1) {
		val = GET_REG(reg);
		
		if ((val & bitmask) == bitmask)
			break;
	}
}

void power_domain_switch(enum power_domain pd, int on)
{
    unsigned long pwr_ctrl_reg;
    unsigned long status;
    unsigned long pd_mask;

    if (pd >= PWR_PD5 && pd <= PWR_PD12){
        pwr_ctrl_reg = PMU_FUNCOREPWR_CTRL_REG;
        pd_mask = (1<<pd);
        
    } else if(pd >= PWR_PD_IOUSB && pd <= PWR_PD_IOLVDS){
        pwr_ctrl_reg = PMU_PADPWR_CTRL_REG;
        pd_mask = (1 << (pd - PWR_PD_IOUSB));
        
    } else {
    	return;
    }
    
    status = GET_REG(pwr_ctrl_reg);

    if (!!on ^ !!(status & pd_mask)) {
    	return;
    }
    
    if (on) {
    	wait_bits_clear(pwr_ctrl_reg, 0xc0000000);

        status &= ~0xc0000000;
        status &= ~pd_mask;
        SET_REG(pwr_ctrl_reg, (status | (1<<31)));

        wait_bits_clear(pwr_ctrl_reg, 0xc0000000);
        
    } else {
    	/*
    	wait_bits_clear(pwr_ctrl_reg, 0xc0000000);
    	
    	status &= ~0xc0000000;
    	status |= pd_mask;
    	SET_REG(pwr_ctrl_reg, (status | (1<<30)));
    	wait_bits_clear(pwr_ctrl_reg, 0xc0000000);
    	*/
    }

}

/*------------------------- Clock Control ----------------------*/
struct PLL_CTRL_REG_BITS{
	union {
		unsigned long val;
		struct {
			unsigned long reserved:7;
			unsigned long lock:1;
			unsigned long test:1;
			unsigned long bypass:1;
			unsigned long INTFB:1;
			unsigned long pdn:1;
			unsigned long NB:6;
			unsigned long OD:4;
			unsigned long NF:6;
			unsigned long NR:4;
		};
	};
};


static unsigned int calc_pll_out_freq(int pll_id)
{
	struct PLL_CTRL_REG_BITS	pll;

	pll.val = GET_REG(SYSC_PLLx_CTRL_REG(pll_id));

	return ((EXT_OSC_HZ) * (pll.NF+1) / (pll.NR+1) / (pll.OD+1));
}

unsigned long get_AHB_clk(void)
{
	unsigned long val;
	int divider = 1;
	unsigned long src_freq;
	
	if (!(GET_REG(SYSC_CLKSRC_SEL_REG) & (1<<27))) {
		// AXI bus clock source from PLL1
		src_freq = calc_pll_out_freq(1);
	} else {
		// AXI bus clock source from PLL2
		src_freq = calc_pll_out_freq(2);
	}

	val = (GET_REG(SYSC_FREQ_DIV1_REG) >> 20) & 0xf;		//AXI bus divider
	if (val >= 1 && val <= 6)
		divider = val + 1;
	
	return ((src_freq / divider) >> 1);
}

unsigned long get_APB_clk(void)
{
	return calc_pll_out_freq(5);
}

unsigned long get_uart_clk(int idx)
{
	return (calc_pll_out_freq(5) >> 1);
}

unsigned long get_timer_clk(void)
{
	return get_APB_clk();
}

unsigned long get_sdio_clk(int idx)
{
	return get_AHB_clk();
}

void clock_switch(enum clock_id clk, int on)
{
	int idx = clk >> 5;
	unsigned long mask = (1 << (clk & 0x1f));
	unsigned long status = GET_REG(CLK_GATING_STATUS_REG(idx));
	unsigned long ctrl;

	if (on && (status & mask))
		return;

	ctrl = GET_REG(CLK_GATING_CONTROL_REG(idx));
	if (on)
		ctrl |= mask;
	else
		ctrl &= ~mask;
	SET_REG(CLK_GATING_CONTROL_REG(idx), ctrl);

	if (on)
		wait_bits_set(CLK_GATING_STATUS_REG(idx), mask);
}


