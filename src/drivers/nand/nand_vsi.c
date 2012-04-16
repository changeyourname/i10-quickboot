/**
 * @file
 * @brief	I10 Nand Controller driver
 * @date	2011/04/06
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <io.h>
#include "nand.h"

#include <hardware.h>

#define nand_err			PRINT
#define nand_dbg			PRINT

#define MAX_CHIPS		1
static struct nand_controller controller;
static struct nand_chip	chips[MAX_CHIPS];

static void i10_nand_select_chip(struct nand_chip* chip, int isCE)
{

}

static uint8_t i10_nand_read_byte(struct nand_controller *nand_ctrl)
{
	return 0;
}

static void i10_nand_command(struct nand_chip* chip, unsigned command, int column, int page_addr)
{

}

int cpu_nand_init(void)
{
	struct nand_controller* ctrl = &controller;
	int ret = 0;

	ctrl->maxchips = MAX_CHIPS;
	ctrl->chip = &chips[0];

	ctrl->select_chip = i10_nand_select_chip;
	ctrl->read_byte = i10_nand_read_byte;
	ctrl->cmdfunc = i10_nand_command;

	ret = nand_controller_register(ctrl);

	return ret;
}

void cpu_nand_exit(void)
{
}
