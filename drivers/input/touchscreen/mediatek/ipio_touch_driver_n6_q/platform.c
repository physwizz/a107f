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
#include "common.h"
#include "core/config.h"
#include "core/i2c.h"
#include "core/spi.h"
#include "core/firmware.h"
#include "core/finger_report.h"
#include "core/flash.h"
#include "core/protocol.h"
#include "platform.h"
#include "core/mp_test.h"
#include "core/gesture.h"
#ifdef CONFIG_OF
#include <linux/of_fdt.h>
#include <linux/of.h>
#endif

#define WT_ADD_TP_HARDWARE_INFO 1
#if WT_ADD_TP_HARDWARE_INFO
#include <linux/hardware_info.h>
char * ven_name[] = {"Truly","TXD"};
#endif

#define WT_ADD_TEST_PROC 1
#if WT_ADD_TEST_PROC
#define CTP_PARENT_PROC_NAME  "touchscreen"
#define CTP_OPEN_PROC_NAME        "ctp_openshort_test"
//extern int dev_mkdir(char *name, umode_t mode);
#endif

#define DTS_INT_GPIO	"touch,irq-gpio"
#define DTS_RESET_GPIO	"touch,reset-gpio"

#if (TP_PLATFORM == PT_MTK)
#define DTS_OF_NAME		"mediatek,cap_touch"
#include "tpd.h"
extern struct tpd_device *tpd;
#define MTK_RST_GPIO GTP_RST_PORT
#define MTK_INT_GPIO GTP_INT_PORT
#else
#define DTS_OF_NAME		"tchip,ilitek"
#endif /* PT_MTK */

#define DEVICE_ID	"ILITEK_TDDI"

uint32_t name_modue = -1;

bool is_first_in_ice = false;
//bug 450027 wuzhenzhen.wt 20190603
bool FIRST_IRQ_AFTER_UPDATE = false;

unsigned char *CTPM_FW = NULL;

unsigned char CTPM_FW_NONE[] = {
	#include "./TP_FW/ili/FW_TDDI_TRUNK_FB.ili"
};

unsigned char CTPM_FW_XL[] = {
	#include "./TP_FW/ili/FW_TDDI_TRUNK_FB_XL.ili"
};

unsigned char CTPM_FW_TXD[] = {
	#include "./TP_FW/ili/FW_TDDI_TRUNK_FB_TXD.ili"
};

uint32_t size_ili = 0;

struct module_name_use{
	char *module_ini_name;
	char *module_bin_name;
	uint32_t size_ili;
};

#define XL_INI_NAME "/vendor/etc/mp_xl.ini"
#define XL_BIN_NAME "/vendor/etc/ILITEK_FW_XL.bin"  //add by songbinbo.wt for ILITEK tp support firmware file of bin format 20190422

#define TXD_INI_NAME "/vendor/etc/mp_txd.ini"
#define TXD_BIN_NAME "/vendor/etc/ILITEK_FW_TXD.bin"  //add by songbinbo.wt for ILITEK tp support firmware file of bin format 20190422

/* Debug level */
uint32_t ipio_debug_level = DEBUG_FIRMWARE;
EXPORT_SYMBOL(ipio_debug_level);

struct ilitek_platform_data *ipd = NULL;

struct module_name_use module_name_is_use[] = {
	{.module_ini_name = XL_INI_NAME, .module_bin_name = XL_BIN_NAME, .size_ili = sizeof(CTPM_FW_XL)},
	{.module_ini_name = TXD_INI_NAME, .module_bin_name = TXD_BIN_NAME, .size_ili = sizeof(CTPM_FW_TXD)},
	{.module_ini_name = "/sdcard/mp.ini", .module_bin_name = "ILITEK_FW", .size_ili = sizeof(CTPM_FW_NONE)},	
};

//+add by songbinbo.wt for ILITEK tp reset sequence 20190417
bool is_lcd_resume = false;
void lcd_load_ili_fw(void)
{
	int ret;

	ipio_info("Starting to resume ...\n");
	if (!ipd->suspended) {
		ipio_info("TP already in resume statues ...\n");
		return;
	}

	mutex_lock(&ipd->touch_mutex);
	/* we hope there's no any reports during resume */
	core_fr->isEnableFR = false;
	ilitek_platform_disable_irq();

	if (core_config->isEnableGesture) {
		disable_irq_wake(ipd->isr_gpio);
	}

	if (ipd->do_otp_check)
            core_config_protect_otp_prog_mode(RST_METHODS);

        ilitek_platform_tp_hw_reset(true);
        ret = core_config_ice_mode_enable(STOP_MCU);
        if (ret < 0) {
             ipio_err("Failed to enter ICE mode, ret = %d\n", ret);
            is_lcd_resume = false;
            ipd->do_otp_check = true;
         }else{
            is_lcd_resume = true;
            ipd->do_otp_check = false;
        }

        mutex_unlock(&ipd->touch_mutex);
        schedule_work(&ipd->resume_work_queue);

	ipio_info("Resume done\n");
}
EXPORT_SYMBOL(lcd_load_ili_fw);

static void ilitek_resume_work_queue(struct work_struct *work)
{
	ipio_info("start resume work queue\n");

	mutex_lock(&ipd->touch_mutex);//ritchie add 1
	core_config_switch_fw_mode(&protocol->demo_mode);

	if (ipd->isEnablePollCheckPower)
		queue_delayed_work(ipd->check_power_status_queue,
			&ipd->check_power_status_work, ipd->work_delay);
	if (ipd->isEnablePollCheckEsd)
		queue_delayed_work(ipd->check_esd_status_queue,
			&ipd->check_esd_status_work, ipd->esd_check_time);

	core_fr->isEnableFR = true;
	ilitek_platform_enable_irq();
	ipd->suspended = false;
	ipio_info("ipd->suspended = false\n");//ritchie add 1
	mutex_unlock(&ipd->touch_mutex);//ritchie add 1

	ipio_info("end resume work queue\n");

	return;
}

//-add by songbinbo.wt for ILITEK tp reset sequence 20190417

void ilitek_platform_disable_irq(void)
{
	unsigned long nIrqFlag;

	spin_lock_irqsave(&ipd->plat_spinlock, nIrqFlag);

	if (!ipd->isEnableIRQ)
		goto out;

	if (!ipd->isr_gpio) {
		ipio_err("gpio_to_irq (%d) is incorrect\n", ipd->isr_gpio);
		goto out;
	}

	disable_irq_nosync(ipd->isr_gpio);
	ipd->isEnableIRQ = false;
	ipio_debug(DEBUG_IRQ, "Disable irq success\n");

out:
	spin_unlock_irqrestore(&ipd->plat_spinlock, nIrqFlag);
}
EXPORT_SYMBOL(ilitek_platform_disable_irq);

void ilitek_platform_enable_irq(void)
{
	unsigned long nIrqFlag;

	spin_lock_irqsave(&ipd->plat_spinlock, nIrqFlag);

	if (ipd->isEnableIRQ)
		goto out;

	if (!ipd->isr_gpio) {
		ipio_err("gpio_to_irq (%d) is incorrect\n", ipd->isr_gpio);
		goto out;
	}

	enable_irq(ipd->isr_gpio);
	ipd->isEnableIRQ = true;
	ipio_debug(DEBUG_IRQ, "Enable irq success\n");

out:
	spin_unlock_irqrestore(&ipd->plat_spinlock, nIrqFlag);
}
EXPORT_SYMBOL(ilitek_platform_enable_irq);

int ilitek_platform_tp_hw_reset(bool isEnable)
{
	int ret = 0;

	if (isEnable) {
#if (TP_PLATFORM == PT_MTK)
		tpd_gpio_output(ipd->reset_gpio, 1);
		mdelay(ipd->delay_time_high);
		tpd_gpio_output(ipd->reset_gpio, 0);
		mdelay(ipd->delay_time_low);
		tpd_gpio_output(ipd->reset_gpio, 1);
		mdelay(ipd->edge_delay);
#else
		gpio_direction_output(ipd->reset_gpio, 1);
		mdelay(ipd->delay_time_high);
		gpio_set_value(ipd->reset_gpio, 0);
		mdelay(ipd->delay_time_low);
		gpio_set_value(ipd->reset_gpio, 1);
		mdelay(ipd->edge_delay);
#endif /* PT_MTK */
	} else {
#if (TP_PLATFORM == PT_MTK)
		tpd_gpio_output(ipd->reset_gpio, 0);
#else
		gpio_set_value(ipd->reset_gpio, 0);
#endif /* PT_MTK */
	}
	return ret;
}
EXPORT_SYMBOL(ilitek_platform_tp_hw_reset);

#ifdef REGULATOR_POWER_ON
void ilitek_regulator_power_on(bool status)
{
	int ret = 0;

	ipio_info("%s\n", status ? "POWER ON" : "POWER OFF");

	if (status) {
		if (ipd->vdd) {
			ret = regulator_enable(ipd->vdd);
			if (ret < 0)
				ipio_err("regulator_enable vdd fail\n");
		}
		if (ipd->vdd_i2c) {
			ret = regulator_enable(ipd->vdd_i2c);
			if (ret < 0)
				ipio_err("regulator_enable vdd_i2c fail\n");
		}
	} else {
		if (ipd->vdd) {
			ret = regulator_disable(ipd->vdd);
			if (ret < 0)
				ipio_err("regulator_enable vdd fail\n");
		}
		if (ipd->vdd_i2c) {
			ret = regulator_disable(ipd->vdd_i2c);
			if (ret < 0)
				ipio_err("regulator_enable vdd_i2c fail\n");
		}
	}
	core_config->icemodeenable = false;
	mdelay(5);
}
EXPORT_SYMBOL(ilitek_regulator_power_on);

static void ilitek_regulator_power_reg(struct ilitek_platform_data *ipd)
{
#if (TP_PLATFORM == PT_MTK)
	const char *vdd_name = "vtouch";
#else
	const char *vdd_name = "vdd";
#endif /* PT_MTK */
	const char *vcc_i2c_name = "vcc_i2c";

#if (TP_PLATFORM == PT_MTK)
	ipd->vdd = regulator_get(tpd->tpd_dev, vdd_name);
	tpd->reg = ipd->vdd;
#else
	ipd->vdd = regulator_get(&ipd->client->dev, vdd_name);
#endif /* PT_MTK */

	if (ERR_ALLOC_MEM(ipd->vdd)) {
		ipio_err("regulator_get vdd fail\n");
		ipd->vdd = NULL;
	} else {
		if (regulator_set_voltage(ipd->vdd, VDD_VOLTAGE, VDD_VOLTAGE) < 0)
			ipio_err("Failed to set vdd %d.\n", VDD_VOLTAGE);
	}

	ipd->vdd_i2c = regulator_get(ipd->dev, vcc_i2c_name);
	if (ERR_ALLOC_MEM(ipd->vdd_i2c)) {
		ipio_err("regulator_get vdd_i2c fail.\n");
		ipd->vdd_i2c = NULL;
	} else {
		if (regulator_set_voltage(ipd->vdd_i2c, VDD_I2C_VOLTAGE, VDD_I2C_VOLTAGE) < 0)
			ipio_err("Failed to set vdd_i2c %d\n", VDD_I2C_VOLTAGE);
	}
	ilitek_regulator_power_on(true);
}
#endif /* REGULATOR_POWER_ON */

#ifdef BATTERY_CHECK
static void read_power_status(uint8_t *buf)
{
	struct file *f = NULL;
	mm_segment_t old_fs;
	ssize_t byte = 0;

	old_fs = get_fs();
	set_fs(get_ds());

	f = filp_open(POWER_STATUS_PATH, O_RDONLY, 0);
	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open %s\n", POWER_STATUS_PATH);
		return;
	}

	f->f_op->llseek(f, 0, SEEK_SET);
	byte = f->f_op->read(f, buf, 20, &f->f_pos);

	ipio_debug(DEBUG_BATTERY, "Read %d bytes\n", (int)byte);

	set_fs(old_fs);
	filp_close(f, NULL);
}

static void ilitek_platform_vpower_notify(struct work_struct *pWork)
{
	uint8_t charge_status[20] = { 0 };
	static int charge_mode = 0;

	ipio_debug(DEBUG_BATTERY, "isEnableCheckPower = %d\n", ipd->isEnablePollCheckPower);
	read_power_status(charge_status);
	ipio_debug(DEBUG_BATTERY, "Batter Status: %s\n", charge_status);

	if (strstr(charge_status, "Charging") != NULL || strstr(charge_status, "Full") != NULL
	    || strstr(charge_status, "Fully charged") != NULL) {
		if (charge_mode != 1) {
			ipio_debug(DEBUG_BATTERY, "Charging mode\n");
			core_config_plug_ctrl(false);
			charge_mode = 1;
		}
	} else {
		if (charge_mode != 2) {
			ipio_debug(DEBUG_BATTERY, "Not charging mode\n");
			core_config_plug_ctrl(true);
			charge_mode = 2;
		}
	}

	if (ipd->isEnablePollCheckPower)
		queue_delayed_work(ipd->check_power_status_queue, &ipd->check_power_status_work, ipd->work_delay);
}
#endif /* BATTERY_CHECK */

#ifdef ESD_CHECK
static void ilitek_platform_esd_recovery(struct work_struct *work)
{
	int ret = 0;

	mutex_lock(&ipd->plat_mutex);
	ret = ilitek_platform_reset_ctrl(true, RST_METHODS);
	mutex_unlock(&ipd->plat_mutex);
}

static void ilitek_platform_esd_check(struct work_struct *pWork)
{
	uint8_t tx_data = 0x82, rx_data = 0;

#if (INTERFACE == SPI_INTERFACE)
	ipio_debug(DEBUG_BATTERY, "isEnablePollCheckEsd = %d\n", ipd->isEnablePollCheckEsd);
	if (spi_write_then_read(core_spi->spi, &tx_data, 1, &rx_data, 1) < 0) {
		ipio_err("spi Write Error\n");
	}

	if (rx_data != 0xA3) {
		ipio_info("Doing ESD recovery (0x%x)\n", rx_data);
		schedule_work(&ipd->esd_recovery);
	} else {
		if (ipd->isEnablePollCheckEsd)
			queue_delayed_work(ipd->check_esd_status_queue,
				&ipd->check_esd_status_work, ipd->esd_check_time);
	}
#endif /* SPI_INTERFACE */
}
#endif /* ESD_CHECK */

#if (TP_PLATFORM == PT_MTK)
static void tpd_resume(struct device *h)
{
        ipio_wt_info("TP Resume in\n");
	//+add by songbinbo.wt for ILITEK tp reset sequence 20190417
//	if (!core_firmware->isUpgrading) {
//		core_config_ic_resume();
//	}
        ipio_wt_info("TP Resume end\n");
	//-add by songbinbo.wt for ILITEK tp reset sequence 20190417
}

static void tpd_suspend(struct device *h)
{
	ipio_info("TP Suspend in\n");

	//+add by songbinbo.wt for ILITEK tp abnormal 201900505
	mutex_lock(&ipd->touch_mutex);
	if (!core_firmware->isUpgrading) {
		core_config_ic_suspend();
	}
	mutex_unlock(&ipd->touch_mutex);
	ipio_info("TP Suspend end\n");
	//-add by songbinbo.wt for ILITEK tp abnormal 201900505
}
#elif defined CONFIG_FB
static int ilitek_platform_notifier_fb(struct notifier_block *self, unsigned long event, void *data)
{
	int *blank;
	struct fb_event *evdata = data;

	ipio_info("Notifier's event = %ld\n", event);

	/*
	 *  FB_EVENT_BLANK(0x09): A hardware display blank change occurred.
	 *  FB_EARLY_EVENT_BLANK(0x10): A hardware display blank early change occurred.
	 */
	if (evdata && evdata->data && (event == FB_EVENT_BLANK)) {
		blank = evdata->data;

#if (TP_PLATFORM == PT_SPRD)
		if (*blank == DRM_MODE_DPMS_OFF)
#else
		if (*blank == FB_BLANK_POWERDOWN)
#endif /* PT_SPRD */
		{
			ipio_info("TP Suspend\n");

			if (!core_firmware->isUpgrading) {
				core_config_ic_suspend();
			}
		}
#if (TP_PLATFORM == PT_SPRD)
		else if (*blank == DRM_MODE_DPMS_ON)
#else
		else if (*blank == FB_BLANK_UNBLANK || *blank == FB_BLANK_NORMAL)
#endif /* PT_SPRD */
		{
			ipio_info("TP Resuem\n");

			if (!core_firmware->isUpgrading) {
				core_config_ic_resume();
			}
		}
	}

	return NOTIFY_OK;
}
#else /* CONFIG_HAS_EARLYSUSPEND */
static void ilitek_platform_early_suspend(struct early_suspend *h)
{
	ipio_info("TP Suspend\n");

	/* TODO: there is doing nothing if an upgrade firmware's processing. */

	core_fr_touch_release(0, 0, 0);

	input_sync(core_fr->input_device);

	core_fr->isEnableFR = false;

	core_config_ic_suspend();
}

static void ilitek_platform_late_resume(struct early_suspend *h)
{
	ipio_info("TP Resuem\n");

	core_fr->isEnableFR = true;
	core_config_ic_resume();
}
#endif /* PT_MTK */

/**
 * reg_power_check - register a thread to inquery status at certain time.
 */
static int ilitek_platform_reg_power_check(void)
{
	int ret = 0;

#ifdef BATTERY_CHECK
	INIT_DELAYED_WORK(&ipd->check_power_status_work, ilitek_platform_vpower_notify);
	ipd->check_power_status_queue = create_workqueue("ili_power_check");
	ipd->work_delay = msecs_to_jiffies(CHECK_BATTERY_TIME);
	ipd->isEnablePollCheckPower = true;
	if (!ipd->check_power_status_queue) {
		ipio_err("Failed to create a work thread to check power status\n");
		ipd->vpower_reg_nb = false;
		ret = -1;
	} else {
		ipio_info("Created a work thread to check power status at every %u jiffies\n",
			 (unsigned)ipd->work_delay);

		if (ipd->isEnablePollCheckPower) {
			queue_delayed_work(ipd->check_power_status_queue, &ipd->check_power_status_work,
					   ipd->work_delay);
			ipd->vpower_reg_nb = true;
		}
	}
#endif /* BATTERY_CHECK */

	return ret;
}

static int ilitek_platform_reg_esd_check(void)
{
	int ret = 0;

#ifdef ESD_CHECK
	INIT_DELAYED_WORK(&ipd->check_esd_status_work, ilitek_platform_esd_check);
	ipd->check_esd_status_queue = create_workqueue("ili_esd_check");
	ipd->esd_check_time = msecs_to_jiffies(CHECK_ESD_TIME);
	ipd->isEnablePollCheckEsd = true;
	if (!ipd->check_esd_status_queue) {
		ipio_err("Failed to create a work thread to check power status\n");
		ipd->vesd_reg_nb = false;
		ret = -1;
	} else {
		ipio_info("Created a work thread to check power status at every %u jiffies\n",
			 (unsigned)ipd->esd_check_time);

		INIT_WORK(&ipd->esd_recovery, ilitek_platform_esd_recovery);

		if (ipd->isEnablePollCheckEsd) {
			queue_delayed_work(ipd->check_esd_status_queue, &ipd->check_esd_status_work,
					   ipd->esd_check_time);
			ipd->vesd_reg_nb = true;
		}
	}
#endif /* ESD_CHECK */

	return ret;
}
/**
 * Register a callback function when the event of suspend and resume occurs.
 *
 * The default used to wake up the cb function comes from notifier block mechnaism.
 * If you'd rather liek to use early suspend, CONFIG_HAS_EARLYSUSPEND in kernel config
 * must be enabled.
 */
static int ilitek_platform_reg_suspend(void)
{
	int ret = 0;

#if (TP_PLATFORM == PT_MTK)
	ipio_wt_info("It does nothing if platform is MTK\n");
#else
	ipio_info("Register suspend/resume callback function\n");
#ifdef CONFIG_FB
	ipd->notifier_fb.notifier_call = ilitek_platform_notifier_fb;
#if (TP_PLATFORM == PT_SPRD)
	ret = adf_register_client(&ipd->notifier_fb);
#else
	ret = fb_register_client(&ipd->notifier_fb);
#endif /* PT_SPRD */
#else
	ipd->early_suspend->suspend = ilitek_platform_early_suspend;
	ipd->early_suspend->esume = ilitek_platform_late_resume;
	ipd->early_suspend->level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ret = register_early_suspend(ipd->early_suspend);
#endif /* CONFIG_FB */
#endif /* PT_MTK */

	return ret;
}

static irqreturn_t ilitek_platform_irq_top_half(int irq, void *dev_id)
{
	if (irq != ipd->isr_gpio || core_firmware->isUpgrading)
		return IRQ_NONE;

	return IRQ_WAKE_THREAD;
}

static irqreturn_t ilitek_platform_irq_bottom_half(int irq, void *dev_id)
{
	//+bug 450027 wuzhenzhen.wt 20190603
	if(FIRST_IRQ_AFTER_UPDATE)
	{
		FIRST_IRQ_AFTER_UPDATE = false;
		ipio_info("ignore the first irq after download firmware\n");
		return IRQ_HANDLED;
	}
	//-bug 450027 wuzhenzhen.wt 20190603

	mutex_lock(&ipd->touch_mutex);

	ilitek_platform_disable_irq();
	core_fr_handler();
	ilitek_platform_enable_irq();

	mutex_unlock(&ipd->touch_mutex);
	return IRQ_HANDLED;
}

static int ilitek_platform_input_init(void)
{
	int ret = 0;

#if (TP_PLATFORM == PT_MTK)
	int i;

	ipd->input_device = tpd->dev;

	if (tpd_dts_data.use_tpd_button) {
		for (i = 0; i < tpd_dts_data.tpd_key_num; i++) {
			input_set_capability(ipd->input_device, EV_KEY, tpd_dts_data.tpd_key_local[i]);
		}
	}
	core_fr_input_set_param(ipd->input_device);
	return ret;
#else
	ipd->input_device = input_allocate_device();

	if (ERR_ALLOC_MEM(ipd->input_device)) {
		ipio_err("Failed to allocate touch input device\n");
		ret = -ENOMEM;
		goto fail_alloc;
	}

#if (INTERFACE == I2C_INTERFACE)
	ipd->input_device->name = ipd->client->name;
	ipd->input_device->phys = "I2C";
	ipd->input_device->dev.parent = &ipd->client->dev;
	ipd->input_device->id.bustype = BUS_I2C;
#else
	ipd->input_device->name = DEVICE_ID;
	ipd->input_device->phys = "SPI";
	ipd->input_device->dev.parent = &ipd->spi->dev;
	ipd->input_device->id.bustype = BUS_SPI;
#endif

	core_fr_input_set_param(ipd->input_device);
	/* register the input device to input sub-system */
	ret = input_register_device(ipd->input_device);
	if (ret < 0) {
		ipio_err("Failed to register touch input device, ret = %d\n", ret);
		goto out;
	}

	return ret;

fail_alloc:
	input_free_device(core_fr->input_device);
	return ret;

out:
	input_unregister_device(ipd->input_device);
	input_free_device(core_fr->input_device);
	return ret;
#endif /* PT_MTK */
}

#ifdef BOOT_FW_UPGRADE
static int kthread_handler(void *arg)
{
	int ret = 0, retry = 0;
	char *str = (char *)arg;

	retry = core_firmware->retry_times;

	if (strcmp(str, "boot_fw") == 0) {
		/* FW Upgrade event */
		core_firmware->isboot = true;

		ilitek_platform_disable_irq();

		ret = core_firmware_upgrade(UPGRADE_FLASH, HEX_FILE, OPEN_FW_METHOD);
		if (ret < 0)
			ipio_err("boot upgrade failed");

		ilitek_platform_enable_irq();

		ilitek_platform_input_init();

		core_firmware->isboot = false;
	}  else {
		ipio_err("Unknown EVENT\n");
	}

	return ret;
}
#endif

static int ilitek_platform_isr_register(void)
{
	int ret = 0;
#if (TP_PLATFORM == PT_MTK)
	struct device_node *node;
#endif /* PT_MTK */

#if (TP_PLATFORM == PT_MTK)
	node = of_find_matching_node(NULL, touch_of_match);
	if (node) {
		ipd->isr_gpio = irq_of_parse_and_map(node, 0);
	}
#else
	ipd->isr_gpio = gpio_to_irq(ipd->int_gpio);
#endif /* PT_MTK */

	ipio_info("ipd->isr_gpio = %d\n", ipd->isr_gpio);

	ret = request_threaded_irq(ipd->isr_gpio,
				   ilitek_platform_irq_top_half,
				   ilitek_platform_irq_bottom_half, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "ilitek", NULL);

	if (ret != 0) {
		ipio_err("Failed to register irq handler, irq = %d, ret = %d\n", ipd->isr_gpio, ret);
		goto out;
	}

	ipd->isEnableIRQ = true;

out:
	return ret;
}

static int ilitek_platform_gpio(void)
{
	int ret = 0;
#if (TP_PLATFORM == PT_MTK)
	ipd->int_gpio = MTK_INT_GPIO;
	ipd->reset_gpio = MTK_RST_GPIO;
#else
#ifdef CONFIG_OF
#if (INTERFACE == I2C_INTERFACE)
	struct device_node *dev_node = ipd->client->dev.of_node;
#else
	struct device_node *dev_node = ipd->spi->dev.of_node;
#endif
	uint32_t flag;

	ipd->int_gpio = of_get_named_gpio_flags(dev_node, DTS_INT_GPIO, 0, &flag);
	ipd->reset_gpio = of_get_named_gpio_flags(dev_node, DTS_RESET_GPIO, 0, &flag);
#endif /* CONFIG_OF */
#endif /* PT_MTK */

	ipio_wt_info("TP GPIO INT: %d\n", ipd->int_gpio);
	ipio_wt_info("TP GPIO RESET: %d\n", ipd->reset_gpio);

	if (!gpio_is_valid(ipd->int_gpio)) {
		ipio_err("Invalid INT gpio: %d\n", ipd->int_gpio);
		return -EBADR;
	}

	if (!gpio_is_valid(ipd->reset_gpio)) {
		ipio_err("Invalid RESET gpio: %d\n", ipd->reset_gpio);
		return -EBADR;
	}

	ret = gpio_request(ipd->int_gpio, "ILITEK_TP_IRQ");
	if (ret < 0) {
		ipio_err("Request IRQ GPIO failed, ret = %d\n", ret);
		gpio_free(ipd->int_gpio);
		ret = gpio_request(ipd->int_gpio, "ILITEK_TP_IRQ");
		if (ret < 0) {
			ipio_err("Retrying request INT GPIO still failed , ret = %d\n", ret);
			goto out;
		}
	}

	ret = gpio_request(ipd->reset_gpio, "ILITEK_TP_RESET");
	if (ret < 0) {
		ipio_err("Request RESET GPIO failed, ret = %d\n", ret);
		gpio_free(ipd->reset_gpio);
		ret = gpio_request(ipd->reset_gpio, "ILITEK_TP_RESET");
		if (ret < 0) {
			ipio_err("Retrying request RESET GPIO still failed , ret = %d\n", ret);
			goto out;
		}
	}

	gpio_direction_input(ipd->int_gpio);

out:
	return ret;
}

int ilitek_platform_read_tp_info(void)
{
	int ret = -1;

	if (core_config_get_protocol_ver() < 0) {
		ipio_err("Failed to get protocol version\n");
		goto out;
	}

	if (core_config_get_fw_ver() < 0) {
		ipio_err("Failed to get firmware version\n");
		goto out;
	}

	if (core_config_get_core_ver() < 0) {
		ipio_err("Failed to get core version\n");
		goto out;
	}

	if (core_config_get_tp_info() < 0) {
		ipio_err("Failed to get TP information\n");
		goto out;
	}

	if (core_config_get_panel_info() < 0) {
		ipio_err("Failed to get panel information\n");
		goto out;
	}
#ifndef HOST_DOWNLOAD
	if (core_config_get_key_info() < 0) {
		ipio_err("Failed to get key information\n");
		goto out;
	}
#endif

	ret = 0;

out:
	return ret;
}
EXPORT_SYMBOL(ilitek_platform_read_tp_info);

/**
 * The function is to initialise all necessary structurs in those core APIs,
 * they must be called before the i2c dev probes up successfully.
 */
static int ilitek_platform_core_init(void)
{
	ipio_wt_info("Initialise core's components\n");

	if (core_config_init() < 0 || core_protocol_init() < 0 ||
		core_firmware_init() < 0 || core_fr_init() < 0 ||
		core_gesture_init () < 0) {
		ipio_err("Failed to initialise core components\n");
		return -EINVAL;
	}

#if (INTERFACE == I2C_INTERFACE)
	if (core_i2c_init(ipd->client) < 0) {
#else
	if (core_spi_init(ipd->spi) < 0) {
#endif

		ipio_err("Failed to initialise interface\n");
		return -EINVAL;
	}
	return 0;
}

int ilitek_platform_reset_ctrl(bool rst, int mode)
{
	int ret = 0;

	atomic_set(&ipd->do_reset, true);
	ilitek_platform_disable_irq();

	if (ipd->do_otp_check)
		core_config_protect_otp_prog_mode(mode);
	switch (mode) {
		case SW_RST:
			ipio_info("SW RESET\n");
			ret = core_config_ic_reset();
			break;
		case HW_RST:
			ipio_info("HW RESET ONLY\n");
			ilitek_platform_tp_hw_reset(rst);
			//mdelay(100);
			break;
		case HW_RST_HOST_DOWNLOAD:
			ipio_info("HW_RST_HOST_DOWNLOAD\n");
	//		ilitek_platform_tp_hw_reset(rst);
			ret = core_firmware_upgrade(UPGRADE_IRAM, HEX_FILE, OPEN_FW_METHOD);
			if (ret < 0)
				ipio_err("host download with retry failed\n");
			break;
		case SW_RST_HOST_DOWNLOAD:
			ipio_info("SW_RST_HOST_DOWNLOAD\n");
			ret = core_config_ic_reset();
			ret = core_firmware_upgrade(UPGRADE_IRAM, HEX_FILE, OPEN_FW_METHOD);
			if (ret < 0)
				ipio_err("host download with retry failed\n");
			break;
		default:
			ipio_err("Unknown RST mode (%d)\n", mode);
			ret = -1;
			break;
	}

	core_config->icemodeenable = false;

	atomic_set(&ipd->do_reset, false);
	ilitek_platform_enable_irq();
	return ret;
}

#if (INTERFACE == I2C_INTERFACE)
static int ilitek_platform_remove(struct i2c_client *client)
#else
static int ilitek_platform_remove(struct spi_device *spi)
#endif
{
	ipio_info("Remove platform components\n");

	if (ipd->isEnableIRQ) {
		disable_irq_nosync(ipd->isr_gpio);
	}

	if (ipd->isr_gpio != 0 && ipd->int_gpio != 0 && ipd->reset_gpio != 0) {
		free_irq(ipd->isr_gpio, (void *)ipd->i2c_id);
		gpio_free(ipd->int_gpio);
		gpio_free(ipd->reset_gpio);
	}
#ifdef CONFIG_FB
	fb_unregister_client(&ipd->notifier_fb);
#else
	unregister_early_suspend(&ipd->early_suspend);
#endif /* CONFIG_FB */

	if (ipd->input_device != NULL) {
		input_unregister_device(ipd->input_device);
		input_free_device(ipd->input_device);
	}

	if (ipd->vpower_reg_nb) {
		cancel_delayed_work_sync(&ipd->check_power_status_work);
		destroy_workqueue(ipd->check_power_status_queue);
	}

	if (ipd->vesd_reg_nb) {
		cancel_delayed_work_sync(&ipd->check_esd_status_work);
		destroy_workqueue(ipd->check_esd_status_queue);
	}

	ilitek_proc_remove();
	return 0;
}

#if WT_ADD_TEST_PROC
static struct proc_dir_entry *ctp_device_proc = NULL;

int ili_open_short_test_result = 0;
static ssize_t ctp_open_proc_read(struct file *file, char __user *buf,size_t count, loff_t *ppos)
{

    int len = count;
	int ret;
	bool lcm_on = true;

	if (*ppos != 0)
		return 0;

    *ppos+=count;

	if (core_firmware->isUpgrading) {
		ipio_err("FW upgrading, please wait to complete\n");
		return 0;
	}

	ili_open_short_test_result = 0;
	/* Running MP Test */
	ret = core_mp_start_test(lcm_on);
	if(ret < 0){
		ipio_err("Mp test fail.\n");
	}

	/* copy MP result to user */
    if (count > 10)
        len = 10;

    if (ili_open_short_test_result == 1){
        if (copy_to_user(buf, "result=1\n", len)) {
            printk("%s copy_to_user fail\n",__func__);
            core_mp_test_free();
            return -1;
        }
    }else{
        if (copy_to_user(buf, "result=0\n", len)) {
            printk("%s copy_to_user fail\n",__func__);
            core_mp_test_free();
            return -1;
        }
    }

    core_mp_test_free();
    return len;
}

static ssize_t ctp_open_proc_write(struct file *filp, const char __user *userbuf,size_t count, loff_t *ppos)
{
    return -1;
}

static const struct file_operations ctp_open_procs_fops =
{
	.write = ctp_open_proc_write,
	.read = ctp_open_proc_read,
	.owner = THIS_MODULE,
};

void create_ctp_proc(void)
{
    //----------------------------------------
    //create read/write interface for tp information
    //the path is :proc/touchscreen
    //child node is :version
    //----------------------------------------
    struct proc_dir_entry *ctp_open_proc = NULL;

    printk(" create_ctp_proc \n");
    if( ctp_device_proc == NULL) {
	    ctp_device_proc = proc_mkdir(CTP_PARENT_PROC_NAME, NULL);
	    if(ctp_device_proc == NULL) {
	        printk("create parent_proc fail\n");
	        return;
	    }
	}
    ctp_open_proc = proc_create(CTP_OPEN_PROC_NAME, 0777, ctp_device_proc, &ctp_open_procs_fops);
    if (ctp_open_proc == NULL) {
        printk("create open_proc fail\n");
    }
}
#endif

/**
 * The probe func would be called after an i2c device was detected by kernel.
 *
 * It will still return zero even if it couldn't get a touch ic info.
 * The reason for why we allow it passing the process is because users/developers
 * might want to have access to ICE mode to upgrade a firwmare forcelly.
 */
#if (INTERFACE == I2C_INTERFACE)
static int ilitek_platform_probe(struct i2c_client *client, const struct i2c_device_id *id)
#else
static int ilitek_platform_probe(struct spi_device *spi)
#endif
{

#if WT_ADD_TP_HARDWARE_INFO
    char firmware_ver[HARDWARE_MAX_ITEM_LONGTH];
#endif

#if (INTERFACE == I2C_INTERFACE)
	if (client == NULL) {
		ipio_err("i2c client is NULL\n");
		return -ENODEV;
	}

	/* Set i2c slave addr if it's not configured */
	ipio_info("I2C Slave address = 0x%x\n", client->addr);
	if (client->addr != ILITEK_I2C_ADDR) {
		client->addr = ILITEK_I2C_ADDR;
		ipio_err("I2C Slave addr doesn't be set up, use default : 0x%x\n", client->addr);
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ipio_err("I2C not supported\n");
		return -ENODEV;
	}
#else
	if (spi == NULL) {
		ipio_err("spi device is NULL\n");
		return -ENODEV;
	}
#endif


	/* initialise the struct of touch ic memebers. */
#if (INTERFACE == I2C_INTERFACE)
	ipd = devm_kzalloc(&client->dev, sizeof(struct ilitek_platform_data), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ipd)) {
		ipio_err("Failed to allocate ipd memory, %ld\n", PTR_ERR(ipd));
		return -ENOMEM;
	}

	ipd->client = client;
	ipd->i2c_id = id;
	ipd->dev = &client->dev;
#else
	ipd = devm_kzalloc(&spi->dev, sizeof(struct ilitek_platform_data), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ipd)) {
		ipio_err("Failed to allocate ipd memory, %ld\n", PTR_ERR(ipd));
		return -ENOMEM;
	}
	/***************************add for compass***********************************************/
	
	//name_modue = XL_MODULE_NAME;
	
	if(name_modue == XL_MODULE_NAME){
		ipd->ini_name = module_name_is_use[XL_MODULE_NAME].module_ini_name;
		ipd->bin_name = module_name_is_use[XL_MODULE_NAME].module_bin_name;
	
		size_ili = module_name_is_use[XL_MODULE_NAME].size_ili;
		
		CTPM_FW = vmalloc(size_ili);
		if (ERR_ALLOC_MEM(CTPM_FW)) {
			ipio_err("Failed to allocate CTPM_FW memory, %ld\n", PTR_ERR(CTPM_FW));
			return -ENOMEM;
		}
		CTPM_FW = CTPM_FW_XL;
		ipio_info("TP is xinli\n");
	
		
	}else if(name_modue == TXD_MODULE_NAME){
		ipd->ini_name = module_name_is_use[TXD_MODULE_NAME].module_ini_name;
		ipd->bin_name = module_name_is_use[TXD_MODULE_NAME].module_bin_name;
	
		size_ili = module_name_is_use[TXD_MODULE_NAME].size_ili;
		CTPM_FW = vmalloc(size_ili);
		if (ERR_ALLOC_MEM(CTPM_FW)) {
			ipio_err("Failed to allocate CTPM_FW memory, %ld\n", PTR_ERR(CTPM_FW));
			return -ENOMEM;
		}
		CTPM_FW = CTPM_FW_TXD;
		ipio_info("TP is tongxingda\n");
	
	
	}else{
		ipd->ini_name = module_name_is_use[NONE_NAME].module_ini_name;
		ipd->bin_name = module_name_is_use[NONE_NAME].module_bin_name;
	
		size_ili = module_name_is_use[NONE_NAME].size_ili;
		CTPM_FW = vmalloc(size_ili);
		if (ERR_ALLOC_MEM(CTPM_FW)) {
			ipio_err("Failed to allocate CTPM_FW memory, %ld\n", PTR_ERR(CTPM_FW));
			return -ENOMEM;
		}
		CTPM_FW = CTPM_FW_NONE;
		ipio_info("TP is normal\n");
	
	}
	/***************************add for compass***********************************************/
	ipd->spi = spi;
	ipd->dev = &spi->dev;
#endif
	ipd->isEnableIRQ = false;
	ipd->isEnablePollCheckPower = false;
	ipd->isEnablePollCheckEsd = false;
	ipd->vpower_reg_nb = false;
	ipd->vesd_reg_nb = false;

	ipio_info("Driver Version : %s\n", DRIVER_VERSION);
	ipio_wt_info("Driver on platform :  %x\n", TP_PLATFORM);
	ipio_wt_info("Driver interface :  %s\n", (INTERFACE == I2C_INTERFACE) ? "I2C" : "SPI");

	ipd->delay_time_high = 1;
	ipd->delay_time_low = 5;
#if (INTERFACE == I2C_INTERFACE)
	ipd->edge_delay = 100;
#else
	ipd->edge_delay = 5;
#endif

	mutex_init(&ipd->plat_mutex);
	mutex_init(&ipd->touch_mutex);
	spin_lock_init(&ipd->plat_spinlock);
	//+add by songbinbo.wt for ILITEK tp reset sequence 20190417
	INIT_WORK(&ipd->resume_work_queue, ilitek_resume_work_queue);
	//-add by songbinbo.wt for ILITEK tp reset sequence 20190417

	/* Init members for debug */
	mutex_init(&ipd->ilitek_debug_mutex);
	mutex_init(&ipd->ilitek_debug_read_mutex);
	init_waitqueue_head(&(ipd->inq));
	ipd->debug_data_frame = 0;
	ipd->debug_node_open = false;
	ipd->raw_count = 1;
	ipd->delta_count = 10;
	ipd->bg_count = 0;
	ipd->debug_data_start_flag = false;

#ifdef REGULATOR_POWER_ON
	ilitek_regulator_power_reg(ipd);
#endif

	/* If kernel failes to allocate memory for the core components, driver will be unloaded. */
	if (ilitek_platform_core_init() < 0) {
		ipio_err("Failed to allocate cores' mem\n");
		return -ENOMEM;
	}

	if (ilitek_platform_gpio() < 0)
		ipio_err("Failed to request gpios\n ");

	/* Pull TP RST low to high after request GPIO succeed for normal work. */
	if (RST_METHODS == HW_RST_HOST_DOWNLOAD || RST_METHODS == HW_RST) {
		ipd->do_otp_check = false;
		ilitek_platform_reset_ctrl(true, HW_RST);
		ipd->do_otp_check = true;
	} else {
		ipd->do_otp_check = true;
		ilitek_platform_reset_ctrl(true, SW_RST);
	}

	is_first_in_ice = true;
	if (core_config_get_chip_id() < 0)
	{
		ipio_err("Failed to get chip id\n");
		return -ENODEV;
	}
	if(core_config->chip_id != 0x9881 )
	{
		ipio_err("Chip id Mismatch\n");
		return -ENODEV;
	}

#ifndef HOST_DOWNLOAD
	core_config_read_flash_info();
#else
	ilitek_platform_reset_ctrl(true, RST_METHODS);
#endif
	is_first_in_ice = false;

	if (ilitek_platform_read_tp_info() < 0)
		ipio_err("Failed to read TP info\n");

	if (ilitek_platform_isr_register() < 0)
		ipio_err("Failed to register ISR\n");

#ifndef BOOT_FW_UPGRADE
	if (ilitek_platform_input_init() < 0)
		ipio_err("Failed to init input device in kernel\n");
#endif

	if (ilitek_platform_reg_suspend() < 0)
		ipio_err("Failed to register suspend/resume function\n");

	if (ilitek_platform_reg_power_check() < 0)
		ipio_err("Failed to register power check function\n");

	if (ilitek_platform_reg_esd_check() < 0)
		ipio_err("Failed to register esd check function\n");

	if (ilitek_proc_init() < 0)
		ipio_err("Failed to create ilitek device nodes\n");

#if WT_ADD_TEST_PROC
    create_ctp_proc();
#endif

#if (TP_PLATFORM == PT_MTK)
	tpd_load_status = 1;
#endif /* PT_MTK */

#ifdef BOOT_FW_UPGRADE
	ipd->update_thread = kthread_run(kthread_handler, "boot_fw", "ili_fw_boot");
	if (ipd->update_thread == (struct task_struct *)ERR_PTR) {
		ipd->update_thread = NULL;
		ipio_err("Failed to create fw upgrade thread\n");
	}
#endif

#if WT_ADD_TP_HARDWARE_INFO
	if (ilitek_platform_read_tp_info() < 0)
		ipio_err("Failed to read TP info\n");

    if (protocol->mid >= 0x3) {
        snprintf(firmware_ver,HARDWARE_MAX_ITEM_LONGTH,"ILI9881H,VND:%s,FW:%d.%d.%d.%d",ven_name[name_modue],\
                 core_config->firmware_ver[1], core_config->firmware_ver[2], core_config->firmware_ver[3], core_config->firmware_ver[4]);
    } else {
        snprintf(firmware_ver,HARDWARE_MAX_ITEM_LONGTH,"ILI9881H,VND:%s,FW:%d.%d.%d",ven_name[name_modue],\
                 core_config->firmware_ver[1], core_config->firmware_ver[2], core_config->firmware_ver[3]);
    }
    hardwareinfo_set_prop(HARDWARE_TP,firmware_ver);
#endif

	return 0;
}

#ifdef CONFIG_OF
struct tag_videolfb {
	u64 fb_base;
	u32 islcmfound;
	u32 fps;
	u32 vram;
	char lcmname[1];	/* this is the minimum size */
};
static char mtkfb_lcm_name[256] = { 0 };

static int __parse_tag_videolfb(struct device_node *node)
{
	struct tag_videolfb *videolfb_tag = NULL;
	unsigned long size = 0;

	videolfb_tag =
		(struct tag_videolfb *)of_get_property(node,
			"atag,videolfb", (int *)&size);
	if (videolfb_tag) {
		memset((void *)mtkfb_lcm_name, 0, sizeof(mtkfb_lcm_name));
		strcpy((char *)mtkfb_lcm_name, videolfb_tag->lcmname);
		mtkfb_lcm_name[strlen(videolfb_tag->lcmname)] = '\0';
	}

	if(0==strcmp(mtkfb_lcm_name,"ili9881h_hd_plus_vdo_truly")){
		name_modue = XL_MODULE_NAME;    //ili9881h_hd_plus_vdo_truly
		ipio_info("modue:truly\n");
	}else if(0==strcmp(mtkfb_lcm_name,"ili9881h_hd_plus_vdo_txd")){
	    name_modue = TXD_MODULE_NAME;   //ili9881h_hd_plus_vdo_txd
		ipio_info("modue:txd\n");
	}if(0==strcmp(mtkfb_lcm_name,"ili9881h_hd_plus_vdo_truly_m")){
		name_modue = XL_MODULE_NAME;    //ili9881h_hd_plus_vdo_truly
		ipio_info("modue:truly\n");
	}
	ipio_info("name_modue=%d\n",name_modue);
	
	return 0;
}

static int _parse_tag_videolfb(void)
{
	int ret;
	struct device_node *chosen_node;
	
	chosen_node = of_find_node_by_path("/chosen");
	if (!chosen_node)
		chosen_node = of_find_node_by_path("/chosen@0");

	if (chosen_node)
		ret = __parse_tag_videolfb(chosen_node);

	return 0;
}	
#endif

static const struct i2c_device_id tp_device_id[] = {
	{DEVICE_ID, 0},
	{},			/* should not omitted */
};

MODULE_DEVICE_TABLE(i2c, tp_device_id);

/*
 * The name in the table must match the definiation
 * in a dts file.
 *
 */
static struct of_device_id tp_match_table[] = {
	{.compatible = DTS_OF_NAME},
	{},
};

#if (TP_PLATFORM == PT_MTK && INTERFACE == I2C_INTERFACE)
static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	ipio_info("TPD detect i2c device\n");
	strcpy(info->type, TPD_DEVICE);
	return 0;
}
#endif /* PT_MTK */

#if (INTERFACE == I2C_INTERFACE)
static struct i2c_driver tp_i2c_driver = {
	.driver = {
		   .name = DEVICE_ID,
		   .owner = THIS_MODULE,
		   .of_match_table = tp_match_table,
		   },
	.probe = ilitek_platform_probe,
	.remove = ilitek_platform_remove,
	.id_table = tp_device_id,
#if (TP_PLATFORM == PT_MTK)
	.detect = tpd_detect,
#endif /* PT_MTK */
};
#else
static struct spi_driver tp_spi_driver = {
	.driver = {
		.name	= DEVICE_ID,
		.owner = THIS_MODULE,
		.of_match_table = tp_match_table,
	},
	.probe = ilitek_platform_probe,
	.remove = ilitek_platform_remove,
};
#endif

#if (TP_PLATFORM == PT_MTK)
static int tpd_local_init(void)
{
	ipio_info("TPD init device driver\n");
	
#ifdef CONFIG_OF
		_parse_tag_videolfb();
#endif

#if (INTERFACE == I2C_INTERFACE)
	if (i2c_add_driver(&tp_i2c_driver) != 0) {
		ipio_err("Unable to add i2c driver\n");
		return -1;
	}
#else
	if (spi_register_driver(&tp_spi_driver) < 0) {
		ipio_err("Failed to add ilitek driver\n");
		spi_unregister_driver(&tp_spi_driver);
		return -ENODEV;
	}
#endif
	if (tpd_load_status == 0) {
		ipio_err("Add error touch panel driver\n");
#if (INTERFACE == I2C_INTERFACE)
		i2c_del_driver(&tp_i2c_driver);
#else
		spi_unregister_driver(&tp_spi_driver);
#endif
		return -1;
	}

	if (tpd_dts_data.use_tpd_button) {
		tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
				   tpd_dts_data.tpd_key_dim_local);
	}

	tpd_type_cap = 1;

	return 0;
}

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = DEVICE_ID,
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
};
#endif /* PT_MTK */

static int __init ilitek_platform_init(void)
{
#if (TP_PLATFORM == PT_MTK)
	int ret = 0;
	ipio_info("TDDI TP driver add i2c interface for MTK\n");
	tpd_get_dts_info();
	ret = tpd_driver_add(&tpd_device_driver);
	if (ret < 0) {
		ipio_err("TPD add TP driver failed\n");
		tpd_driver_remove(&tpd_device_driver);
		return -ENODEV;
	}
#else
#if (INTERFACE == I2C_INTERFACE)
	ipio_info("TDDI TP driver add i2c interface\n");
	return i2c_add_driver(&tp_i2c_driver);
#else
	int ret = 0;
	ipio_info("TDDI TP driver add spi interface\n");
	ret = spi_register_driver(&tp_spi_driver);
	if (ret < 0) {
		ipio_err("Failed to add ilitek driver\n");
		spi_unregister_driver(&tp_spi_driver);
		return -ENODEV;
	}
#endif
#endif /* PT_MTK */
	return 0;
}

static void __exit ilitek_platform_exit(void)
{
	ipio_info("I2C driver has been removed\n");

#if (TP_PLATFORM == PT_MTK)
	tpd_driver_remove(&tpd_device_driver);
#else
#if (INTERFACE == I2C_INTERFACE)
	i2c_del_driver(&tp_i2c_driver);
#else
	spi_unregister_driver(&tp_spi_driver);
#endif
#endif /* PT_MTK */
}

module_init(ilitek_platform_init);
module_exit(ilitek_platform_exit);
MODULE_AUTHOR("ILITEK");
MODULE_LICENSE("GPL");
