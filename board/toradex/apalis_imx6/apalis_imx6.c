/*
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
 * Copyright (C) 2013, Boundary Devices <info@boundarydevices.com>
 * Copyright (C) 2014, Toradex AG
 * copied from nitrogen6x
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <malloc.h>
#include <asm/arch/mx6-pins.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/sata.h>
#include <asm/imx-common/boot_mode.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <micrel.h>
#include <miiphy.h>
#include <netdev.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/mxc_hdmi.h>
#include <i2c.h>
#include "pf0100.h"

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_BOARD_LATE_INIT) && (defined(CONFIG_TRDX_CFG_BLOCK) || \
		defined(CONFIG_SERIAL_TAG))
/* buffer suitable for DMA */
#define CONFIG_BLOCK_BUFFER_SIZE 4096
static unsigned char config_block[roundup(CONFIG_BLOCK_BUFFER_SIZE, ARCH_DMA_MINALIGN)]
                                  __aligned(ARCH_DMA_MINALIGN);
#endif

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define BUTTON_PAD_CTRL (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define I2C_PAD_CTRL	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define WEAK_PULLUP	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_SRE_SLOW)

#define WEAK_PULLDOWN	(PAD_CTL_PUS_100K_DOWN |		\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

#define TRISTATE	(PAD_CTL_HYS | PAD_CTL_SPEED_MED)

#define OUTPUT_40OHM (PAD_CTL_SPEED_MED|PAD_CTL_DSE_40ohm)

int dram_init(void)
{
	/* use the DDR controllers configured size */
	gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				    (ulong)imx_ddr_size());

	return 0;
}

/* Apalis UART1 */
iomux_v3_cfg_t const uart1_pads[] = {
#ifndef CONFIG_MXC_UART_DTE
	MX6_PAD_CSI0_DAT10__UART1_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_CSI0_DAT11__UART1_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
#else
	MX6_PAD_CSI0_DAT10__UART1_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_CSI0_DAT11__UART1_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
#endif
};

#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* Apalis I2C1 */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX6_PAD_CSI0_DAT9__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_CSI0_DAT9__GPIO5_IO27 | PC,
		.gp = IMX_GPIO_NR(5, 27)
	},
	.sda = {
		.i2c_mode = MX6_PAD_CSI0_DAT8__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_CSI0_DAT8__GPIO5_IO26 | PC,
		.gp = IMX_GPIO_NR(5, 26)
	}
};

/* Apalis local, PMIC, SGTL5000, STMPE811*/
struct i2c_pads_info i2c_pad_info_loc = {
	.scl = {
		.i2c_mode = MX6_PAD_KEY_COL3__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_KEY_COL3__GPIO4_IO12 | PC,
		.gp = IMX_GPIO_NR(4, 12)
	},
	.sda = {
		.i2c_mode = MX6_PAD_KEY_ROW3__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_KEY_ROW3__GPIO4_IO13 | PC,
		.gp = IMX_GPIO_NR(4, 13)
	}
};

/* Apalis I2C3 / CAM */
struct i2c_pads_info i2c_pad_info3 = {
	.scl = {
		.i2c_mode = MX6_PAD_EIM_D17__I2C3_SCL | PC,
		.gpio_mode = MX6_PAD_EIM_D17__GPIO3_IO17 | PC,
		.gp = IMX_GPIO_NR(3, 17)
	},
	.sda = {
		.i2c_mode = MX6_PAD_EIM_D18__I2C3_SDA | PC,
		.gpio_mode = MX6_PAD_EIM_D18__GPIO3_IO18 | PC,
		.gp = IMX_GPIO_NR(3, 18)
	}
};

/* Apalis I2C2 / DDC */
struct i2c_pads_info i2c_pad_info_ddc = {
	.scl = {
		.i2c_mode = MX6_PAD_EIM_EB2__HDMI_TX_DDC_SCL | PC,
		.gpio_mode = MX6_PAD_EIM_EB2__GPIO2_IO30 | PC,
		.gp = IMX_GPIO_NR(2, 30)
	},
	.sda = {
		.i2c_mode = MX6_PAD_EIM_D16__HDMI_TX_DDC_SDA | PC,
		.gpio_mode = MX6_PAD_EIM_D16__GPIO3_IO16 | PC,
		.gp = IMX_GPIO_NR(3, 16)
	}
};

/* Apalis MMC1 */
iomux_v3_cfg_t const usdhc1_pads[] = {
	MX6_PAD_SD1_CLK__SD1_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__SD1_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_D0__SD1_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_D1__SD1_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_D2__SD1_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_D3__SD1_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_DI0_PIN4__GPIO4_IO20   | MUX_PAD_CTRL(NO_PAD_CTRL), /* CD */
#	define GPIO_MMC_CD IMX_GPIO_NR(4, 20)
};

/* Apalis SD1 */
iomux_v3_cfg_t const usdhc2_pads[] = {
	MX6_PAD_SD2_CLK__SD2_CLK    | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_CMD__SD2_CMD    | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT0__SD2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT1__SD2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT2__SD2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT3__SD2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_CS1__GPIO6_IO14  | MUX_PAD_CTRL(NO_PAD_CTRL), /* CD */
#	define GPIO_SD_CD IMX_GPIO_NR(6, 14)
};

/* eMMC */
iomux_v3_cfg_t const usdhc3_pads[] = {
	MX6_PAD_SD3_CLK__SD3_CLK    | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_CMD__SD3_CMD    | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT4__SD3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT5__SD3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT6__SD3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT7__SD3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_RST__SD3_RESET  | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

int mx6_rgmii_rework(struct phy_device *phydev)
{
	/*
	 * Bug: Apparently Apalis iMX6 does not works with Gigabit switches...
	 * Limiting speed to 10/100Mbps, and setting master mode, seems to
	 * be the only way to have a successfull PHY auto negotiation.
	 * How to fix: Understand why Linux kernel do not have this issue.
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, 0x1c00);

	/* control data pad skew - devaddr = 0x02, register = 0x04 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* rx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* tx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* gtx and rx clock pad skew - devaddr = 0x02, register = 0x08 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x03FF);
	return 0;
}

iomux_v3_cfg_t const enet_pads[] = {
	MX6_PAD_ENET_MDIO__ENET_MDIO		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_MDC__ENET_MDC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TXC__RGMII_TXC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD0__RGMII_TD0		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD1__RGMII_TD1		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD2__RGMII_TD2		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD3__RGMII_TD3		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TX_CTL__RGMII_TX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_REF_CLK__ENET_TX_CLK	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RXC__RGMII_RXC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD0__RGMII_RD0		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD1__RGMII_RD1		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD2__RGMII_RD2		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD3__RGMII_RD3		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RX_CTL__RGMII_RX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* KSZ9031 PHY Reset */
	MX6_PAD_ENET_CRS_DV__GPIO1_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL),
#	define GPIO_ENET_PHY_RESET IMX_GPIO_NR(1, 25)
};

static void setup_iomux_enet(void)
{
	imx_iomux_v3_setup_multiple_pads(enet_pads, ARRAY_SIZE(enet_pads));
}

static int reset_enet_phy (struct mii_dev *bus)
{
	/* Reset KSZ9031 PHY */
	gpio_direction_output(GPIO_ENET_PHY_RESET, 0);
	mdelay(10);
	gpio_set_value(GPIO_ENET_PHY_RESET, 1);

	return 0;
}

iomux_v3_cfg_t const usb_pads[] = {
	/* TODO This pin has a dedicated USB power functionality, can we use it? */
	/* USBH_EN */
	MX6_PAD_GPIO_0__GPIO1_IO00  | MUX_PAD_CTRL(NO_PAD_CTRL),
#	define GPIO_USBH_EN IMX_GPIO_NR(1, 0)
	/* USB_VBUS_DET */
	MX6_PAD_EIM_D28__GPIO3_IO28 | MUX_PAD_CTRL(NO_PAD_CTRL),
#	define GPIO_USB_VBUS_DET IMX_GPIO_NR(3, 28)
	/* USBO1_ID */
	MX6_PAD_ENET_RX_ER__USB_OTG_ID	| MUX_PAD_CTRL(WEAK_PULLUP),
	/* USBO1_EN */
	MX6_PAD_EIM_D22__GPIO3_IO22 | MUX_PAD_CTRL(NO_PAD_CTRL),
#	define GPIO_USBO_EN IMX_GPIO_NR(3, 22)
};

/* if UARTs are used in DTE mode, switch the mode on all UARTs before
 * any pinmuxing connects a (DCE) output to a transceiver output.
 */
#define UFCR		0x90	/* FIFO Control Register */
#define UFCR_DCEDTE	(1<<6)	/* DCE=0 */
#define SET_DCEDTE(p)	(writel( (readl((u32 *) (p)) | UFCR_DCEDTE), (u32 *) (p)))

#ifdef CONFIG_MXC_UART_DTE
static void setup_dtemode_uart(void)
{
	SET_DCEDTE(UART1_BASE + UFCR);
	SET_DCEDTE(UART2_BASE + UFCR);
	SET_DCEDTE(UART4_BASE + UFCR);
	SET_DCEDTE(UART5_BASE + UFCR);
}
#endif 

static void setup_iomux_uart(void)
{
#ifdef CONFIG_MXC_UART_DTE
	setup_dtemode_uart();
#endif
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_USB_EHCI_MX6
int board_ehci_hcd_init(int port)
{
	imx_iomux_v3_setup_multiple_pads(usb_pads, ARRAY_SIZE(usb_pads));
	return 0;
}

int board_ehci_power(int port, int on)
{
	switch (port) {
	case 0:
		/* control OTG power */
		gpio_direction_output(GPIO_USBO_EN, on);
		mdelay(100);
		break;
	case 1:
		/* Control MXM USBH */
		gpio_direction_output(GPIO_USBH_EN, on);
		mdelay(2);
		/* Control onboard USB Hub VBUS */
		gpio_direction_output(GPIO_USB_VBUS_DET, on);
		mdelay(100);
		break;
	default:
		break;
	}
	return 0;
}
#endif

#ifdef CONFIG_FSL_ESDHC
/* use the following sequence: eMMC, MMC, SD */
struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC3_BASE_ADDR},
	{USDHC1_BASE_ADDR},
	{USDHC2_BASE_ADDR},
};

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = true; /* default: assume inserted */

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		gpio_direction_input(GPIO_MMC_CD);
		ret = !gpio_get_value(GPIO_MMC_CD);
		break;
	case USDHC2_BASE_ADDR:
		gpio_direction_input(GPIO_SD_CD);
		ret = !gpio_get_value(GPIO_SD_CD);
		break;
	}

	return ret;
}

int board_mmc_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
	usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	usdhc_cfg[2].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);

	usdhc_cfg[0].max_bus_width = 8;
	usdhc_cfg[1].max_bus_width = 8;
	usdhc_cfg[2].max_bus_width = 4;

	for (index = 0; index < CONFIG_SYS_FSL_USDHC_NUM; ++index) {
		switch (index) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
			break;
		case 2:
			imx_iomux_v3_setup_multiple_pads(
				usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) then supported by the board (%d)\n",
				index + 1, CONFIG_SYS_FSL_USDHC_NUM);
			return status;
		}

		status |= fsl_esdhc_initialize(bis, &usdhc_cfg[index]);
	}

	return status;
}
#endif

int board_phy_config(struct phy_device *phydev)
{
	mx6_rgmii_rework(phydev);
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	uint32_t base = IMX_FEC_BASE;
	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;
	int ret;

	setup_iomux_enet();

#ifdef CONFIG_FEC_MXC
	bus = fec_get_miibus(base, -1);
	if (!bus)
		return 0;
	bus->reset = reset_enet_phy;
	/* scan phy 4,5,6,7 */
	phydev = phy_find_by_mask(bus, (0xf << 4), PHY_INTERFACE_MODE_RGMII);
	if (!phydev) {
		free(bus);
		puts("no phy found\n");
		return 0;
	}
	printf("using phy at %d\n", phydev->addr);
	ret  = fec_probe(bis, -1, base, bus, phydev);
	if (ret) {
		printf("FEC MXC: %s:failed\n", __func__);
		free(phydev);
		free(bus);
	}
#endif
	return 0;
}

#if defined(CONFIG_VIDEO_IPUV3)

static iomux_v3_cfg_t const backlight_pads[] = {
	/* Backlight on RGB connector: J15 */
	MX6_PAD_EIM_DA13__GPIO3_IO13 | MUX_PAD_CTRL(NO_PAD_CTRL),
#define RGB_BACKLIGHT_GP IMX_GPIO_NR(3, 13)
/* TODO PWM not GPIO */
	/* additional CPU pin on BKL_PWM, keep in tristate */
	MX6_PAD_EIM_DA14__GPIO3_IO14 | MUX_PAD_CTRL(TRISTATE),
	/* PWM4 pin */
	MX6_PAD_SD4_DAT2__GPIO2_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
#define RGB_BACKLIGHTPWM_GP IMX_GPIO_NR(2, 10)
	/* buffer output enable 0: buffer enabled*/
	MX6_PAD_EIM_A25__GPIO5_IO02 | MUX_PAD_CTRL(WEAK_PULLUP),
#define RGB_BACKLIGHTPWM_OE IMX_GPIO_NR(5, 2)
	/* PSAVE# integrated VDAC */
	MX6_PAD_EIM_BCLK__GPIO6_IO31 | MUX_PAD_CTRL(NO_PAD_CTRL),
#define VGA_PSAVE_NOT_GP IMX_GPIO_NR(6, 31)
};

static iomux_v3_cfg_t const pwr_intb_pads[] = {
	/* the bootrom sets the iomux to vselect, potentially connecting
	 * two outputs. Set this back to GPIO */
	MX6_PAD_GPIO_18__GPIO7_IO13 | MUX_PAD_CTRL(NO_PAD_CTRL)
};

static iomux_v3_cfg_t const rgb_pads[] = {
	MX6_PAD_EIM_A16__IPU1_DI1_DISP_CLK,
	MX6_PAD_EIM_DA10__IPU1_DI1_PIN15,
	MX6_PAD_EIM_DA11__IPU1_DI1_PIN02,
	MX6_PAD_EIM_DA12__IPU1_DI1_PIN03,
	MX6_PAD_EIM_DA9__IPU1_DISP1_DATA00,
	MX6_PAD_EIM_DA8__IPU1_DISP1_DATA01,
	MX6_PAD_EIM_DA7__IPU1_DISP1_DATA02,
	MX6_PAD_EIM_DA6__IPU1_DISP1_DATA03,
	MX6_PAD_EIM_DA5__IPU1_DISP1_DATA04,
	MX6_PAD_EIM_DA4__IPU1_DISP1_DATA05,
	MX6_PAD_EIM_DA3__IPU1_DISP1_DATA06,
	MX6_PAD_EIM_DA2__IPU1_DISP1_DATA07,
	MX6_PAD_EIM_DA1__IPU1_DISP1_DATA08,
	MX6_PAD_EIM_DA0__IPU1_DISP1_DATA09,
	MX6_PAD_EIM_EB1__IPU1_DISP1_DATA10,
	MX6_PAD_EIM_EB0__IPU1_DISP1_DATA11,
	MX6_PAD_EIM_A17__IPU1_DISP1_DATA12,
	MX6_PAD_EIM_A18__IPU1_DISP1_DATA13,
	MX6_PAD_EIM_A19__IPU1_DISP1_DATA14,
	MX6_PAD_EIM_A20__IPU1_DISP1_DATA15,
	MX6_PAD_EIM_A21__IPU1_DISP1_DATA16,
	MX6_PAD_EIM_A22__IPU1_DISP1_DATA17,
	MX6_PAD_EIM_A23__IPU1_DISP1_DATA18,
	MX6_PAD_EIM_A24__IPU1_DISP1_DATA19,
	MX6_PAD_EIM_D26__IPU1_DISP1_DATA22,
	MX6_PAD_EIM_D27__IPU1_DISP1_DATA23,
	MX6_PAD_EIM_D30__IPU1_DISP1_DATA21,
	MX6_PAD_EIM_D31__IPU1_DISP1_DATA20,
};

static iomux_v3_cfg_t const vga_pads[] = {
#ifdef FOR_DL_SOLO
	/* Dualite/Solo doesn't have IPU2 */
	MX6_PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK,
	MX6_PAD_DI0_PIN15__IPU1_DI0_PIN15,
	MX6_PAD_DI0_PIN2__IPU1_DI0_PIN02,
	MX6_PAD_DI0_PIN3__IPU1_DI0_PIN03,
	MX6_PAD_DISP0_DAT0__IPU1_DISP0_DATA00,
	MX6_PAD_DISP0_DAT1__IPU1_DISP0_DATA01,
	MX6_PAD_DISP0_DAT2__IPU1_DISP0_DATA02,
	MX6_PAD_DISP0_DAT3__IPU1_DISP0_DATA03,
	MX6_PAD_DISP0_DAT4__IPU1_DISP0_DATA04,
	MX6_PAD_DISP0_DAT5__IPU1_DISP0_DATA05,
	MX6_PAD_DISP0_DAT6__IPU1_DISP0_DATA06,
	MX6_PAD_DISP0_DAT7__IPU1_DISP0_DATA07,
	MX6_PAD_DISP0_DAT8__IPU1_DISP0_DATA08,
	MX6_PAD_DISP0_DAT9__IPU1_DISP0_DATA09,
	MX6_PAD_DISP0_DAT10__IPU1_DISP0_DATA10,
	MX6_PAD_DISP0_DAT11__IPU1_DISP0_DATA11,
	MX6_PAD_DISP0_DAT12__IPU1_DISP0_DATA12,
	MX6_PAD_DISP0_DAT13__IPU1_DISP0_DATA13,
	MX6_PAD_DISP0_DAT14__IPU1_DISP0_DATA14,
	MX6_PAD_DISP0_DAT15__IPU1_DISP0_DATA15,
#else
	MX6_PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK,
	MX6_PAD_DI0_PIN15__IPU2_DI0_PIN15,
	MX6_PAD_DI0_PIN2__IPU2_DI0_PIN02,
	MX6_PAD_DI0_PIN3__IPU2_DI0_PIN03,
	MX6_PAD_DISP0_DAT0__IPU2_DISP0_DATA00,
	MX6_PAD_DISP0_DAT1__IPU2_DISP0_DATA01,
	MX6_PAD_DISP0_DAT2__IPU2_DISP0_DATA02,
	MX6_PAD_DISP0_DAT3__IPU2_DISP0_DATA03,
	MX6_PAD_DISP0_DAT4__IPU2_DISP0_DATA04,
	MX6_PAD_DISP0_DAT5__IPU2_DISP0_DATA05,
	MX6_PAD_DISP0_DAT6__IPU2_DISP0_DATA06,
	MX6_PAD_DISP0_DAT7__IPU2_DISP0_DATA07,
	MX6_PAD_DISP0_DAT8__IPU2_DISP0_DATA08,
	MX6_PAD_DISP0_DAT9__IPU2_DISP0_DATA09,
	MX6_PAD_DISP0_DAT10__IPU2_DISP0_DATA10,
	MX6_PAD_DISP0_DAT11__IPU2_DISP0_DATA11,
	MX6_PAD_DISP0_DAT12__IPU2_DISP0_DATA12,
	MX6_PAD_DISP0_DAT13__IPU2_DISP0_DATA13,
	MX6_PAD_DISP0_DAT14__IPU2_DISP0_DATA14,
	MX6_PAD_DISP0_DAT15__IPU2_DISP0_DATA15,
#endif
};

struct display_info_t {
	int	bus;
	int	addr;
	int	pixfmt;
	int	(*detect)(struct display_info_t const *dev);
	void	(*enable)(struct display_info_t const *dev);
	struct	fb_videomode mode;
};


static int detect_hdmi(struct display_info_t const *dev)
{
	struct hdmi_regs *hdmi	= (struct hdmi_regs *)HDMI_ARB_BASE_ADDR;
	return readb(&hdmi->phy_stat0) & HDMI_DVI_STAT;
}

static void do_enable_hdmi(struct display_info_t const *dev)
{
	imx_enable_hdmi_phy();
}

static int detect_i2c(struct display_info_t const *dev)
{
	return ((0 == i2c_set_bus_num(dev->bus))
		&&
		(0 == i2c_probe(dev->addr)));
}

static void enable_lvds(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)
				IOMUXC_BASE_ADDR;
	u32 reg = readl(&iomux->gpr[2]);
	reg |= IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT;
	writel(reg, &iomux->gpr[2]);
	gpio_direction_output(RGB_BACKLIGHT_GP, 1);
	gpio_direction_output(RGB_BACKLIGHTPWM_GP, 0);
	gpio_direction_output(RGB_BACKLIGHTPWM_OE, 0);
}

static void enable_rgb(struct display_info_t const *dev)
{
	imx_iomux_v3_setup_multiple_pads(
		rgb_pads,
		ARRAY_SIZE(rgb_pads));
	gpio_direction_output(RGB_BACKLIGHT_GP, 1);
	gpio_direction_output(RGB_BACKLIGHTPWM_GP, 0);
	gpio_direction_output(RGB_BACKLIGHTPWM_OE, 0);
}

static void enable_vga(struct display_info_t const *dev)
{
	imx_iomux_v3_setup_multiple_pads(
		vga_pads,
		ARRAY_SIZE(vga_pads));
	gpio_direction_output(VGA_PSAVE_NOT_GP, 1);
}

static struct display_info_t const displays[] = {{
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_hdmi,
	.enable	= do_enable_hdmi,
	.mode	= {
		.name           = "HDMI",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_LVDS666,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "Hannstar-XGA",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_LVDS666,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "wsvga-lvds",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 600,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB666,
	.detect	= detect_i2c,
	.enable	= enable_rgb,
	.mode	= {
		.name           = "wvga-rgb",
		.refresh        = 57,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 37037,
		.left_margin    = 40,
		.right_margin   = 60,
		.upper_margin   = 10,
		.lower_margin   = 10,
		.hsync_len      = 20,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB565,
	.enable	= enable_vga,
	.mode	= {
		.name           = "xga-analog-rgb",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
} } };

int board_video_skip(void)
{
	int i;
	int ret;
	char const *panel = getenv("panel");
	if (!panel) {
		for (i = 0; i < ARRAY_SIZE(displays); i++) {
			struct display_info_t const *dev = displays+i;
			if (dev->detect) {
				if (dev->detect(dev)) {
					panel = dev->mode.name;
					printf("auto-detected panel %s\n", panel);
					break;
				}
			}
			else {
				/* assume connected */
				panel = dev->mode.name;
				printf("auto-detected panel %s\n", panel);
				break;
			}
		}
		if (!panel) {
			panel = displays[0].mode.name;
			printf("No panel detected: default to %s\n", panel);
			i = 0;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(displays); i++) {
			if (!strcmp(panel, displays[i].mode.name))
				break;
		}
	}
	if (i < ARRAY_SIZE(displays)) {
		ret = ipuv3_fb_init(&displays[i].mode, 0,
				    displays[i].pixfmt);
		if (!ret) {
			displays[i].enable(displays+i);
			printf("Display: %s (%ux%u)\n",
			       displays[i].mode.name,
			       displays[i].mode.xres,
			       displays[i].mode.yres);
		} else {
			printf("LCD %s cannot be configured: %d\n",
			       displays[i].mode.name, ret);
		}
	} else {
		printf("unsupported panel %s\n", panel);
		ret = -EINVAL;
	}
	return (0 != ret);
}

static void setup_display(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int reg;

	enable_ipu_clock();
	imx_setup_hdmi();
	/* Turn on LDB0,IPU,IPU DI0 clocks */
	reg = __raw_readl(&mxc_ccm->CCGR3);
	reg |=  MXC_CCM_CCGR3_LDB_DI0_MASK;
	writel(reg, &mxc_ccm->CCGR3);

	/* set LDB0, LDB1 clk select to 011/011 */
	reg = readl(&mxc_ccm->cs2cdr);
	reg &= ~(MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK
		 |MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_MASK);
	reg |= (3<<MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET)
	      |(3<<MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->cs2cdr);

	reg = readl(&mxc_ccm->cscmr2);
	reg |= MXC_CCM_CSCMR2_LDB_DI0_IPU_DIV;
	writel(reg, &mxc_ccm->cscmr2);

	reg = readl(&mxc_ccm->chsccdr);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<<MXC_CCM_CHSCCDR_IPU1_DI0_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->chsccdr);

	reg = IOMUXC_GPR2_BGREF_RRMODE_EXTERNAL_RES
	     |IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_HIGH
	     |IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW
	     |IOMUXC_GPR2_BIT_MAPPING_CH1_SPWG
	     |IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT
	     |IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG
	     |IOMUXC_GPR2_DATA_WIDTH_CH0_18BIT
	     |IOMUXC_GPR2_LVDS_CH1_MODE_DISABLED
	     |IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0;
	writel(reg, &iomux->gpr[2]);

	reg = readl(&iomux->gpr[3]);
	reg = (reg & ~(IOMUXC_GPR3_LVDS0_MUX_CTL_MASK
			|IOMUXC_GPR3_HDMI_MUX_CTL_MASK))
	    | (IOMUXC_GPR3_MUX_SRC_IPU1_DI0
	       <<IOMUXC_GPR3_LVDS0_MUX_CTL_OFFSET);
	writel(reg, &iomux->gpr[3]);

	/* backlights unconditionally on for now */
	imx_iomux_v3_setup_multiple_pads(backlight_pads,
					 ARRAY_SIZE(backlight_pads));
	/* use 0 for EDT 7", use 1 for LG fullHD panel */
	gpio_direction_output(RGB_BACKLIGHTPWM_GP, 0);
	gpio_direction_output(RGB_BACKLIGHTPWM_OE, 0);
	gpio_direction_output(RGB_BACKLIGHT_GP, 1);
}
#endif /* defined(CONFIG_VIDEO_IPUV3) */

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(pwr_intb_pads,
					ARRAY_SIZE(pwr_intb_pads));
	setup_iomux_uart();

#if defined(CONFIG_VIDEO_IPUV3)
	setup_display();
#endif
	return 0;
}

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info_loc);
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info3);

	(void) pmic_init();

#ifdef CONFIG_CMD_SATA
	setup_sata();
#endif

	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_TRDX_CFG_BLOCK
	char env_str[256];

	int i;
	unsigned size = 0;

	char *addr_str, *end;
	unsigned char bi_enetaddr[6]	= {0, 0, 0, 0, 0, 0};	/* Ethernet
								   address */
	unsigned char *mac_addr;
	unsigned char mac_addr00[6]	= {0, 0, 0, 0, 0, 0};

	struct mmc *mmc;

	unsigned char toradex_oui[3]	= { 0x00, 0x14, 0x2d };
	int valid			= 0;

	int ret;
	/* Read production parameter config block from eMMC */
	mmc = find_mmc_device(0);
	/* Just reading one 512 byte block */
	ret = mmc->block_dev.block_read(0, (CONFIG_TRDX_CFG_BLOCK_OFFSET / 512), 1,
					(unsigned char *)config_block);
	if (ret == 1) {
		ret = 0;
		size = 512;
	}

	/* Check validity */
	if ((ret == 0) && (size > 0)) {
		mac_addr = config_block + 8;
		if (!(memcmp(mac_addr, toradex_oui, 3)))
			valid = 1;
	}

	if (!valid) {
		printf("Missing Apalis config block\n");
		memset((void *)config_block, 0, size);
	} else {
		/* Get MAC address from environment */
		addr_str = getenv("ethaddr");
		if (addr_str != NULL) {
			for (i = 0; i < 6; i++) {
				bi_enetaddr[i] = addr_str ? simple_strtoul(
						addr_str, &end, 16) : 0;
				if (addr_str)
					addr_str = (*end) ? end + 1 : end;
			}
		}

		/* Set Ethernet MAC address from config block if not already set
		 */
		if (memcmp(mac_addr00, bi_enetaddr, 6) == 0) {
			sprintf(env_str, "%02x:%02x:%02x:%02x:%02x:%02x",
				mac_addr[0], mac_addr[1], mac_addr[2],
				mac_addr[3], mac_addr[4], mac_addr[5]);
			setenv("ethaddr", env_str);
#ifndef CONFIG_ENV_IS_NOWHERE
			saveenv();
#endif
		}
	}
#endif /* CONFIG_TRDX_CFG_BLOCK */
	return 0;
}
#endif /* CONFIG_BOARD_LATE_INIT */

/* i.MX6 uses the 'standard' board revision for things, i.e.
   video decoding no longer works.
   so don't interfere with the Apalis iMX6 HW Revision */
#if 0
#ifdef CONFIG_REVISION_TAG
u32 get_board_rev(void)
{
#ifdef CONFIG_BOARD_LATE_INIT
	int i;
	unsigned short major = 0, minor = 0, release = 0;
	size_t size = 4096;

	if (config_block == NULL)
		return 0;

	/* Parse revision information in config block */
	for (i = 0; i < (size - 8); i++) {
		if (config_block[i] == 0x02 && config_block[i+1] == 0x40 &&
				config_block[i+2] == 0x08) {
			break;
		}
	}

	major = (config_block[i+3] << 8) | config_block[i+4];
	minor = (config_block[i+5] << 8) | config_block[i+6];
	release = (config_block[i+7] << 8) | config_block[i+8];

	/* Check validity */
	if (major)
		return ((major & 0xff) << 8) | ((minor & 0xf) << 4) |
		       ((release & 0xf) + 0xa);
	else
		return 0;
#else
	return 0;
#endif /* CONFIG_BOARD_LATE_INIT */
}
#endif /* CONFIG_REVISION_TAG */
#endif

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
#ifdef CONFIG_BOARD_LATE_INIT
	int array[8];
	int i;
	unsigned int serial		= 0;
	unsigned int serial_offset	= 11;

	if (config_block == NULL) {
		serialnr->low = 0;
		serialnr->high = 0;
		return;
	}

	/* Get MAC address from config block */
	memcpy(&serial, config_block + serial_offset, 3);
	serial = ntohl(serial);
	serial >>= 8;

	/* Check validity */
	if (serial) {
		/* Convert to Linux serial number format (hexadecimal coded
		   decimal) */
		i = 7;
		while (serial) {
			array[i--] = serial % 10;
			serial /= 10;
		}
		while (i >= 0)
			array[i--] = 0;
		serial = array[0];
		for (i = 1; i < 8; i++) {
			serial *= 16;
			serial += array[i];
		}
	}

	serialnr->low = serial;
#else
	serialnr->low = 0;
#endif /* CONFIG_BOARD_LATE_INIT */
	serialnr->high = 0;
}
#endif /* CONFIG_SERIAL_TAG */

int checkboard(void)
{
	puts("Board: Toradex Apalis iMX6\n");
	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"mmc0",	MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	{"mmc1",	MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{NULL,		0},
};
#endif

int misc_init_r(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif
	return 0;
}