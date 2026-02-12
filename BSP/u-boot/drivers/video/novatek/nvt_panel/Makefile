# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2021 Novateck, Inc
#
include $(NVT_PRJCFG_MODEL_CFG)

ifneq ($(CONFIG_TARGET_NA51090_A64),y)
ifeq ($(CONFIG_VIDEO_LOGO),y)
obj-y += $(LCD1)/
obj-y += pinctrl-nvt-plat.o dispdev_panlcomn.o dispdev_platform.o
endif
endif
