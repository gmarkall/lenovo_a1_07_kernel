#ifndef WAKEUP_SOURCE_H
#define WAKEUP_SOURCE_H


#define OFFSET_CONTROL_PADCONF_WAKEUPENABLE 	(0x1 << 14)
#define OFFSET_CONTROL_PADCONF_WAKEUPEVENT 	(0x1 << 15)

// PIH_ISR_P1 register
#define TPS65921_PIH_ISR_GPIO 					(0x1 << 0)
#define TPS65921_PIH_ISR_KEYPAD 				(0x1 << 1)
#define TPS65921_PIH_ISR_POWER_MANAGEMENT 	(0x1 << 5)

// PWR_ISR1 register
#define TPS65921_PWR_ISR_PWRON 		(0x1 << 0)
#define TPS65921_PWR_ISR_USB_PRES		(0x1 << 2)
#define TPS65921_PWR_ISR_RTC_IT		(0x1 << 3)

enum wkup_reason{
	WKUP_PWR_KEY,
	WKUP_KEYPAD,
	WKUP_RTC,
	WKUP_CABLE,
	WKUP_SD,
	WKUP_SIM_CARD,	
	WKUP_BATT_LOW,
	WKUP_CHG_FULL,
	WKUP_UNKNOWN,	
};

struct wkup_source {
	u16 pad_offset;
	const char *name;
	enum wkup_reason reason;
};

static struct wkup_source ep10_wkup_source[] = {
		{.pad_offset=0x01E0, .name="wkup_pwr_key",	.reason=WKUP_PWR_KEY},		// GPIO_0, SYS_nIRQ
		{.pad_offset=0x01E0, .name="wkup_keypad",	.reason=WKUP_KEYPAD},		// GPIO_0, SYS_nIRQ
		{.pad_offset=0x01E0, .name="wkup_rtc",		.reason=WKUP_RTC},			// GPIO_0, SYS_nIRQ
		{.pad_offset=0x01E0, .name="wkup_cable",	.reason=WKUP_CABLE},		// GPIO_0, SYS_nIRQ
		{.pad_offset=0x01E0, .name="wkup_sd",		.reason=WKUP_SD},			// GPIO_0, SYS_nIRQ
		{.pad_offset=0x01E0, .name="wkup_sim_card",	.reason=WKUP_SIM_CARD},		// GPIO_0, SYS_nIRQ		
		{.pad_offset=0x009E, .name="wkup_batt_low",	.reason=WKUP_BATT_LOW},		// GPIO_44, VOLT_INT
		{.pad_offset=0x017E, .name="wkup_chg_full",	.reason=WKUP_CHG_FULL},		// GPIO_149, CHG	
		{.pad_offset=0,      .name="wkup_unknown",  .reason=WKUP_UNKNOWN},
};

// record for the reason from PMIC
extern u8 g_twl4030_pih_isr;
extern u8 g_twl4030_sih_isr;
extern u8 wakeup_stage;
// record for the reason from OMAP
extern u32 g_core_wkst;
extern u32 g_core_wkst3;
extern u32 g_wkup_wkst;
extern u32 g_per_wkst;
extern u32 g_usbhost_wkst;

extern int omap3_get_wakeup_reason(void);

#endif // WAKEUP_SOURCE_H
