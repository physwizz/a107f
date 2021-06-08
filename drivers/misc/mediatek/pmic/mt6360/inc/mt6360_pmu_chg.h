/*
 *  drivers/misc/mediatek/pmic/mt6360/inc/mt6360_pmu_chg.h
 *
 *  Copyright (C) 2018 Mediatek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __MT6360_PMU_CHG_H
#define __MT6360_PMU_CHG_H

/* Define this macro if DCD timeout is supported */
#define CONFIG_MT6360_DCDTOUT_SUPPORT

struct mt6360_chg_platform_data {
	u32 ichg;
	u32 aicr;
	u32 mivr;
	u32 cv;
	u32 ieoc;
	u32 safety_timer;
	u32 ircmp_resistor;
	u32 ircmp_vclamp;
	u32 en_te;
	u32 en_wdt;
	u32 aicc_once;
	u32 post_aicc;
	const char *chg_name;
};

/* MT6360_PMU_CHG_CTRL1 : 0x11 */
#define MT6360_MASK_FORCE_SLEEP	BIT(3)
#define MT6360_SHFT_FORCE_SLEEP	(3)
#define MT6360_MASK_OPA_MODE	BIT(0)
#define MT6360_SHFT_OPA_MODE	(0)

/* MT6360_PMU_CHG_CTRL2 : 0x12 */
#define MT6360_MASK_IINLMTSEL	(0x0C)
#define MT6360_SHFT_IINLMTSEL	(2)
#define MT6360_MASK_TE_EN	BIT(4)
#define MT6360_SHFT_TE_EN	(4)
#define MT6360_MASK_CFO_EN	BIT(1)
#define MT6360_SHFT_CFO_EN	(1)
#define MT6360_MASK_CHG_EN	BIT(0)
#define MT6360_SHFT_CHG_EN	(0)

/* MT6360_PMU_CHG_CTRL3 : 0x13 */
#define MT6360_MASK_AICR	(0xFC)
#define MT6360_SHFT_AICR	(2)
#define MT6360_AICR_MAXVAL	(0x3F)
#define MT6360_MASK_ILIM_EN	BIT(0)

/* MT6360_PMU_CHG_CTRL4 : 0x14 */
#define MT6360_MASK_VOREG	(0xFE)
#define MT6360_SHFT_VOREG	(1)
#define MT6360_VOREG_MAXVAL	(0x51)

/* MT6360_PMU_CHG_CTRL6 : 0x16 */
#define MT6360_MASK_MIVR	(0xFE)
#define MT6360_SHFT_MIVR	(1)
#define MT6360_MIVR_MAXVAL	(0x5F)

/* MT6360_PMU_CHG_CTRL7 : 0x17 */
#define MT6360_MASK_ICHG	(0xFC)
#define MT6360_SHFT_ICHG	(2)
#define MT6360_ICHG_MAXVAL	(0x31)

/* MT6360_PMU_CHG_CTRL9 : 0x19 */
#define MT6360_MASK_IEOC	(0xF0)
#define MT6360_SHFT_IEOC	(4)
#define MT6360_IEOC_MAXVAL	(0x0F)

/* MT6360_PMU_CHG_AICC_RESULT : 0x21 */
#define MT6360_MASK_RG_AICC_RESULT	(0xFC)
#define MT6360_SHFT_RG_AICC_RESULT	(2)

/* MT6360_PMU_CHG_CTRL10 : 0x1A */
#define MT6360_MASK_OTG_OC	(0x07)
#define MT6360_SHFT_OTG_OC	(0)
#define MT6360_OTG_OC_MAXVAL	(0x07)

/* MT6360_PMU_CHG_CTRL12 : 0x1C */
#define MT6360_MASK_TMR_EN	BIT(1)
#define MT6360_SHFT_TMR_EN	(1)

/* MT6360_PMU_CHG_CTRL13 : 0x1D */
#define MT6360_MASK_CHG_WDT_EN	BIT(7)

/* MT6360_PMU_CHG_CTRL14 : 0x1E */
#define MT6360_MASK_RG_EN_AICC		BIT(7)
#define MT6360_MASK_RG_AICC_ONCE	BIT(2)

/* MT6360_PMU_DEVICE_TYPE : 0x22 */
#define MT6360_MASK_USBCHGEN	BIT(7)
#define MT6360_SHFT_USBCHGEN	(7)
#define MT6360_MASK_DCDTOUTEN	BIT(6)
#define MT6360_SHFT_DCDTOUTEN	6

/* MT6360_PMU_USB_STATUS1 : 0x27 */
#define MT6360_MASK_USB_STATUS	(0x70)
#define MT6360_SHFT_USB_STATUS	(4)

/* MT6360_PMU_CHG_CTRL16 : 0x2A */
#define MT6360_MASK_AICC_VTH	(0xFE)
#define MT6360_SHFT_AICC_VTH	(1)
#define MT6360_AICC_VTH_MAXVAL	(0x5F)

/* MT6360_PMU_CHG_CTRL17 : 0x2B */
#define MT6360_MASK_EN_PUMPX	BIT(7)
#define MT6360_SHFT_EN_PUMPX	(7)
#define MT6360_MASK_PUMPX_20_10	BIT(6)
#define MT6360_SHFT_PUMPX_20_10	(6)
#define MT6360_MASK_PUMPX_UP_DN	BIT(5)
#define MT6360_SHFT_PUMPX_UP_DN	(5)
#define MT6360_MASK_PUMPX_DEC	(0x1F)
#define MT6360_SHFT_PUMPX_DEC	(0)
#define MT6360_PUMPX_20_MAXVAL	(0x1D)

/* MT6360_PMU_CHG_CTRL18 : 0x2C */
#define MT6360_MASK_BAT_COMP	(0x38)
#define MT6360_SHFT_BAT_COMP	(3)
#define MT6360_BAT_COMP_MAXVAL	(0x07)
#define MT6360_MASK_VCLAMP	(0x7)
#define MT6360_SHFT_VCLAMP	(0)
#define MT6360_VCLAMP_MAXVAL	(0x07)

/* MT6360_PMU_CHG_HIDDEN_CTRL2 : 0x31 */
#define MT6360_MASK_EOC_RST	BIT(7)
#define MT6360_MASK_DISCHG	BIT(2)
#define MT6360_SHFT_DISCHG	(2)

/* MT6360_PMU_CHG_STAT : 0x4A */
#define MT6360_MASK_CHG_STAT	(0xC0)
#define MT6360_SHFT_CHG_STAT	(6)
#define MT6360_MASK_CHG_BATSYSUV	BIT(1)

/* MT6360_PMU_ADC_CONFIG : 0x56 */
#define MT6360_MASK_ZCV_EN	BIT(6)
#define MT6360_SHFT_ZCV_EN	(6)

/* MT6360_PMU_CHG_CTRL19 : 0x61 */
#define MT6360_MASK_CHG_VIN_OVP_VTHSEL	(0x60)
#define MT6360_SHFT_CHG_VIN_OVP_VTHSEL	(5)

/* MT6360_PMU_FOD_CTRL : 0x65 */
#define MT6360_MASK_FOD_SWEN	BIT(7)

/* MT6360_PMU_CHG_CTRL20 : 0x66 */
#define MT6360_MASK_EN_SDI	BIT(1)

/* MT6360_PMU_USBID_CTRL1 : 0x6D */
#define MT6360_MASK_USBID_EN	BIT(7)
#define MT6360_SHFT_ID_RPULSEL	5
#define MT6360_MASK_ID_RPULSEL	0x60
#define MT6360_MASK_ISTDET	0x1C
#define MT6360_SHFT_ISTDET	2

/* MT6360_PMU_USBID_CTRL2 : 0x6E */
#define MT6360_MASK_IDTD	0xE0
#define MT6360_MASK_USBID_FLOAT	BIT(1)

/* MT6360_PMU_FLED_EN : 0x7E */
#define MT6360_MASK_STROBE_EN	BIT(2)

/* MT6360_PMU_CHG_STAT1 : 0xE0 */
#define MT6360_MASK_PWR_RDY_EVT	BIT(7)
#define MT6360_SHFT_PWR_RDY_EVT	(7)
#define MT6360_MASK_MIVR_EVT	BIT(6)
#define MT6360_SHFT_MIVR_EVT	(6)
#define MT6360_MASK_CHG_TREG	BIT(4)
#define MT6360_SHFT_CHG_TREG	(4)

/* MT6360_PMU_CHG_STAT4 : 0xE3 */
#define MT6360_MASK_CHG_TMRI	BIT(3)

#endif /* __MT6360_PMU_CHG_H */
