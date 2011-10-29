/* linux/arch/arm/mach-msm/board-mecha-panel.c
 *
 * Copyright (c) 2010 HTC.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/io.h>
#include <asm/mach-types.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <mach/vreg.h>
#include <mach/msm_fb.h>
#include <mach/msm_iomap.h>
#include <mach/atmega_microp.h>

#include "pmic.h"
#include "board-mecha.h"
#include "devices.h"
#include "proc_comm.h"
#include "../../../drivers/video/msm/mdp_hw.h"

#define DEBUG_LCM

#ifdef DEBUG_LCM
#define LCMDBG(fmt, arg...)     printk("[lcm]%s"fmt, __func__, ##arg)
#else
#define LCMDBG(fmt, arg...)     {}
#endif
#define	BRIGHTNESS_DEFAULT_LEVEL	102
enum {
	PANEL_AUO,
	PANEL_SHARP,
	PANEL_SHARP565,
	PANEL_UNKNOW
};

enum {
	CABC_STATE = 1 << 0,
};

static unsigned long status = 0;
static int color_enhancement = 0;
int qspi_send_16bit(unsigned char id, unsigned data);
int qspi_send_9bit(struct spi_msg *msg);
static uint8_t last_val = BRIGHTNESS_DEFAULT_LEVEL;
extern int panel_type;
int panel_rev = 0;
static DEFINE_MUTEX(panel_lock);
static struct vreg *vreg_lcm_1v8, *vreg_lcm_2v8;
#define LCM_MDELAY	0xFF

#define LCM_GPIO_CFG(gpio, func) \
PCOM_GPIO_CFG(gpio, func, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_4MA)
#define LCM_GPIO_CFG_I(gpio, func) \
PCOM_GPIO_CFG(gpio, func, GPIO_INPUT, GPIO_NO_PULL, GPIO_4MA)

static uint32_t display_on_gpio_table[] = {
	LCM_GPIO_CFG(MECHA_LCD_PCLK, 1),
	LCM_GPIO_CFG(MECHA_LCD_DE, 1),
	LCM_GPIO_CFG(MECHA_LCD_VSYNC, 1),
	LCM_GPIO_CFG(MECHA_LCD_HSYNC, 1),
	LCM_GPIO_CFG(MECHA_LCD_G0, 1),
	LCM_GPIO_CFG(MECHA_LCD_G1, 1),
	LCM_GPIO_CFG(MECHA_LCD_G2, 1),
	LCM_GPIO_CFG(MECHA_LCD_G3, 1),
	LCM_GPIO_CFG(MECHA_LCD_G4, 1),
	LCM_GPIO_CFG(MECHA_LCD_G5, 1),
	LCM_GPIO_CFG(MECHA_LCD_G6, 1),
	LCM_GPIO_CFG(MECHA_LCD_G7, 1),
	LCM_GPIO_CFG(MECHA_LCD_B0, 1),
	LCM_GPIO_CFG(MECHA_LCD_B1, 1),
	LCM_GPIO_CFG(MECHA_LCD_B2, 1),
	LCM_GPIO_CFG(MECHA_LCD_B3, 1),
	LCM_GPIO_CFG(MECHA_LCD_B4, 1),
	LCM_GPIO_CFG(MECHA_LCD_B5, 1),
	LCM_GPIO_CFG(MECHA_LCD_B6, 1),
	LCM_GPIO_CFG(MECHA_LCD_B7, 1),
	LCM_GPIO_CFG(MECHA_LCD_R0, 1),
	LCM_GPIO_CFG(MECHA_LCD_R1, 1),
	LCM_GPIO_CFG(MECHA_LCD_R2, 1),
	LCM_GPIO_CFG(MECHA_LCD_R3, 1),
	LCM_GPIO_CFG(MECHA_LCD_R4, 1),
	LCM_GPIO_CFG(MECHA_LCD_R5, 1),
	LCM_GPIO_CFG(MECHA_LCD_R6, 1),
	LCM_GPIO_CFG(MECHA_LCD_R7, 1),
};

static uint32_t display_off_gpio_table[] = {
	LCM_GPIO_CFG(MECHA_LCD_PCLK, 0),
	LCM_GPIO_CFG(MECHA_LCD_DE, 0),
	LCM_GPIO_CFG(MECHA_LCD_VSYNC, 0),
	LCM_GPIO_CFG(MECHA_LCD_HSYNC, 0),
	LCM_GPIO_CFG_I(MECHA_LCD_G0, 0),
	LCM_GPIO_CFG(MECHA_LCD_G1, 0),
	LCM_GPIO_CFG(MECHA_LCD_G2, 0),
	LCM_GPIO_CFG(MECHA_LCD_G3, 0),
	LCM_GPIO_CFG(MECHA_LCD_G4, 0),
	LCM_GPIO_CFG(MECHA_LCD_G5, 0),
	LCM_GPIO_CFG(MECHA_LCD_G6, 0),
	LCM_GPIO_CFG(MECHA_LCD_G7, 0),
	LCM_GPIO_CFG_I(MECHA_LCD_B0, 0),
	LCM_GPIO_CFG(MECHA_LCD_B1, 0),
	LCM_GPIO_CFG(MECHA_LCD_B2, 0),
	LCM_GPIO_CFG(MECHA_LCD_B3, 0),
	LCM_GPIO_CFG(MECHA_LCD_B4, 0),
	LCM_GPIO_CFG(MECHA_LCD_B5, 0),
	LCM_GPIO_CFG(MECHA_LCD_B6, 0),
	LCM_GPIO_CFG(MECHA_LCD_B7, 0),
	LCM_GPIO_CFG(MECHA_LCD_R0, 0),
	LCM_GPIO_CFG(MECHA_LCD_R1, 0),
	LCM_GPIO_CFG(MECHA_LCD_R2, 0),
	LCM_GPIO_CFG(MECHA_LCD_R3, 0),
	LCM_GPIO_CFG(MECHA_LCD_R4, 0),
	LCM_GPIO_CFG(MECHA_LCD_R5, 0),
	LCM_GPIO_CFG(MECHA_LCD_R6, 0),
	LCM_GPIO_CFG(MECHA_LCD_R7, 0),
};

static int panel_gpio_switch(int on)
{
	config_gpio_table(
		!!on ? display_on_gpio_table : display_off_gpio_table,
		ARRAY_SIZE(display_on_gpio_table));

	return 0;
}

static void mecha_sharp_panel_power(bool on_off)
{
	if (!!on_off) {
		LCMDBG("%s(%d):\n", __func__, on_off);
		gpio_set_value(MECHA_LCD_RSTz, 1);
		udelay(500);
		gpio_set_value(MECHA_LCD_RSTz, 0);
		udelay(500);
		gpio_set_value(MECHA_LCD_RSTz, 1);
		hr_msleep(20);
		panel_gpio_switch(on_off);
	} else {
		LCMDBG("%s(%d):\n", __func__, on_off);
		gpio_set_value(MECHA_LCD_RSTz, 1);
		hr_msleep(70);
		panel_gpio_switch(on_off);
	}
}

static void mecha_auo_panel_power(bool on_off)
{
	if (!!on_off) {
		LCMDBG("%s(%d):\n", __func__, on_off);
		gpio_set_value(MECHA_LCD_RSTz, 1);
		udelay(500);
		gpio_set_value(MECHA_LCD_RSTz, 0);
		udelay(500);
		gpio_set_value(MECHA_LCD_RSTz, 1);
		hr_msleep(20);
		panel_gpio_switch(on_off);
	} else {
		LCMDBG("%s(%d):\n", __func__, on_off);
		gpio_set_value(MECHA_LCD_RSTz, 1);
		hr_msleep(70);
		panel_gpio_switch(on_off);
	}
}

struct mdp_reg mecha_sharp_mdp_init_lut[] = {
	{0x94800, 0x000000, 0x0},
	{0x94804, 0x000000, 0x0},
	{0x94808, 0x010101, 0x0},
	{0x9480C, 0x010101, 0x0},
	{0x94810, 0x020202, 0x0},
	{0x94814, 0x030303, 0x0},
	{0x94818, 0x030403, 0x0},
	{0x9481C, 0x040404, 0x0},
	{0x94820, 0x050505, 0x0},
	{0x94824, 0x050605, 0x0},
	{0x94828, 0x060706, 0x0},
	{0x9482C, 0x070807, 0x0},
	{0x94830, 0x080807, 0x0},
	{0x94834, 0x080908, 0x0},
	{0x94838, 0x090A09, 0x0},
	{0x9483C, 0x0A0B0A, 0x0},
	{0x94840, 0x0B0C0A, 0x0},
	{0x94844, 0x0B0C0B, 0x0},
	{0x94848, 0x0C0D0C, 0x0},
	{0x9484C, 0x0D0E0D, 0x0},
	{0x94850, 0x0E0F0D, 0x0},
	{0x94854, 0x0F0F0E, 0x0},
	{0x94858, 0x0F100F, 0x0},
	{0x9485C, 0x101110, 0x0},
	{0x94860, 0x111210, 0x0},
	{0x94864, 0x121211, 0x0},
	{0x94868, 0x121312, 0x0},
	{0x9486C, 0x131413, 0x0},
	{0x94870, 0x141514, 0x0},
	{0x94874, 0x151514, 0x0},
	{0x94878, 0x161615, 0x0},
	{0x9487C, 0x161716, 0x0},
	{0x94880, 0x171817, 0x0},
	{0x94884, 0x181818, 0x0},
	{0x94888, 0x191918, 0x0},
	{0x9488C, 0x1A1A19, 0x0},
	{0x94890, 0x1A1A1A, 0x0},
	{0x94894, 0x1B1B1B, 0x0},
	{0x94898, 0x1C1C1C, 0x0},
	{0x9489C, 0x1D1D1C, 0x0},
	{0x948A0, 0x1E1D1D, 0x0},
	{0x948A4, 0x1F1E1E, 0x0},
	{0x948A8, 0x1F1F1F, 0x0},
	{0x948AC, 0x201F20, 0x0},
	{0x948B0, 0x212020, 0x0},
	{0x948B4, 0x222121, 0x0},
	{0x948B8, 0x232222, 0x0},
	{0x948BC, 0x242223, 0x0},
	{0x948C0, 0x242324, 0x0},
	{0x948C4, 0x252424, 0x0},
	{0x948C8, 0x262425, 0x0},
	{0x948CC, 0x272526, 0x0},
	{0x948D0, 0x282627, 0x0},
	{0x948D4, 0x292728, 0x0},
	{0x948D8, 0x2A2729, 0x0},
	{0x948DC, 0x2B2829, 0x0},
	{0x948E0, 0x2B292A, 0x0},
	{0x948E4, 0x2C292B, 0x0},
	{0x948E8, 0x2D2A2C, 0x0},
	{0x948EC, 0x2E2B2D, 0x0},
	{0x948F0, 0x2F2C2E, 0x0},
	{0x948F4, 0x302C2E, 0x0},
	{0x948F8, 0x312D2F, 0x0},
	{0x948FC, 0x322E30, 0x0},
	{0x94900, 0x332E31, 0x0},
	{0x94904, 0x342F32, 0x0},
	{0x94908, 0x353033, 0x0},
	{0x9490C, 0x353133, 0x0},
	{0x94910, 0x363134, 0x0},
	{0x94914, 0x373235, 0x0},
	{0x94918, 0x383336, 0x0},
	{0x9491C, 0x393437, 0x0},
	{0x94920, 0x3A3438, 0x0},
	{0x94924, 0x3B3539, 0x0},
	{0x94928, 0x3C3639, 0x0},
	{0x9492C, 0x3D373A, 0x0},
	{0x94930, 0x3E373B, 0x0},
	{0x94934, 0x3F383C, 0x0},
	{0x94938, 0x40393D, 0x0},
	{0x9493C, 0x413A3E, 0x0},
	{0x94940, 0x423A3F, 0x0},
	{0x94944, 0x433B40, 0x0},
	{0x94948, 0x443C40, 0x0},
	{0x9494C, 0x453D41, 0x0},
	{0x94950, 0x463E42, 0x0},
	{0x94954, 0x473E43, 0x0},
	{0x94958, 0x483F44, 0x0},
	{0x9495C, 0x494045, 0x0},
	{0x94960, 0x4A4146, 0x0},
	{0x94964, 0x4B4247, 0x0},
	{0x94968, 0x4C4247, 0x0},
	{0x9496C, 0x4D4348, 0x0},
	{0x94970, 0x4F4449, 0x0},
	{0x94974, 0x50454A, 0x0},
	{0x94978, 0x51464B, 0x0},
	{0x9497C, 0x52464C, 0x0},
	{0x94980, 0x53474D, 0x0},
	{0x94984, 0x54484E, 0x0},
	{0x94988, 0x55494F, 0x0},
	{0x9498C, 0x564A50, 0x0},
	{0x94990, 0x574B50, 0x0},
	{0x94994, 0x584B51, 0x0},
	{0x94998, 0x594C52, 0x0},
	{0x9499C, 0x5A4D53, 0x0},
	{0x949A0, 0x5C4E54, 0x0},
	{0x949A4, 0x5D4F55, 0x0},
	{0x949A8, 0x5E5056, 0x0},
	{0x949AC, 0x5F5157, 0x0},
	{0x949B0, 0x605158, 0x0},
	{0x949B4, 0x615259, 0x0},
	{0x949B8, 0x62535A, 0x0},
	{0x949BC, 0x63545B, 0x0},
	{0x949C0, 0x64555C, 0x0},
	{0x949C4, 0x66565D, 0x0},
	{0x949C8, 0x67575E, 0x0},
	{0x949CC, 0x68585E, 0x0},
	{0x949D0, 0x69585F, 0x0},
	{0x949D4, 0x6A5960, 0x0},
	{0x949D8, 0x6B5A61, 0x0},
	{0x949DC, 0x6C5B62, 0x0},
	{0x949E0, 0x6E5C63, 0x0},
	{0x949E4, 0x6F5D64, 0x0},
	{0x949E8, 0x705E65, 0x0},
	{0x949EC, 0x715F66, 0x0},
	{0x949F0, 0x726067, 0x0},
	{0x949F4, 0x736168, 0x0},
	{0x949F8, 0x746269, 0x0},
	{0x949FC, 0x76626A, 0x0},
	{0x94A00, 0x77636B, 0x0},
	{0x94A04, 0x78646C, 0x0},
	{0x94A08, 0x79656D, 0x0},
	{0x94A0C, 0x7A666E, 0x0},
	{0x94A10, 0x7B676F, 0x0},
	{0x94A14, 0x7C6870, 0x0},
	{0x94A18, 0x7E6971, 0x0},
	{0x94A1C, 0x7F6A72, 0x0},
	{0x94A20, 0x806B73, 0x0},
	{0x94A24, 0x816C74, 0x0},
	{0x94A28, 0x826D75, 0x0},
	{0x94A2C, 0x836E76, 0x0},
	{0x94A30, 0x856F77, 0x0},
	{0x94A34, 0x867078, 0x0},
	{0x94A38, 0x877179, 0x0},
	{0x94A3C, 0x88727A, 0x0},
	{0x94A40, 0x89737B, 0x0},
	{0x94A44, 0x8A747C, 0x0},
	{0x94A48, 0x8B757E, 0x0},
	{0x94A4C, 0x8D767F, 0x0},
	{0x94A50, 0x8E7780, 0x0},
	{0x94A54, 0x8F7881, 0x0},
	{0x94A58, 0x907982, 0x0},
	{0x94A5C, 0x917983, 0x0},
	{0x94A60, 0x927A84, 0x0},
	{0x94A64, 0x937B85, 0x0},
	{0x94A68, 0x957C86, 0x0},
	{0x94A6C, 0x967D87, 0x0},
	{0x94A70, 0x977E88, 0x0},
	{0x94A74, 0x987F89, 0x0},
	{0x94A78, 0x99808A, 0x0},
	{0x94A7C, 0x9A818B, 0x0},
	{0x94A80, 0x9B828D, 0x0},
	{0x94A84, 0x9C838E, 0x0},
	{0x94A88, 0x9E858F, 0x0},
	{0x94A8C, 0x9F8690, 0x0},
	{0x94A90, 0xA08791, 0x0},
	{0x94A94, 0xA18892, 0x0},
	{0x94A98, 0xA28993, 0x0},
	{0x94A9C, 0xA38A94, 0x0},
	{0x94AA0, 0xA48B95, 0x0},
	{0x94AA4, 0xA58C97, 0x0},
	{0x94AA8, 0xA68D98, 0x0},
	{0x94AAC, 0xA78E99, 0x0},
	{0x94AB0, 0xA98F9A, 0x0},
	{0x94AB4, 0xAA909B, 0x0},
	{0x94AB8, 0xAB919C, 0x0},
	{0x94ABC, 0xAC929D, 0x0},
	{0x94AC0, 0xAD939E, 0x0},
	{0x94AC4, 0xAE94A0, 0x0},
	{0x94AC8, 0xAF95A1, 0x0},
	{0x94ACC, 0xB096A2, 0x0},
	{0x94AD0, 0xB197A3, 0x0},
	{0x94AD4, 0xB298A4, 0x0},
	{0x94AD8, 0xB399A5, 0x0},
	{0x94ADC, 0xB49AA7, 0x0},
	{0x94AE0, 0xB59BA8, 0x0},
	{0x94AE4, 0xB69CA9, 0x0},
	{0x94AE8, 0xB79EAA, 0x0},
	{0x94AEC, 0xB89FAB, 0x0},
	{0x94AF0, 0xB9A0AC, 0x0},
	{0x94AF4, 0xBAA1AD, 0x0},
	{0x94AF8, 0xBBA2AF, 0x0},
	{0x94AFC, 0xBCA3B0, 0x0},
	{0x94B00, 0xBDA4B1, 0x0},
	{0x94B04, 0xBEA5B2, 0x0},
	{0x94B08, 0xBFA6B3, 0x0},
	{0x94B0C, 0xC0A7B5, 0x0},
	{0x94B10, 0xC1A9B6, 0x0},
	{0x94B14, 0xC2AAB7, 0x0},
	{0x94B18, 0xC3ABB8, 0x0},
	{0x94B1C, 0xC4ACB9, 0x0},
	{0x94B20, 0xC5ADBA, 0x0},
	{0x94B24, 0xC6AEBC, 0x0},
	{0x94B28, 0xC7AFBD, 0x0},
	{0x94B2C, 0xC8B1BE, 0x0},
	{0x94B30, 0xC9B2BF, 0x0},
	{0x94B34, 0xCAB3C0, 0x0},
	{0x94B38, 0xCBB4C2, 0x0},
	{0x94B3C, 0xCCB5C3, 0x0},
	{0x94B40, 0xCDB6C4, 0x0},
	{0x94B44, 0xCEB8C5, 0x0},
	{0x94B48, 0xCFB9C6, 0x0},
	{0x94B4C, 0xCFBAC8, 0x0},
	{0x94B50, 0xD0BBC9, 0x0},
	{0x94B54, 0xD1BDCA, 0x0},
	{0x94B58, 0xD2BECB, 0x0},
	{0x94B5C, 0xD3BFCC, 0x0},
	{0x94B60, 0xD4C0CE, 0x0},
	{0x94B64, 0xD5C2CF, 0x0},
	{0x94B68, 0xD6C3D0, 0x0},
	{0x94B6C, 0xD7C4D1, 0x0},
	{0x94B70, 0xD8C5D2, 0x0},
	{0x94B74, 0xD9C7D3, 0x0},
	{0x94B78, 0xD9C8D5, 0x0},
	{0x94B7C, 0xDAC9D6, 0x0},
	{0x94B80, 0xDBCBD7, 0x0},
	{0x94B84, 0xDCCCD8, 0x0},
	{0x94B88, 0xDDCED9, 0x0},
	{0x94B8C, 0xDECFDB, 0x0},
	{0x94B90, 0xDFD0DC, 0x0},
	{0x94B94, 0xE0D2DD, 0x0},
	{0x94B98, 0xE1D3DE, 0x0},
	{0x94B9C, 0xE1D5DF, 0x0},
	{0x94BA0, 0xE2D6E0, 0x0},
	{0x94BA4, 0xE3D8E2, 0x0},
	{0x94BA8, 0xE4D9E3, 0x0},
	{0x94BAC, 0xE5DBE4, 0x0},
	{0x94BB0, 0xE6DCE5, 0x0},
	{0x94BB4, 0xE7DEE6, 0x0},
	{0x94BB8, 0xE8DFE7, 0x0},
	{0x94BBC, 0xE9E1E8, 0x0},
	{0x94BC0, 0xEAE2EA, 0x0},
	{0x94BC4, 0xEAE4EB, 0x0},
	{0x94BC8, 0xEBE6EC, 0x0},
	{0x94BCC, 0xECE7ED, 0x0},
	{0x94BD0, 0xEDE9EE, 0x0},
	{0x94BD4, 0xEEEBEF, 0x0},
	{0x94BD8, 0xEFECF0, 0x0},
	{0x94BDC, 0xF0EEF1, 0x0},
	{0x94BE0, 0xF1F0F3, 0x0},
	{0x94BE4, 0xF2F2F4, 0x0},
	{0x94BE8, 0xF3F4F5, 0x0},
	{0x94BEC, 0xF4F6F6, 0x0},
	{0x94BF0, 0xF5F7F7, 0x0},
	{0x94BF4, 0xF6F9F8, 0x0},
	{0x94BF8, 0xF7FBF9, 0x0},
	{0x94BFC, 0xF8FDFA, 0x0},
	{0x90070, 0x000017, 0x17},
};
struct mdp_reg mecha_sharp_mdp_init_color[] = {
	{0x90070, 0x0008, 0x8},
	{0x93400, 0x021A, 0x0},
	{0x93404, 0xFFD9, 0x0},
	{0x93408, 0x000F, 0x0},
	{0x9340C, 0xFFD7, 0x0},
	{0x93410, 0x023B, 0x0},
	{0x93414, 0xFFEB, 0x0},
	{0x93418, 0xFFF2, 0x0},
	{0x9341C, 0xFFDB, 0x0},
	{0x93420, 0x0237, 0x0},

	{0x93600, 0x0000, 0x0},
	{0x93604, 0x00FF, 0x0},
	{0x93608, 0x0000, 0x0},
	{0x9360C, 0x00FF, 0x0},
	{0x93610, 0x0000, 0x0},
	{0x93614, 0x00FF, 0x0},
	{0x93680, 0x0000, 0x0},
	{0x93684, 0x00FF, 0x0},
	{0x93688, 0x0000, 0x0},
	{0x9368C, 0x00FF, 0x0},
	{0x93690, 0x0000, 0x0},
	{0x93694, 0x00FF, 0x0},
};
struct mdp_reg mecha_auo_mdp_init_lut[] = {
	{0x94800, 0x000000, 0x0},
	{0x94804, 0x000000, 0x0},
	{0x94808, 0x000101, 0x0},
	{0x9480C, 0x010201, 0x0},
	{0x94810, 0x020202, 0x0},
	{0x94814, 0x030303, 0x0},
	{0x94818, 0x030404, 0x0},
	{0x9481C, 0x040404, 0x0},
	{0x94820, 0x050505, 0x0},
	{0x94824, 0x050506, 0x0},
	{0x94828, 0x060607, 0x0},
	{0x9482C, 0x070707, 0x0},
	{0x94830, 0x080708, 0x0},
	{0x94834, 0x090809, 0x0},
	{0x94838, 0x09090A, 0x0},
	{0x9483C, 0x0A090A, 0x0},
	{0x94840, 0x0B0A0B, 0x0},
	{0x94844, 0x0C0B0C, 0x0},
	{0x94848, 0x0D0B0D, 0x0},
	{0x9484C, 0x0D0C0D, 0x0},
	{0x94850, 0x0E0C0E, 0x0},
	{0x94854, 0x0F0D0F, 0x0},
	{0x94858, 0x100E10, 0x0},
	{0x9485C, 0x110E10, 0x0},
	{0x94860, 0x110F11, 0x0},
	{0x94864, 0x121012, 0x0},
	{0x94868, 0x131013, 0x0},
	{0x9486C, 0x141114, 0x0},
	{0x94870, 0x151114, 0x0},
	{0x94874, 0x161215, 0x0},
	{0x94878, 0x161316, 0x0},
	{0x9487C, 0x171317, 0x0},
	{0x94880, 0x181417, 0x0},
	{0x94884, 0x191518, 0x0},
	{0x94888, 0x1A1519, 0x0},
	{0x9488C, 0x1B161A, 0x0},
	{0x94890, 0x1C161A, 0x0},
	{0x94894, 0x1C171B, 0x0},
	{0x94898, 0x1D181C, 0x0},
	{0x9489C, 0x1E181D, 0x0},
	{0x948A0, 0x1F191D, 0x0},
	{0x948A4, 0x201A1E, 0x0},
	{0x948A8, 0x211A1F, 0x0},
	{0x948AC, 0x221B20, 0x0},
	{0x948B0, 0x221C20, 0x0},
	{0x948B4, 0x231C21, 0x0},
	{0x948B8, 0x241D22, 0x0},
	{0x948BC, 0x251E23, 0x0},
	{0x948C0, 0x261E23, 0x0},
	{0x948C4, 0x271F24, 0x0},
	{0x948C8, 0x282025, 0x0},
	{0x948CC, 0x282026, 0x0},
	{0x948D0, 0x292127, 0x0},
	{0x948D4, 0x2A2227, 0x0},
	{0x948D8, 0x2B2228, 0x0},
	{0x948DC, 0x2C2329, 0x0},
	{0x948E0, 0x2D242A, 0x0},
	{0x948E4, 0x2E252B, 0x0},
	{0x948E8, 0x2F252B, 0x0},
	{0x948EC, 0x30262C, 0x0},
	{0x948F0, 0x30272D, 0x0},
	{0x948F4, 0x31282E, 0x0},
	{0x948F8, 0x32282F, 0x0},
	{0x948FC, 0x33292F, 0x0},
	{0x94900, 0x342A30, 0x0},
	{0x94904, 0x352B31, 0x0},
	{0x94908, 0x362B32, 0x0},
	{0x9490C, 0x372C33, 0x0},
	{0x94910, 0x382D34, 0x0},
	{0x94914, 0x382E34, 0x0},
	{0x94918, 0x392F35, 0x0},
	{0x9491C, 0x3A2F36, 0x0},
	{0x94920, 0x3B3037, 0x0},
	{0x94924, 0x3C3138, 0x0},
	{0x94928, 0x3D3239, 0x0},
	{0x9492C, 0x3E333A, 0x0},
	{0x94930, 0x3F343B, 0x0},
	{0x94934, 0x40343B, 0x0},
	{0x94938, 0x41353C, 0x0},
	{0x9493C, 0x42363D, 0x0},
	{0x94940, 0x43373E, 0x0},
	{0x94944, 0x44383F, 0x0},
	{0x94948, 0x443940, 0x0},
	{0x9494C, 0x453A41, 0x0},
	{0x94950, 0x463B42, 0x0},
	{0x94954, 0x473C43, 0x0},
	{0x94958, 0x483C44, 0x0},
	{0x9495C, 0x493D44, 0x0},
	{0x94960, 0x4A3E45, 0x0},
	{0x94964, 0x4B3F46, 0x0},
	{0x94968, 0x4C4047, 0x0},
	{0x9496C, 0x4D4148, 0x0},
	{0x94970, 0x4E4249, 0x0},
	{0x94974, 0x4F434A, 0x0},
	{0x94978, 0x50444B, 0x0},
	{0x9497C, 0x51454C, 0x0},
	{0x94980, 0x52464D, 0x0},
	{0x94984, 0x53474E, 0x0},
	{0x94988, 0x54484F, 0x0},
	{0x9498C, 0x554950, 0x0},
	{0x94990, 0x564A51, 0x0},
	{0x94994, 0x574B52, 0x0},
	{0x94998, 0x584C53, 0x0},
	{0x9499C, 0x594D54, 0x0},
	{0x949A0, 0x5A4E55, 0x0},
	{0x949A4, 0x5B4F56, 0x0},
	{0x949A8, 0x5B5057, 0x0},
	{0x949AC, 0x5C5158, 0x0},
	{0x949B0, 0x5D5259, 0x0},
	{0x949B4, 0x5E535A, 0x0},
	{0x949B8, 0x5F545B, 0x0},
	{0x949BC, 0x60555C, 0x0},
	{0x949C0, 0x61565D, 0x0},
	{0x949C4, 0x62575E, 0x0},
	{0x949C8, 0x63595F, 0x0},
	{0x949CC, 0x645A60, 0x0},
	{0x949D0, 0x665B61, 0x0},
	{0x949D4, 0x675C62, 0x0},
	{0x949D8, 0x685D63, 0x0},
	{0x949DC, 0x695E64, 0x0},
	{0x949E0, 0x6A5F65, 0x0},
	{0x949E4, 0x6B6066, 0x0},
	{0x949E8, 0x6C6167, 0x0},
	{0x949EC, 0x6D6269, 0x0},
	{0x949F0, 0x6E636A, 0x0},
	{0x949F4, 0x6F646B, 0x0},
	{0x949F8, 0x70666C, 0x0},
	{0x949FC, 0x71676D, 0x0},
	{0x94A00, 0x72686E, 0x0},
	{0x94A04, 0x73696F, 0x0},
	{0x94A08, 0x746A70, 0x0},
	{0x94A0C, 0x756B71, 0x0},
	{0x94A10, 0x766C72, 0x0},
	{0x94A14, 0x776D73, 0x0},
	{0x94A18, 0x786E74, 0x0},
	{0x94A1C, 0x796F76, 0x0},
	{0x94A20, 0x7A7177, 0x0},
	{0x94A24, 0x7B7278, 0x0},
	{0x94A28, 0x7C7379, 0x0},
	{0x94A2C, 0x7D747A, 0x0},
	{0x94A30, 0x7E757B, 0x0},
	{0x94A34, 0x80767C, 0x0},
	{0x94A38, 0x81777D, 0x0},
	{0x94A3C, 0x82787E, 0x0},
	{0x94A40, 0x83797F, 0x0},
	{0x94A44, 0x847B80, 0x0},
	{0x94A48, 0x857C82, 0x0},
	{0x94A4C, 0x867D83, 0x0},
	{0x94A50, 0x877E84, 0x0},
	{0x94A54, 0x887F85, 0x0},
	{0x94A58, 0x898086, 0x0},
	{0x94A5C, 0x8A8187, 0x0},
	{0x94A60, 0x8B8288, 0x0},
	{0x94A64, 0x8C8389, 0x0},
	{0x94A68, 0x8E848A, 0x0},
	{0x94A6C, 0x8F858C, 0x0},
	{0x94A70, 0x90878D, 0x0},
	{0x94A74, 0x91888E, 0x0},
	{0x94A78, 0x92898F, 0x0},
	{0x94A7C, 0x938A90, 0x0},
	{0x94A80, 0x948B91, 0x0},
	{0x94A84, 0x958C92, 0x0},
	{0x94A88, 0x968D93, 0x0},
	{0x94A8C, 0x978E94, 0x0},
	{0x94A90, 0x998F95, 0x0},
	{0x94A94, 0x9A9097, 0x0},
	{0x94A98, 0x9B9198, 0x0},
	{0x94A9C, 0x9C9299, 0x0},
	{0x94AA0, 0x9D939A, 0x0},
	{0x94AA4, 0x9E949B, 0x0},
	{0x94AA8, 0x9F959C, 0x0},
	{0x94AAC, 0xA0969D, 0x0},
	{0x94AB0, 0xA1979E, 0x0},
	{0x94AB4, 0xA3999F, 0x0},
	{0x94AB8, 0xA49AA0, 0x0},
	{0x94ABC, 0xA59BA2, 0x0},
	{0x94AC0, 0xA69CA3, 0x0},
	{0x94AC4, 0xA79DA4, 0x0},
	{0x94AC8, 0xA89EA5, 0x0},
	{0x94ACC, 0xA99FA6, 0x0},
	{0x94AD0, 0xAAA0A7, 0x0},
	{0x94AD4, 0xACA1A8, 0x0},
	{0x94AD8, 0xADA2A9, 0x0},
	{0x94ADC, 0xAEA3AA, 0x0},
	{0x94AE0, 0xAFA4AB, 0x0},
	{0x94AE4, 0xB0A5AC, 0x0},
	{0x94AE8, 0xB1A6AE, 0x0},
	{0x94AEC, 0xB2A7AF, 0x0},
	{0x94AF0, 0xB3A8B0, 0x0},
	{0x94AF4, 0xB5A9B1, 0x0},
	{0x94AF8, 0xB6AAB2, 0x0},
	{0x94AFC, 0xB7ABB3, 0x0},
	{0x94B00, 0xB8ACB4, 0x0},
	{0x94B04, 0xB9ADB5, 0x0},
	{0x94B08, 0xBAAEB6, 0x0},
	{0x94B0C, 0xBBAFB7, 0x0},
	{0x94B10, 0xBDB0B8, 0x0},
	{0x94B14, 0xBEB1BA, 0x0},
	{0x94B18, 0xBFB2BB, 0x0},
	{0x94B1C, 0xC0B3BC, 0x0},
	{0x94B20, 0xC1B4BD, 0x0},
	{0x94B24, 0xC2B5BE, 0x0},
	{0x94B28, 0xC3B6BF, 0x0},
	{0x94B2C, 0xC4B7C0, 0x0},
	{0x94B30, 0xC6B8C1, 0x0},
	{0x94B34, 0xC7B9C2, 0x0},
	{0x94B38, 0xC8BAC3, 0x0},
	{0x94B3C, 0xC9BBC4, 0x0},
	{0x94B40, 0xCABCC5, 0x0},
	{0x94B44, 0xCBBDC7, 0x0},
	{0x94B48, 0xCCBEC8, 0x0},
	{0x94B4C, 0xCEBFC9, 0x0},
	{0x94B50, 0xCFC1CA, 0x0},
	{0x94B54, 0xD0C2CB, 0x0},
	{0x94B58, 0xD1C3CC, 0x0},
	{0x94B5C, 0xD2C4CD, 0x0},
	{0x94B60, 0xD3C5CE, 0x0},
	{0x94B64, 0xD4C6CF, 0x0},
	{0x94B68, 0xD5C7D0, 0x0},
	{0x94B6C, 0xD7C8D2, 0x0},
	{0x94B70, 0xD8C9D3, 0x0},
	{0x94B74, 0xD9CBD4, 0x0},
	{0x94B78, 0xDACCD5, 0x0},
	{0x94B7C, 0xDBCDD6, 0x0},
	{0x94B80, 0xDCCED7, 0x0},
	{0x94B84, 0xDDCFD8, 0x0},
	{0x94B88, 0xDED1DA, 0x0},
	{0x94B8C, 0xE0D2DB, 0x0},
	{0x94B90, 0xE1D3DC, 0x0},
	{0x94B94, 0xE2D4DD, 0x0},
	{0x94B98, 0xE3D6DE, 0x0},
	{0x94B9C, 0xE4D7DF, 0x0},
	{0x94BA0, 0xE5D8E1, 0x0},
	{0x94BA4, 0xE6DAE2, 0x0},
	{0x94BA8, 0xE7DBE3, 0x0},
	{0x94BAC, 0xE9DCE4, 0x0},
	{0x94BB0, 0xEADEE5, 0x0},
	{0x94BB4, 0xEBDFE7, 0x0},
	{0x94BB8, 0xECE1E8, 0x0},
	{0x94BBC, 0xEDE2E9, 0x0},
	{0x94BC0, 0xEEE4EA, 0x0},
	{0x94BC4, 0xEFE5EC, 0x0},
	{0x94BC8, 0xF0E7ED, 0x0},
	{0x94BCC, 0xF1E8EE, 0x0},
	{0x94BD0, 0xF2EAEF, 0x0},
	{0x94BD4, 0xF4ECF1, 0x0},
	{0x94BD8, 0xF5EEF2, 0x0},
	{0x94BDC, 0xF6EFF3, 0x0},
	{0x94BE0, 0xF7F1F5, 0x0},
	{0x94BE4, 0xF8F3F6, 0x0},
	{0x94BE8, 0xF9F5F8, 0x0},
	{0x94BEC, 0xFAF7F9, 0x0},
	{0x94BF0, 0xFBF9FA, 0x0},
	{0x94BF4, 0xFCFBFC, 0x0},
	{0x94BF8, 0xFDFDFD, 0x0},
	{0x94BFC, 0xFEFFFF, 0x0},
	{0x90070, 0x000017,0x17},
};

struct mdp_reg mecha_auo_mdp_init_color[] = {
	{0x90070, 0x0008, 0x8},
	{0x93400, 0x0233, 0x0},
	{0x93404, 0x0015, 0x0},
	{0x93408, 0xFFB8, 0x0},
	{0x9340C, 0xFFF5, 0x0},
	{0x93410, 0x0205, 0x0},
	{0x93414, 0x0006, 0x0},
	{0x93418, 0xFFF7, 0x0},
	{0x9341C, 0xFFFE, 0x0},
	{0x93420, 0x020C, 0x0},

	{0x93600, 0x0000, 0x0},
	{0x93604, 0x00FF, 0x0},
	{0x93608, 0x0000, 0x0},
	{0x9360C, 0x00FF, 0x0},
	{0x93610, 0x0000, 0x0},
	{0x93614, 0x00FF, 0x0},
	{0x93680, 0x0000, 0x0},
	{0x93684, 0x00FF, 0x0},
	{0x93688, 0x0000, 0x0},
	{0x9368C, 0x00FF, 0x0},
	{0x93690, 0x0000, 0x0},
	{0x93694, 0x00FF, 0x0},
};

void mecha_mdp_color_enhancement(struct mdp_device *mdp_dev)
{
	struct mdp_info *mdp = container_of(mdp_dev, struct mdp_info, mdp_dev);
	if (panel_type == PANEL_AUO) {
		mdp->write_regs(mdp, mecha_auo_mdp_init_lut, ARRAY_SIZE(mecha_auo_mdp_init_lut));
		mdp->write_regs(mdp, mecha_auo_mdp_init_color, ARRAY_SIZE(mecha_auo_mdp_init_color));
	} else {
		mdp->write_regs(mdp, mecha_sharp_mdp_init_lut, ARRAY_SIZE(mecha_sharp_mdp_init_lut));
		mdp->write_regs(mdp, mecha_sharp_mdp_init_color, ARRAY_SIZE(mecha_sharp_mdp_init_color));
	}
}

static struct msm_mdp_platform_data mdp_pdata = {
	.color_format = MSM_MDP_OUT_IF_FMT_RGB888,
};

struct lcm_cmd {
        uint8_t		cmd;
        uint8_t		data;
};

/*----------------------------------------------------------------------------*/

static struct lcm_cmd auo_init_seq[] = {
        {0x1, 0xc0}, {0x0, 0x00}, {0x2, 0x86},
        {0x1, 0xc0}, {0x0, 0x01}, {0x2, 0x00},
        {0x1, 0xc0}, {0x0, 0x02}, {0x2, 0x86},
        {0x1, 0xc0}, {0x0, 0x03}, {0x2, 0x00},
        {0x1, 0xc1}, {0x0, 0x00}, {0x2, 0x40},
        {0x1, 0xc2}, {0x0, 0x00}, {0x2, 0x02},
	{LCM_MDELAY, 1},
        {0x1, 0xc2}, {0x0, 0x02}, {0x2, 0x32},
        {0x1, 0x6A}, {0x0, 0x01}, {0x2, 0x00},
        {0x1, 0x6A}, {0x0, 0x02}, {0x2, 0x02},
};

static struct lcm_cmd auo_gamma_normal_seq[] = {
	/* Gamma setting: start */
	{0x1, 0xE0}, {0x0, 0x00}, {0x2, 0x0E},
	{0x1, 0xE0}, {0x0, 0x01}, {0x2, 0x16},
	{0x1, 0xE0}, {0x0, 0x02}, {0x2, 0x24},
	{0x1, 0xE0}, {0x0, 0x03}, {0x2, 0x3B},
	{0x1, 0xE0}, {0x0, 0x04}, {0x2, 0x1B},
	{0x1, 0xE0}, {0x0, 0x05}, {0x2, 0x2D},
	{0x1, 0xE0}, {0x0, 0x06}, {0x2, 0x5F},
	{0x1, 0xE0}, {0x0, 0x07}, {0x2, 0x3E},
	{0x1, 0xE0}, {0x0, 0x08}, {0x2, 0x20},
	{0x1, 0xE0}, {0x0, 0x09}, {0x2, 0x29},
	{0x1, 0xE0}, {0x0, 0x0A}, {0x2, 0x82},
	{0x1, 0xE0}, {0x0, 0x0B}, {0x2, 0x13},
	{0x1, 0xE0}, {0x0, 0x0C}, {0x2, 0x32},
	{0x1, 0xE0}, {0x0, 0x0D}, {0x2, 0x56},
	{0x1, 0xE0}, {0x0, 0x0E}, {0x2, 0x79},
	{0x1, 0xE0}, {0x0, 0x0F}, {0x2, 0xB8},
	{0x1, 0xE0}, {0x0, 0x10}, {0x2, 0x55},
	{0x1, 0xE0}, {0x0, 0x11}, {0x2, 0x57},

	{0x1, 0xE1}, {0x0, 0x00}, {0x2, 0x0E},
	{0x1, 0xE1}, {0x0, 0x01}, {0x2, 0x16},
	{0x1, 0xE1}, {0x0, 0x02}, {0x2, 0x24},
	{0x1, 0xE1}, {0x0, 0x03}, {0x2, 0x3B},
	{0x1, 0xE1}, {0x0, 0x04}, {0x2, 0x1B},
	{0x1, 0xE1}, {0x0, 0x05}, {0x2, 0x2D},
	{0x1, 0xE1}, {0x0, 0x06}, {0x2, 0x5F},
	{0x1, 0xE1}, {0x0, 0x07}, {0x2, 0x3E},
	{0x1, 0xE1}, {0x0, 0x08}, {0x2, 0x20},
	{0x1, 0xE1}, {0x0, 0x09}, {0x2, 0x29},
	{0x1, 0xE1}, {0x0, 0x0A}, {0x2, 0x82},
	{0x1, 0xE1}, {0x0, 0x0B}, {0x2, 0x13},
	{0x1, 0xE1}, {0x0, 0x0C}, {0x2, 0x32},
	{0x1, 0xE1}, {0x0, 0x0D}, {0x2, 0x56},
	{0x1, 0xE1}, {0x0, 0x0E}, {0x2, 0x79},
	{0x1, 0xE1}, {0x0, 0x0F}, {0x2, 0xB8},
	{0x1, 0xE1}, {0x0, 0x10}, {0x2, 0x55},
	{0x1, 0xE1}, {0x0, 0x11}, {0x2, 0x57},

	{0x1, 0xE2}, {0x0, 0x00}, {0x2, 0x0E},
	{0x1, 0xE2}, {0x0, 0x01}, {0x2, 0x16},
	{0x1, 0xE2}, {0x0, 0x02}, {0x2, 0x24},
	{0x1, 0xE2}, {0x0, 0x03}, {0x2, 0x3B},
	{0x1, 0xE2}, {0x0, 0x04}, {0x2, 0x1E},
	{0x1, 0xE2}, {0x0, 0x05}, {0x2, 0x2D},
	{0x1, 0xE2}, {0x0, 0x06}, {0x2, 0x5F},
	{0x1, 0xE2}, {0x0, 0x07}, {0x2, 0x3E},
	{0x1, 0xE2}, {0x0, 0x08}, {0x2, 0x20},
	{0x1, 0xE2}, {0x0, 0x09}, {0x2, 0x29},
	{0x1, 0xE2}, {0x0, 0x0A}, {0x2, 0x82},
	{0x1, 0xE2}, {0x0, 0x0B}, {0x2, 0x13},
	{0x1, 0xE2}, {0x0, 0x0C}, {0x2, 0x32},
	{0x1, 0xE2}, {0x0, 0x0D}, {0x2, 0x56},
	{0x1, 0xE2}, {0x0, 0x0E}, {0x2, 0x79},
	{0x1, 0xE2}, {0x0, 0x0F}, {0x2, 0xB8},
	{0x1, 0xE2}, {0x0, 0x10}, {0x2, 0x55},
	{0x1, 0xE2}, {0x0, 0x11}, {0x2, 0x57},

	{0x1, 0xE3}, {0x0, 0x00}, {0x2, 0x0E},
	{0x1, 0xE3}, {0x0, 0x01}, {0x2, 0x16},
	{0x1, 0xE3}, {0x0, 0x02}, {0x2, 0x24},
	{0x1, 0xE3}, {0x0, 0x03}, {0x2, 0x3B},
	{0x1, 0xE3}, {0x0, 0x04}, {0x2, 0x1E},
	{0x1, 0xE3}, {0x0, 0x05}, {0x2, 0x2D},
	{0x1, 0xE3}, {0x0, 0x06}, {0x2, 0x5F},
	{0x1, 0xE3}, {0x0, 0x07}, {0x2, 0x3E},
	{0x1, 0xE3}, {0x0, 0x08}, {0x2, 0x20},
	{0x1, 0xE3}, {0x0, 0x09}, {0x2, 0x29},
	{0x1, 0xE3}, {0x0, 0x0A}, {0x2, 0x82},
	{0x1, 0xE3}, {0x0, 0x0B}, {0x2, 0x13},
	{0x1, 0xE3}, {0x0, 0x0C}, {0x2, 0x32},
	{0x1, 0xE3}, {0x0, 0x0D}, {0x2, 0x56},
	{0x1, 0xE3}, {0x0, 0x0E}, {0x2, 0x79},
	{0x1, 0xE3}, {0x0, 0x0F}, {0x2, 0xB8},
	{0x1, 0xE3}, {0x0, 0x10}, {0x2, 0x55},
	{0x1, 0xE3}, {0x0, 0x11}, {0x2, 0x57},

	{0x1, 0xE4}, {0x0, 0x00}, {0x2, 0x0E},
	{0x1, 0xE4}, {0x0, 0x01}, {0x2, 0x16},
	{0x1, 0xE4}, {0x0, 0x02}, {0x2, 0x24},
	{0x1, 0xE4}, {0x0, 0x03}, {0x2, 0x3B},
	{0x1, 0xE4}, {0x0, 0x04}, {0x2, 0x1E},
	{0x1, 0xE4}, {0x0, 0x05}, {0x2, 0x2D},
	{0x1, 0xE4}, {0x0, 0x06}, {0x2, 0x5F},
	{0x1, 0xE4}, {0x0, 0x07}, {0x2, 0x3E},
	{0x1, 0xE4}, {0x0, 0x08}, {0x2, 0x20},
	{0x1, 0xE4}, {0x0, 0x09}, {0x2, 0x29},
	{0x1, 0xE4}, {0x0, 0x0A}, {0x2, 0x82},
	{0x1, 0xE4}, {0x0, 0x0B}, {0x2, 0x13},
	{0x1, 0xE4}, {0x0, 0x0C}, {0x2, 0x32},
	{0x1, 0xE4}, {0x0, 0x0D}, {0x2, 0x56},
	{0x1, 0xE4}, {0x0, 0x0E}, {0x2, 0x79},
	{0x1, 0xE4}, {0x0, 0x0F}, {0x2, 0xB8},
	{0x1, 0xE4}, {0x0, 0x10}, {0x2, 0x55},
	{0x1, 0xE4}, {0x0, 0x11}, {0x2, 0x57},

	{0x1, 0xE5}, {0x0, 0x00}, {0x2, 0x0E},
	{0x1, 0xE5}, {0x0, 0x01}, {0x2, 0x16},
	{0x1, 0xE5}, {0x0, 0x02}, {0x2, 0x24},
	{0x1, 0xE5}, {0x0, 0x03}, {0x2, 0x3B},
	{0x1, 0xE5}, {0x0, 0x04}, {0x2, 0x1E},
	{0x1, 0xE5}, {0x0, 0x05}, {0x2, 0x2D},
	{0x1, 0xE5}, {0x0, 0x06}, {0x2, 0x5F},
	{0x1, 0xE5}, {0x0, 0x07}, {0x2, 0x3E},
	{0x1, 0xE5}, {0x0, 0x08}, {0x2, 0x20},
	{0x1, 0xE5}, {0x0, 0x09}, {0x2, 0x29},
	{0x1, 0xE5}, {0x0, 0x0A}, {0x2, 0x82},
	{0x1, 0xE5}, {0x0, 0x0B}, {0x2, 0x13},
	{0x1, 0xE5}, {0x0, 0x0C}, {0x2, 0x32},
	{0x1, 0xE5}, {0x0, 0x0D}, {0x2, 0x56},
	{0x1, 0xE5}, {0x0, 0x0E}, {0x2, 0x79},
	{0x1, 0xE5}, {0x0, 0x0F}, {0x2, 0xB8},
	{0x1, 0xE5}, {0x0, 0x10}, {0x2, 0x55},
	{0x1, 0xE5}, {0x0, 0x11}, {0x2, 0x57},
	/* Gamma setting: done */

	/* Set RGB-888 */
	{0x1, 0x3a}, {0x0, 0x00}, {0x2, 0x70},
	{0x1, 0x44}, {0x0, 0x00}, {0x2, 0x00},
	{0x1, 0x44}, {0x0, 0x01}, {0x2, 0x00},
	{0x1, 0x35}, {0x0, 0x00}, {0x2, 0x00},
	{0x1, 0x11}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 120},
	{0x1, 0x29}, {0x0, 0x0 }, {0x2, 0x0 },
};

static struct lcm_cmd auo_blank_seq[] = {
        {0x1, 0x28}, {0x0, 0x00}, {0x2, 0x00},
        {0x1, 0x10}, {0x0, 0x00}, {0x2, 0x00},
        {LCM_MDELAY, 60},
};

static struct lcm_cmd auo_uninit_seq[] = {
        {LCM_MDELAY, 5},
        {0x1, 0x4F}, {0x0, 0x00}, {0x2, 0x01},
};

static struct lcm_cmd auo_turn_on_backlight[] = {
        {0x1, 0x53}, {0x0, 0x00}, {0x2, 0x24},
};

static struct lcm_cmd auo_enable_cabc[] = {
        {0x1, 0x55}, {0x0, 0x00}, {0x2, 0x01},
        {0x1, 0x53}, {0x0, 0x00}, {0x2, 0x24},
};

static struct lcm_cmd auo_disable_cabc[] = {
        {0x1, 0x55}, {0x0, 0x00}, {0x2, 0x00},
        {0x1, 0x53}, {0x0, 0x00}, {0x2, 0x24},
};

#ifdef CONFIG_PANEL_SELF_REFRESH
static struct lcm_cmd auo_refresh_in_seq[] = {
        {0x1, 0x3B}, {0x0, 0x00}, {0x2, 0x13},
};

static struct lcm_cmd auo_refresh_out_seq[] = {
        {0x1, 0x3B}, {0x0, 0x00}, {0x2, 0x03},
};
#endif


static int lcm_auo_write_seq(struct lcm_cmd *cmd_table, unsigned size)
{
        int i;

        for (i = 0; i < size; i++) {
		if (cmd_table[i].cmd == LCM_MDELAY) {
			hr_msleep(cmd_table[i].data);
			continue;
		}
		qspi_send_16bit(cmd_table[i].cmd, cmd_table[i].data);
	}
        return 0;
}

static int mecha_auo_panel_init(struct msm_lcdc_panel_ops *ops)
{
	LCMDBG("\n");

	mutex_lock(&panel_lock);
	mecha_auo_panel_power(1);
	lcm_auo_write_seq(auo_init_seq, ARRAY_SIZE(auo_init_seq));
	lcm_auo_write_seq(auo_gamma_normal_seq, ARRAY_SIZE(auo_gamma_normal_seq));

	mutex_unlock(&panel_lock);
	return 0;
}

static int mecha_auo_panel_uninit(struct msm_lcdc_panel_ops *ops)
{
	LCMDBG("\n");
	mutex_lock(&panel_lock);
	lcm_auo_write_seq(auo_uninit_seq, ARRAY_SIZE(auo_uninit_seq));
	mecha_auo_panel_power(0);
	mutex_unlock(&panel_lock);
	return 0;
}

static int mecha_auo_panel_unblank(struct msm_lcdc_panel_ops *ops)
{
	LCMDBG("\n");
	if (color_enhancement == 0) {
		mecha_mdp_color_enhancement(mdp_pdata.mdp_dev);
		color_enhancement = 1;
	}
	mutex_lock(&panel_lock);
	qspi_send_16bit(0x1, 0x51);
	qspi_send_16bit(0x0, 0x00);
	qspi_send_16bit(0x2, last_val);
	lcm_auo_write_seq(auo_turn_on_backlight, ARRAY_SIZE(auo_turn_on_backlight));
	mutex_unlock(&panel_lock);
	return 0;
}

static int mecha_auo_panel_blank(struct msm_lcdc_panel_ops *ops)
{
	LCMDBG("\n");
	mutex_lock(&panel_lock);
	lcm_auo_write_seq(auo_blank_seq, ARRAY_SIZE(auo_blank_seq));
	mutex_unlock(&panel_lock);
	return 0;
}

#ifdef CONFIG_PANEL_SELF_REFRESH
static int mecha_auo_panel_refresh_enable(struct msm_lcdc_panel_ops *ops)
{
	mutex_lock(&panel_lock);
        lcm_auo_write_seq(auo_refresh_in_seq, ARRAY_SIZE(auo_refresh_in_seq));
	mutex_unlock(&panel_lock);

        return 0;
}


static int mecha_auo_panel_refresh_disable(struct msm_lcdc_panel_ops *ops)
{
	mutex_lock(&panel_lock);
        lcm_auo_write_seq(auo_refresh_out_seq, ARRAY_SIZE(auo_refresh_out_seq));
	mutex_unlock(&panel_lock);

        return 0;
}
#endif

static struct msm_lcdc_panel_ops mecha_auo_panel_ops = {
	.init		= mecha_auo_panel_init,
	.uninit		= mecha_auo_panel_uninit,
	.blank		= mecha_auo_panel_blank,
	.unblank	= mecha_auo_panel_unblank,
};

#ifdef CONFIG_PANEL_SELF_REFRESH
static struct msm_lcdc_timing mecha_auo_timing = {
	.clk_rate		= 24576000,
	.hsync_pulse_width	= 2,
	.hsync_back_porch	= 10,
	.hsync_front_porch	= 10,
	.hsync_skew		= 0,
	.vsync_pulse_width	= 2,
	.vsync_back_porch	= 3,
	.vsync_front_porch	= 25,
	.vsync_act_low		= 1,
	.hsync_act_low		= 1,
	.den_act_low		= 0,
};
#else
static struct msm_lcdc_timing mecha_auo_timing = {
	.clk_rate		= 24576000,
	.hsync_pulse_width	= 4,
	.hsync_back_porch	= 4,
	.hsync_front_porch	= 4,
	.hsync_skew		= 0,
	.vsync_pulse_width	= 4,
	.vsync_back_porch	= 4,
	.vsync_front_porch	= 6,
	.vsync_act_low		= 1,
	.hsync_act_low		= 1,
	.den_act_low		= 0,
};
#endif

/*----------------------------------------------------------------------------*/

#define LCM_CMD(_cmd, ...)                                      \
{                                                               \
	.cmd = _cmd,                                            \
	.data = (u8 []){__VA_ARGS__},                           \
	.len = sizeof((u8 []){__VA_ARGS__}) / sizeof(u8)        \
}

static struct spi_msg sharp565_init_seq[] = {
        LCM_CMD(0x11),
	LCM_CMD(LCM_MDELAY, 110),
	LCM_CMD(0xB9, 0xFF, 0x83, 0x63),
	LCM_CMD(0x3A, 0x50),
	LCM_CMD(0x29),
};

static struct spi_msg sharp_init_seq[] = {
	LCM_CMD(0x11), LCM_CMD(LCM_MDELAY, 110),
	LCM_CMD(0xB9, 0xFF, 0x83, 0x63),
	LCM_CMD(0x3A, 0x70),
	LCM_CMD(0xC9, 0x0F, 0x3E, 0x02),
};

static struct spi_msg sharp_gamma_seq[] = {
	LCM_CMD(0xB1, 0x78, 0x34, 0x07, 0x01, 0x02, 0x03, 0x0F, 0x00, 0x3A,
		0x42, 0x1F, 0x29),
	LCM_CMD(0xE0, 0x00, 0xCA, 0x91, 0xD2, 0x6D, 0xBF, 0x07, 0x4D, 0xD0,
		0xD6, 0x19, 0x96, 0xD5, 0x0B, 0x13, 0x00, 0xC8, 0x8E, 0xD8,
		0x6F, 0xBF, 0x07, 0x4D, 0xCE, 0x53, 0x17, 0x94, 0xD5, 0xCB,
		0x14),
};

static struct spi_msg sharp_blank_seq[] = {
        LCM_CMD(0x28), LCM_CMD(0x10),
};

static struct spi_msg sharp_turn_on_backlight[] = {
	LCM_CMD(0x29),
        LCM_CMD(0x53, 0x2C),
};

static struct spi_msg sharp_enable_cabc[] = {
	LCM_CMD(0x55, 0x01),
	LCM_CMD(0x53, 0x2C),
};

static struct spi_msg sharp_disable_cabc[] = {
	LCM_CMD(0x55, 0x00),
	LCM_CMD(0x53, 0x2C),
};

static int lcm_sharp_write_seq(struct spi_msg *cmd_table, unsigned size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (cmd_table[i].cmd == LCM_MDELAY) {
			hr_msleep(cmd_table[i].data[0]);
			continue;
		}
		qspi_send_9bit(cmd_table + i);
	}
	return 0;
}

static int mecha_sharp_panel_init(struct msm_lcdc_panel_ops *ops)
{
        LCMDBG("\n");

        mutex_lock(&panel_lock);
        mecha_sharp_panel_power(1);
	if (panel_type == PANEL_SHARP) {
		lcm_sharp_write_seq(sharp_init_seq, ARRAY_SIZE(sharp_init_seq));
		if (panel_rev == 0)
			lcm_sharp_write_seq(sharp_gamma_seq, ARRAY_SIZE(sharp_gamma_seq));
	} else
		lcm_sharp_write_seq(sharp565_init_seq, ARRAY_SIZE(sharp565_init_seq));
        mutex_unlock(&panel_lock);
        return 0;
}

static int mecha_sharp_panel_uninit(struct msm_lcdc_panel_ops *ops)
{
        LCMDBG("\n");
        mutex_lock(&panel_lock);
        mecha_sharp_panel_power(0);
        mutex_unlock(&panel_lock);
        return 0;
}

static int mecha_sharp_panel_unblank(struct msm_lcdc_panel_ops *ops)
{
	struct spi_msg set_brightness = {
                .cmd = 0x51,
                .len = 1,
                .data = (unsigned char *)&last_val,
	};

        LCMDBG("\n");
	if (color_enhancement == 0) {
		mecha_mdp_color_enhancement(mdp_pdata.mdp_dev);
		color_enhancement = 1;
	}
        mutex_lock(&panel_lock);
	qspi_send_9bit(&set_brightness);
	lcm_sharp_write_seq(sharp_turn_on_backlight,
	ARRAY_SIZE(sharp_turn_on_backlight));
        mutex_unlock(&panel_lock);
        return 0;
}

static int mecha_sharp_panel_blank(struct msm_lcdc_panel_ops *ops)
{
	LCMDBG("\n");
	mutex_lock(&panel_lock);
	lcm_sharp_write_seq(sharp_blank_seq, ARRAY_SIZE(sharp_blank_seq));
	hr_msleep(100);
	mutex_unlock(&panel_lock);
	return 0;
}

static struct msm_lcdc_panel_ops mecha_sharp_panel_ops = {
	.init		= mecha_sharp_panel_init,
	.uninit		= mecha_sharp_panel_uninit,
	.blank		= mecha_sharp_panel_blank,
	.unblank	= mecha_sharp_panel_unblank,
};

static struct msm_lcdc_timing mecha_sharp_timing = {
        .clk_rate               = 25550000,
        .hsync_pulse_width      = 6,
        .hsync_back_porch       = 15,
        .hsync_front_porch      = 6,
        .hsync_skew             = 0,
        .vsync_pulse_width      = 3,
        .vsync_back_porch       = 3,
        .vsync_front_porch      = 3,
        .vsync_act_low          = 1,
        .hsync_act_low          = 1,
        .den_act_low            = 0,
};

/*----------------------------------------------------------------------------*/


static struct resource resources_msm_fb[] = {
	{
		.start = MSM_FB_BASE,
		.end = MSM_FB_BASE + MSM_FB_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct msm_fb_data mecha_lcdc_fb_data = {
	.xres		= 480,
	.yres		= 800,
	.width		= 56,
	.height		= 94,
	.output_format	= 0,
};

static struct msm_lcdc_platform_data mecha_lcdc_platform_data = {
	.fb_id		= 0,
	.fb_data	= &mecha_lcdc_fb_data,
	.fb_resource	= &resources_msm_fb[0],
};

static struct platform_device mecha_lcdc_device = {
	.name	= "msm_mdp_lcdc",
	.id	= -1,
	.dev	= {
		.platform_data = &mecha_lcdc_platform_data,
	},
};

/*----------------------------------------------------------------------------*/

static void mecha_brightness_set(struct led_classdev *led_cdev,
		enum led_brightness val)
{
	struct spi_msg set_brightness = {
                .cmd = 0x51,
                .len = 1,
                .data = (unsigned char *)&val,
	};
	uint8_t shrink_br;

	mutex_lock(&panel_lock);
	if (val < 30)
		shrink_br = 8;
	else if ((val >= 30) && (val <= 143))
		shrink_br = 151 * (val - 30) / 113 + 8;
	else
		shrink_br = 96 * (val - 143) / 112 + 159;

	if (panel_type == PANEL_AUO) {
		qspi_send_16bit(0x1, 0x51);
		qspi_send_16bit(0x0, 0x00);
		qspi_send_16bit(0x2, shrink_br);
	} else {
		set_brightness.data = (unsigned char *)&shrink_br;
		qspi_send_9bit(&set_brightness);
	}
	last_val = shrink_br;
	mutex_unlock(&panel_lock);
}

static int mecha_cabc_switch(int on)
{

	if (test_bit(CABC_STATE, &status) == on)
               return 1;

	if (on) {
		printk(KERN_DEBUG "turn on CABC\n");
		set_bit(CABC_STATE, &status);
		mutex_lock(&panel_lock);
		if (panel_type == PANEL_AUO)
			lcm_auo_write_seq(auo_enable_cabc, ARRAY_SIZE(auo_enable_cabc));
		else
			lcm_sharp_write_seq(sharp_enable_cabc, ARRAY_SIZE(sharp_enable_cabc));
		mutex_unlock(&panel_lock);
	} else {
		printk(KERN_DEBUG "turn off CABC\n");
		clear_bit(CABC_STATE, &status);
		mutex_lock(&panel_lock);
		if (panel_type == PANEL_AUO)
			lcm_auo_write_seq(auo_disable_cabc, ARRAY_SIZE(auo_disable_cabc));
		else
			lcm_sharp_write_seq(sharp_disable_cabc, ARRAY_SIZE(sharp_disable_cabc));
		mutex_unlock(&panel_lock);
	}
	return 1;
}

static ssize_t
auto_backlight_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t
auto_backlight_store(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count);
#define CABC_ATTR(name) __ATTR(name, 0644, auto_backlight_show, auto_backlight_store)

static struct device_attribute auto_attr = CABC_ATTR(auto);
static ssize_t
auto_backlight_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;

	i += scnprintf(buf + i, PAGE_SIZE - 1, "%d\n",
				test_bit(CABC_STATE, &status));
	return i;
}

static ssize_t
auto_backlight_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int rc;
	unsigned long res;

	rc = strict_strtoul(buf, 10, &res);
	if (rc) {
		printk(KERN_ERR "invalid parameter, %s %d\n", buf, rc);
		count = -EINVAL;
		goto err_out;
	}

	if (mecha_cabc_switch(!!res))
		count = -EIO;

err_out:
	return count;
}

static struct led_classdev mecha_backlight_led = {
	.name = "lcd-backlight",
	.brightness = LED_FULL,
	.brightness_set = mecha_brightness_set,
};

static int mecha_backlight_probe(struct platform_device *pdev)
{
	int rc;

	rc = led_classdev_register(&pdev->dev, &mecha_backlight_led);
	if (rc)
		LCMDBG("backlight: failure on register led_classdev\n");

	rc = device_create_file(mecha_backlight_led.dev, &auto_attr);
	if (rc)
		LCMDBG("backlight: failure on create attr\n");

	return 0;
}

static struct platform_device mecha_backlight_pdev = {
	.name = "mecha-backlight",
};

static struct platform_driver mecha_backlight_pdrv = {
	.probe          = mecha_backlight_probe,
	.driver         = {
		.name   = "mecha-backlight",
		.owner  = THIS_MODULE,
	},
};

static int __init mecha_backlight_init(void)
{
	return platform_driver_register(&mecha_backlight_pdrv);
}

/*----------------------------------------------------------------------------*/
int __init mecha_init_panel(void)
{
	int ret;

        vreg_lcm_1v8 = vreg_get(0, "gp13");
        if (IS_ERR(vreg_lcm_1v8))
                return PTR_ERR(vreg_lcm_1v8);
        vreg_lcm_2v8 = vreg_get(0, "wlan2");
        if (IS_ERR(vreg_lcm_2v8))
                return PTR_ERR(vreg_lcm_2v8);


	LCMDBG("panel_type=%d\n", panel_type);
	if (panel_type & 0x8) {
		panel_rev = 1;
		panel_type &= 0x7;
	}
	switch (panel_type) {
	case PANEL_AUO:
#ifdef CONFIG_PANEL_SELF_REFRESH
		mdp_pdata.overrides |= MSM_MDP_RGB_PANEL_SELE_REFRESH;
		LCMDBG("MECHA AUO Panel:RGB_PANEL_SELE_REFRESH \n");
		mecha_auo_panel_ops.refresh_enable = mecha_auo_panel_refresh_enable;
		mecha_auo_panel_ops.refresh_disable = mecha_auo_panel_refresh_disable;
		msm_device_mdp.dev.platform_data = &mdp_pdata;
#endif
		mecha_lcdc_platform_data.timing = &mecha_auo_timing;
		mecha_lcdc_platform_data.panel_ops = &mecha_auo_panel_ops;
		break;
	case PANEL_SHARP:
	case PANEL_SHARP565:
		if (panel_type == PANEL_SHARP565)
			mdp_pdata.overrides |= MSM_MDP_DMA_PACK_ALIGN_LSB;
		msm_device_mdp.dev.platform_data = &mdp_pdata;
		mecha_lcdc_platform_data.timing = &mecha_sharp_timing;
		mecha_lcdc_platform_data.panel_ops = &mecha_sharp_panel_ops;
		break;
	default:
		return -EINVAL;
	}

	ret = platform_device_register(&msm_device_mdp);
	if (ret != 0)
		return ret;

	ret = platform_device_register(&mecha_lcdc_device);
	if (ret != 0)
		return ret;

	ret = platform_device_register(&mecha_backlight_pdev);
        if (ret)
                return ret;

	return 0;
}
module_init(mecha_backlight_init);
