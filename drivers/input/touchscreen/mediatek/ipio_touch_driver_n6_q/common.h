/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __COMMON_H
#define __COMMON_H

//#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <net/sock.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/time.h>

#include <linux/namei.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/security.h>
#include <linux/mount.h>

#ifdef CONFIG_OF
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#else
#include <linux/earlysuspend.h>
#endif

/*
 * Relative Driver with Touch IC
 */
/* An Touch IC currently supported by driver */
#define CHIP_TYPE_ILI9881	0x9881
#define CHIP_TYPE_ILI7807	0x7807
#define TP_TOUCH_IC		CHIP_TYPE_ILI9881

/* A platform currently supported by driver */
#define PT_QCOM	1
#define PT_MTK	2
#define PT_SPRD	3
#define TP_PLATFORM PT_MTK

/* A interface currently supported by driver */
#define I2C_INTERFACE 1
#define SPI_INTERFACE 2
#define INTERFACE SPI_INTERFACE

/* Reset methods */
#define SW_RST	1
#define HW_RST	2
#define HW_RST_HOST_DOWNLOAD	3
#define SW_RST_HOST_DOWNLOAD	4
#define RST_METHODS	HW_RST_HOST_DOWNLOAD

/* Choise open hex file function*/
#define REQUEST_FIRMWARE	0
#define FILP_OPEN 			1
#define OPEN_FW_METHOD FILP_OPEN

/* Driver version */
#define DRIVER_VERSION	"1.1.5.2" //ritchie add

/* Protocol version */
#define PROTOCOL_MAJOR		0x5
#define PROTOCOL_MID		0x6
#define PROTOCOL_MINOR		0x0

#define IPIO_DEBUG_EN    0    // add by songbinbo.wt for cancel  unnecessary log  20190625

/*  Debug messages */
#ifdef BIT
#undef BIT
#endif
#define BIT(x)	(1 << (x))

enum {
	DEBUG_NONE = 0,
	DEBUG_IRQ = BIT(0),
	DEBUG_FINGER_REPORT = BIT(1),
	DEBUG_FIRMWARE = BIT(2),
	DEBUG_CONFIG = BIT(3),
	DEBUG_I2C = BIT(4),
	DEBUG_BATTERY = BIT(5),
	DEBUG_MP_TEST = BIT(6),
	DEBUG_IOCTL = BIT(7),
	DEBUG_NETLINK = BIT(8),
	DEBUG_PARSER = BIT(9),
	DEBUG_GESTURE = BIT(10),
	DEBUG_SPI = BIT(11),
	DEBUG_ALL = ~0,
};

enum module_name{
	XL_MODULE_NAME = 0,
	TXD_MODULE_NAME = 1,
	NONE_NAME = 2,
};


#define ipio_info(fmt, arg...)	\
	pr_info("ILITEK: info (%s, %d): " fmt, __func__, __LINE__, ##arg);

#define ipio_err(fmt, arg...)	\
	pr_err("ILITEK: err (%s, %d): " fmt, __func__, __LINE__, ##arg);

#define ipio_debug(level, fmt, arg...)									\
	do {																\
		if (level & ipio_debug_level)									\
		pr_info("ILITEK: dbg (%s, %d): " fmt, __func__, __LINE__, ##arg);	\
	} while (0)


//+ add by songbinbo.wt for cancel  unnecessary log  20190625
#ifdef IPIO_DEBUG_EN
#define ipio_wt_info(fmt, arg...)	\
	pr_info("ILITEK: info (%s, %d): " fmt, __func__, __LINE__, ##arg);
#else
#define ipio_wt_info(fmt, arg...)

#endif
//- add by songbinbo.wt for cancel  unnecessary log  20190625


/* MCU status */
enum {
	NO_STOP_MCU = 0,
	STOP_MCU
};

/* Distributed to all core functions */
extern uint32_t ipio_debug_level;
extern uint32_t ipio_chip_list[2];

/* Macros */
#define CHECK_EQUAL(X, Y) 	((X == Y) ? 0 : -1)
#define ERR_ALLOC_MEM(X)	((IS_ERR(X) || X == NULL) ? 1 : 0)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define USEC	1
#define MSEC	(USEC * 1000)
#define K (1024)
#define M (K * K)

/* The definition for firmware upgrade */
#define MAX_HEX_FILE_SIZE			(160*1024)
#define MAX_FLASH_FIRMWARE_SIZE		(256*1024)
#define MAX_IRAM_FIRMWARE_SIZE		(60*1024)
#define ILI_FILE_HEADER				64
#define MAX_AP_FIRMWARE_SIZE		(64*1024)
#define MAX_DLM_FIRMWARE_SIZE		(8*1024)
#define MAX_MP_FIRMWARE_SIZE		(64*1024)
#define MAX_GESTURE_FIRMWARE_SIZE	(8*1024)
#define MAX_TUNING_FIRMWARE_SIZE	(4*1024)
#define MAX_DDI_FIRMWARE_SIZE		(4*1024)

#define DLM_START_ADDRESS           0x20610
#define DLM_HEX_ADDRESS             0x10000
#define MP_HEX_ADDRESS              0x13000
#define SPI_UPGRADE_LEN				2048
#define SPI_READ_LEN				2048

#define FW_BLOCK_INFO_NUM			7
#define UPDATE_RETRY_COUNT			3

#define AP_BLOCK_NUM					1
#define DATA_BLOCK_NUM					2
#define TUNING_BLOCK_NUM				3
#define GESTURE_BLOCK_NUM				4
#define MP_BLOCK_NUM					5
#define DDI_BLOCK_NUM					6

#define ILITEK_I2C_ADDR			0x41

#define PID_ADDR		0x4009C
#define OTP_ID_ADDR		0x400A0
#define ANA_ID_ADDR		0x400A4
#define WDT_ADDR		0x5100C
#define ICE_MODE_ADDR		0x181062
#define CHIP_RESET_ADDR 	0x40050

/*
 * ILI9881 Series
 */
enum ili9881_types {
	TYPE_F = 0x0F,
	TYPE_H = 0x11
};
#define ILI9881_PC_COUNTER_ADDR 0x44008

/*
 * ILI7807 Series
 */
enum ili7807_types {
	TYPE_G = 0x10,
};
#define ILI7807_PC_COUNTER_ADDR 0x44008

/*
 * Other settings
 */
/*
#define CSV_LCM_ON_PATH     "/sdcard"
#define CSV_LCM_OFF_PATH	"/sdcard"
*/

#define CSV_LCM_ON_PATH     "/data/vendor/fac_sources/"
#define CSV_LCM_OFF_PATH	"/data/vendor/fac_sources/"

#define INI_NAME_PATH		"/sdcard/mp.ini"
#define UPDATE_FW_PATH		"/sdcard/ILITEK_FW"
#define POWER_STATUS_PATH 	"/sys/class/power_supply/battery/status"
#define DUMP_FLASH_PATH		"/sdcard/flash_dump"
#define CHECK_BATTERY_TIME  2000
#define CHECK_ESD_TIME		4000
#define VDD_VOLTAGE			1800000
#define VDD_I2C_VOLTAGE		1800000

 /* define the width and heigth of a screen. */
#define TOUCH_SCREEN_X_MIN 0
#define TOUCH_SCREEN_Y_MIN 0
#define TOUCH_SCREEN_X_MAX 720
#define TOUCH_SCREEN_Y_MAX 1520

/* define the range on panel */
#define TPD_HEIGHT 2048
#define TPD_WIDTH 2048

/* How many numbers of touch are supported by IC. */
#define MAX_TOUCH_NUM	10

/* It's only for spi interface used to download data to iram */
#if (INTERFACE == SPI_INTERFACE)
#define HOST_DOWNLOAD
#endif

/* Set spi clk up to 10Mhz (it must be enabled if chip is ILI7807G_AA) */
#if (TP_TOUCH_IC == CHIP_TYPE_ILI7807 && INTERFACE == SPI_INTERFACE)
#define ENABLE_SPI_SPEED_UP
#endif

/* Linux multiple touch protocol, either B type or A type. */
#define MT_B_TYPE

/* Enable the support of regulator power. */
//#define REGULATOR_POWER_ON

/* Enable DMA with I2C. */
//#define I2C_DMA

/* Split the length written to or read from IC via I2C. */
//#define I2C_SEGMENT

/* Be able to upgrade fw at boot stage */
//#define BOOT_FW_UPGRADE
#define BOOT_FW_HEX_NAME "ILITEK_FW"
#define BOOT_UPDATE_FW_DELAY_TIME 10000

/* Enable gesture function */
//#define GESTURE_ENABLE

/* Check battery's status in order to avoid some effects from charge. */
//#define BATTERY_CHECK

/* Check whether the IC is damaged by ESD */
//#define ESD_CHECK

static inline void ipio_kfree(void **mem) {
	if (*mem != NULL) {
		kfree(*mem);
		*mem = NULL;
	}
}

static inline void ipio_vfree(void **mem) {
	if (*mem != NULL) {
		vfree(*mem);
		*mem = NULL;
	}
}

static inline void *ipio_memcpy(void *dest, const void *src, size_t n, size_t dest_size) {
    if (n > dest_size)
         n = dest_size;

    return memcpy(dest, src, n);
}

extern int katoi(char *string);
extern int str2hex(char *str);
#endif /* __COMMON_H */
