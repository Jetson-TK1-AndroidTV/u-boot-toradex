/*
 * (C) Copyright 2013-2014
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

/* enable PMIC */
#define CONFIG_AS3722_POWER

#include "tegra124-common.h"

/* High-level configuration options */
#define V_PROMPT			"Tegra124 (Jetson TK1) # "
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA Jetson TK1"

/* Board-specific serial config */
#define CONFIG_SERIAL_MULTI
#define CONFIG_TEGRA_ENABLE_UARTD
#define CONFIG_SYS_NS16550_COM1		NV_PA_APB_UARTD_BASE

/* I2C */
#define CONFIG_SYS_I2C_TEGRA
#define CONFIG_CMD_I2C

/* SD/MMC */
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_TEGRA_MMC
#define CONFIG_CMD_MMC

/* Environment in eMMC, at the end of 2nd "boot sector" */
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_ENV_OFFSET		(-CONFIG_ENV_SIZE)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		2

/* SPI */
#define CONFIG_TEGRA114_SPI		/* Compatible w/ Tegra114 SPI */
#define CONFIG_TEGRA114_SPI_CTRLS	6
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED		24000000
#define CONFIG_CMD_SPI
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH_SIZE		(4 << 20)

/* USB Host support */
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_TEGRA
#define CONFIG_USB_MAX_CONTROLLER_COUNT	2
#define CONFIG_USB_STORAGE
#define CONFIG_CMD_USB

/* USB networking support */
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX

/* PCI host support */
#define CONFIG_PCI
#define CONFIG_PCI_TEGRA
#define CONFIG_PCI_PNP
#define CONFIG_CMD_PCI
#define CONFIG_CMD_PCI_ENUM

/* PCI networking support */
#define CONFIG_RTL8169

/* General networking support */
#define CONFIG_CMD_NET
#define CONFIG_CMD_DHCP

/* Fastboot support */
#define CONFIG_CMD_FASTBOOT

#ifdef CONFIG_CMD_FASTBOOT
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_CMD_BOOTAI
#define CONFIG_CMD_BOOTAI_IGNORE_HDR_ADDR
#define MEMORY_BASE                     0x80000000
#define CONFIG_ADDR_DOWNLOAD            (MEMORY_BASE + 0x04000000)
#define CONFIG_USB_FASTBOOT_BUF_ADDR	CONFIG_ADDR_DOWNLOAD
#define CONFIG_USB_FASTBOOT_BUF_SIZE	0x40000000
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV	0
#define BOARD_EXTRA_ENV_SETTINGS \
	"bootargs_append=pmuboard=0x0177:0x0000:0x02:0x43:0x00 otf_key=c75e5bb91eb3bd947560357b64422f85 board_info=0x0177:0x0000:0x02:0x43:0x00\0"
#endif

#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

#endif /* __CONFIG_H */
