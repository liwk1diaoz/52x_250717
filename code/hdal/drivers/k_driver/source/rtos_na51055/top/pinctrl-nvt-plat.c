/*
 * Driver for the Novatek pinmux
 *
 * Copyright (c) 2018, NOVATEK MICROELECTRONIC CORPORATION.  All rights reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libfdt.h>
#include <compiler.h>
#include <rtosfdt.h>
#include "malloc.h"
#include "nvt_pinmux.h"

u32 top_reg_addr = 0;
extern u32 top_reg_addr;

static PINMUX_FUNC_ID id_restore = 0x0;
static u32 pinmux_restore = 0x0;

static void nvt_pinmux_show_conflict(struct nvt_pinctrl_info *info)
{
	int i;

	pinmux_parsing(info);
	for (i = 0; i < PIN_FUNC_MAX; i++) {
		printf("pinmux %-2d config 0x%lx\n", i, info->top_pinmux[i].config);
	}
}


int nvt_pinmux_capture(PIN_GROUP_CONFIG *pinmux_config, int count)
{
	struct nvt_pinctrl_info *info;
	uint8_t i, j;
	int ret = E_OK;

	info = malloc(sizeof(struct nvt_pinctrl_info));
	if (!info) {
		printf("nvt pinmux mem alloc fail\n");
		return -E_OACV;
	}

	if (top_reg_addr) {
		info->top_base = top_reg_addr;
		pinmux_parsing(info);

		for (j = 0; j < count; j++) {
			for (i = 0; i < PIN_FUNC_MAX; i++) {
				if (i == pinmux_config[j].pin_function)
					pinmux_config[j].config = info->top_pinmux[i].config;
			}
		}
	} else {
		printf("invalid pinmux address\n");
		ret = -E_OACV;
	}

	free(info);

	return ret;
}

int nvt_pinmux_update(PIN_GROUP_CONFIG *pinmux_config, int count)
{
	struct nvt_pinctrl_info *info;
	uint8_t i, j;
	int ret = E_OK;

	info = malloc(sizeof(struct nvt_pinctrl_info));
	if (!info) {
		printf("nvt pinmux mem alloc fail\n");
		return -E_OACV;
	}

	if (top_reg_addr) {
		info->top_base = top_reg_addr;
		pinmux_parsing(info);

		for (j = 0; j < count; j++) {
			for (i = 0; i < PIN_FUNC_MAX; i++) {
				if (i == pinmux_config[j].pin_function)
					info->top_pinmux[i].config = pinmux_config[j].config;
			}
		}

		ret = pinmux_init(info);
		if (ret == E_OK)
			ret = pinmux_set_config(id_restore, pinmux_restore);
	} else {
		printf("invalid pinmux address\n");
		ret = -E_OACV;
	}

	free(info);

	return ret;
}


int pinmux_set_config(PINMUX_FUNC_ID id, u32 pinmux)
{
	struct nvt_pinctrl_info *info;
	int ret;

	info = malloc(sizeof(struct nvt_pinctrl_info));
	if (!info) {
		printf("nvt pinmux mem alloc fail\n");
		return -E_OACV;
	}

	if (top_reg_addr) {
		info->top_base = top_reg_addr;
		ret = pinmux_set_host(info, id, pinmux);

		if (id <= PINMUX_FUNC_ID_LCD2) {
			id_restore = id;
			pinmux_restore = pinmux;
		}
	} else {
		printf("invalid pinmux address\n");
		ret = -E_OACV;
	}

	free(info);

	return ret;
}

static void gpio_init(struct nvt_gpio_info *gpio, int nr_gpio, struct nvt_pinctrl_info *info)
{
	uint32_t reg_data, tmp, ofs, offset;
	int cnt = 0;

	for (cnt = 0; cnt < nr_gpio; cnt++) {
		offset = gpio[cnt].gpio_pin;
		ofs = (offset >> 5) << 2;
		offset &= (32-1);
		tmp = (1 << offset);
		reg_data = GPIO_GETREG(info, NVT_GPIO_STG_DIR_0 + ofs);
		reg_data |= (1<<offset);    /*output*/
		GPIO_SETREG(info, NVT_GPIO_STG_DIR_0 + ofs, reg_data);

		if (gpio[cnt].direction)
			GPIO_SETREG(info, NVT_GPIO_STG_SET_0 + ofs, tmp);
		else
			GPIO_SETREG(info, NVT_GPIO_STG_CLR_0 + ofs, tmp);
	}
}

int nvt_pinmux_init(void)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();;
	char path[20] = {0};
	int nodeoffset;
	uint32_t *cell = NULL;
	int node, nr_pinmux = 0, nr_pad = 0, nr_gpio = 0;
	struct nvt_pinctrl_info *info;
	struct nvt_gpio_info *gpio;

    if(fdt_addr ==NULL)
		return -E_OACV;

	info = malloc(sizeof(struct nvt_pinctrl_info));
	info->top_base = IOADDR_TOP_REG_BASE;
	info->pad_base = IOADDR_PAD_REG_BASE;
	info->gpio_base = IOADDR_GPIO_REG_BASE;
	top_reg_addr = info->top_base;

	sprintf(path,"/top@%x",IOADDR_TOP_REG_BASE);
	nodeoffset = fdt_path_offset((const void*)fdt_addr, path);

	fdt_for_each_subnode(node, fdt_addr, nodeoffset) {
		cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, node, "gpio_config", NULL);

		if (cell > 0)
			nr_gpio++;
	}

	gpio = malloc(nr_gpio * sizeof(struct nvt_pinctrl_info));

	nr_gpio = 0;

	fdt_for_each_subnode(node, fdt_addr, nodeoffset) {
		cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, node, "pinmux", NULL);
		if (cell > 0) {
			info->top_pinmux[nr_pinmux].pin_function = nr_pinmux;
			info->top_pinmux[nr_pinmux].config = be32_to_cpu(cell[0]);
			nr_pinmux++;
		}

		cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, node, "pad_config", NULL);
		if (cell > 0) {
			info->pad[nr_pad].pad_ds_pin = be32_to_cpu(cell[0]);
			info->pad[nr_pad].driving = be32_to_cpu(cell[1]);
			info->pad[nr_pad].pad_gpio_pin = be32_to_cpu(cell[2]);
			info->pad[nr_pad].direction = be32_to_cpu(cell[3]);
			nr_pad++;
		}

		cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, node, "gpio_config", NULL);
		if (cell > 0) {
			gpio[nr_gpio].gpio_pin = be32_to_cpu(cell[0]);
			gpio[nr_gpio].direction = be32_to_cpu(cell[1]);
			nr_gpio++;
		}
	}

	if (nr_pinmux == 0) {
		free(info);
		free(gpio);
		return -E_OACV;
	}

	pinmux_preset(info);

	if (pinmux_init(info))
		nvt_pinmux_show_conflict(info);

	pad_init(info, nr_pad);

	if (nr_gpio)
		gpio_init(gpio, nr_gpio, info);

	free(info);
	free(gpio);

	return E_OK;
}
