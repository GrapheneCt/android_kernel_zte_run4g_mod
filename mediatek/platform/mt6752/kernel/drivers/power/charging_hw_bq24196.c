/*****************************************************************************
 *
 * Filename:
 * ---------
 *    charging_pmic.c
 *
 * Project:
 * --------
 *   ALPS_Software
 *
 * Description:
 * ------------
 *   This file implements the interface between BMT and ADC scheduler.
 *
 * Author:
 * -------
 *  Oscar Liu
 *
 *============================================================================
  * $Revision:   1.0  $
 * $Modtime:   11 Aug 2005 10:28:16  $
 * $Log:   //mtkvs01/vmdata/Maui_sw/archives/mcu/hal/peripheral/inc/bmt_chr_setting.h-arc  $
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <mach/charging.h>
#include <mach/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_gpio.h>
#include <mach/mt_sleep.h>
#include <mach/mt_boot.h>
#include <mach/system.h>
#include <cust_gpio_usage.h>
#include <cust_charging.h>
#include "bq24196.h"
#include <mach/mt6311.h>
#ifdef MTK_DUAL_INPUT_CHARGER_SUPPORT
#include <mach/diso.h>
#include "cust_diso.h"
#include <linux/of.h>
#include <linux/of_irq.h>
#ifdef MTK_DISCRETE_SWITCH
#include <mach/eint.h>
#include <cust_eint.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#endif
#if !defined(MTK_AUXADC_IRQ_SUPPORT)
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#endif
#endif

// ============================================================ //
// Define
// ============================================================ //
#define STATUS_OK	0
#define STATUS_UNSUPPORTED	-1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))

// ============================================================ //
// Global variable
// ============================================================ //
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
#define WIRELESS_CHARGER_EXIST_STATE 0

	#if defined(GPIO_PWR_AVAIL_WLC)
		kal_uint32 wireless_charger_gpio_number = GPIO_PWR_AVAIL_WLC; 
	#else
		kal_uint32 wireless_charger_gpio_number = 0; 
	#endif
    
#endif

static CHARGER_TYPE g_charger_type = CHARGER_UNKNOWN;

kal_bool charging_type_det_done = KAL_TRUE;

const kal_uint32 VBAT_CV_VTH[]=
{
	3504000,    3520000,    3536000,    3552000,
	3568000,    3584000,    3600000,    3616000,
	3632000,    3648000,    3664000,    3680000,
	3696000,    3712000,	3728000,    3744000,
	3760000,    3776000,    3792000,    3808000,
	3824000,    3840000,    3856000,    3872000,
	3888000,    3904000,    3920000,    3936000,
	3952000,    3968000,    3984000,    4000000,
	4016000,    4032000,    4048000,    4064000,
	4080000,    4096000,    4112000,    4128000,
	4144000,    4160000,    4176000,    4192000,	
	4208000,    4224000,    4240000,    4256000,
	4272000,    4288000,    4304000
};

const kal_uint32 CS_VTH[]=
{
	51200,  57600,  64000,  70400,
	76800,  83200,  89600,  96000,
	102400, 108800, 115200, 121600,
	128000, 134400, 140800, 147200,
	153600, 160000, 166400, 172800,
	179200, 185600, 192000, 198400,
	204800, 211200, 217600, 224000
}; 

const kal_uint32 INPUT_CS_VTH[]=
{
 CHARGE_CURRENT_100_00_MA,  CHARGE_CURRENT_150_00_MA,	CHARGE_CURRENT_500_00_MA,  CHARGE_CURRENT_900_00_MA,
 CHARGE_CURRENT_1200_00_MA, CHARGE_CURRENT_1500_00_MA,  CHARGE_CURRENT_2000_00_MA,  CHARGE_CURRENT_MAX
}; 

const kal_uint32 VCDT_HV_VTH[]=
{
  BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V,	  BATTERY_VOLT_04_300000_V,   BATTERY_VOLT_04_350000_V,
  BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V,	  BATTERY_VOLT_04_500000_V,   BATTERY_VOLT_04_550000_V,
  BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V,	  BATTERY_VOLT_06_500000_V,   BATTERY_VOLT_07_000000_V,
  BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V,	  BATTERY_VOLT_09_500000_V,   BATTERY_VOLT_10_500000_V		  
};

#ifdef MTK_DUAL_INPUT_CHARGER_SUPPORT
#ifndef CUST_GPIO_VIN_SEL
#define CUST_GPIO_VIN_SEL 18
#endif
#if !defined(MTK_AUXADC_IRQ_SUPPORT)
#define SW_POLLING_PERIOD 100 //100 ms
#define MSEC_TO_NSEC(x)		(x * 1000000UL)

static DEFINE_MUTEX(diso_polling_mutex);
static DECLARE_WAIT_QUEUE_HEAD(diso_polling_thread_wq);
static struct hrtimer diso_kthread_timer;
static kal_bool diso_thread_timeout = KAL_FALSE;
static struct delayed_work diso_polling_work;
static void diso_polling_handler(struct work_struct *work);
static DISO_Polling_Data DISO_Polling;
#else
DISO_IRQ_Data DISO_IRQ;
#endif
int g_diso_state  = 0;
int vin_sel_gpio_number   = (CUST_GPIO_VIN_SEL | 0x80000000); 

static char *DISO_state_s[8] = {
  "IDLE",
  "OTG_ONLY",
  "USB_ONLY",
  "USB_WITH_OTG",
  "DC_ONLY",
  "DC_WITH_OTG",
  "DC_WITH_USB",
  "DC_USB_OTG",
};
#endif

// ============================================================ //
// function prototype
// ============================================================ //

 
// ============================================================ //
//extern variable
// ============================================================ //

// ============================================================ //
//extern function
// ============================================================ //
extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
extern bool mt_usb_is_device(void);
extern void Charger_Detect_Init(void);
extern void Charger_Detect_Release(void);
extern int hw_charging_get_charger_type(void);
extern void mt_power_off(void);
extern kal_uint32 mt6311_get_chip_id(void);
extern int is_mt6311_exist(void);
extern int is_mt6311_sw_ready(void);

static kal_uint32 charging_error = false;
static kal_uint32 charging_get_error_state(void);
static kal_uint32 charging_set_error_state(void *data);
// ============================================================ //
kal_uint32 charging_value_to_parameter(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	if (val < array_size)
	{
		return parameter[val];
	}
	else
	{
		battery_xlog_printk(BAT_LOG_CRTI, "Can't find the parameter \r\n");	
		return parameter[0];
	}
}

kal_uint32 charging_parameter_to_value(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	kal_uint32 i;

	battery_xlog_printk(BAT_LOG_FULL, "array_size = %d \r\n", array_size);

	for(i=0;i<array_size;i++)
	{
		if (val == *(parameter + i))
		{
				return i;
		}
	}

	battery_xlog_printk(BAT_LOG_CRTI, "NO register value match. val=%d\r\n", val);
	//TODO: ASSERT(0);	// not find the value
	return 0;
}

static kal_uint32 bmt_find_closest_level(const kal_uint32 *pList,kal_uint32 number,kal_uint32 level)
{
	kal_uint32 i;
	kal_uint32 max_value_in_last_element;
 
	if(pList[0] < pList[1])
		max_value_in_last_element = KAL_TRUE;
	else
		max_value_in_last_element = KAL_FALSE;

	if(max_value_in_last_element == KAL_TRUE)
	{
		for(i = (number-1); i != 0; i--)	 //max value in the last element
		{
			if(pList[i] <= level)
			{
				return pList[i];
			}	  
		}

		battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, small value first \r\n");
		return pList[0];
		//return CHARGE_CURRENT_0_00_MA;
	}
	else
	{
		for(i = 0; i< number; i++)  // max value in the first element
		{
			if(pList[i] <= level)
			{
				return pList[i];
			}	  
		}

		battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, large value first \r\n"); 	 
		return pList[number -1];
  		//return CHARGE_CURRENT_0_00_MA;
	}
}

static kal_uint32 is_chr_det(void)
{
	kal_uint32 val=0;
	val = mt6325_upmu_get_rgs_chrdet();
	battery_xlog_printk(BAT_LOG_CRTI,"[is_chr_det] %d\n", val);
 
	return val;
}

static kal_uint32 charging_hw_init(void *data)
{
	kal_uint32 status = STATUS_OK;

    bq24196_set_en_hiz(0x0);
	bq24196_set_vindpm(0xA); //VIN DPM check 4.68V
	bq24196_set_reg_rst(0x0);
	bq24196_set_wdt_rst(0x1); //Kick watchdog	
	bq24196_set_sys_min(0x5); //Minimum system voltage 3.5V	
	bq24196_set_iprechg(0x3); //Precharge current 512mA
	bq24196_set_iterm(0x0); //Termination current 128mA

	#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
	bq24196_set_vreg(0x32); //VREG 4.304V
	#else
	bq24196_set_vreg(0x2C); //VREG 4.208V
	#endif    

	bq24196_set_batlowv(0x1); //BATLOWV 3.0V
	bq24196_set_vrechg(0x0); //VRECHG 0.1V (4.108V)
	bq24196_set_en_term(0x1); //Enable termination
	bq24196_set_term_stat(0x0); //Match ITERM
	bq24196_set_watchdog(0x1); //WDT 40s
	bq24196_set_en_timer(0x0); //Disable charge timer
	bq24196_set_int_mask(0x0); //Disable fault interrupt

	#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	if(wireless_charger_gpio_number!=0)
	{
		mt_set_gpio_mode(wireless_charger_gpio_number,0); // 0:GPIO mode
		mt_set_gpio_dir(wireless_charger_gpio_number,0); // 0: input, 1: output
	}
	#endif

	#ifdef MTK_DUAL_INPUT_CHARGER_SUPPORT
	mt_set_gpio_mode(vin_sel_gpio_number,0); // 0:GPIO mode
	mt_set_gpio_dir(vin_sel_gpio_number,0); // 0: input, 1: output
	#endif
	return status;
}

static kal_uint32 charging_dump_register(void *data)
{
	kal_uint32 status = STATUS_OK;

	battery_xlog_printk(BAT_LOG_CRTI, "charging_dump_register\r\n");
	bq24196_dump_register();

	return status;
}	

static kal_uint32 charging_enable(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 enable = *(kal_uint32*)(data);

	if(KAL_TRUE == enable)
	{
		bq24196_set_en_hiz(0x0);	            	
		bq24196_set_chg_config(0x1); // charger enable
	}
	else
	{
		#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if(mt_usb_is_device())
		#endif 			
		{
			bq24196_set_chg_config(0x0);
			if (charging_get_error_state()) {
				battery_xlog_printk(BAT_LOG_CRTI,"[charging_enable] bq24196_set_hz_mode(0x1)\n");
				bq24196_set_en_hiz(0x1);	// disable power path
			}
		}
		#if defined(MTK_DUAL_INPUT_CHARGER_SUPPORT)
		bq24196_set_chg_config(0x0);
		bq24196_set_en_hiz(0x1);	// disable power path
		#endif
	}

	return status;
}

static kal_uint32 charging_set_cv_voltage(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 array_size;
	kal_uint32 set_cv_voltage;
	kal_uint16 register_value;
	kal_uint32 cv_value = *(kal_uint32 *)(data);	
	static kal_int16 pre_register_value = -1;

	#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
	//highest of voltage will be 4.3V, because powerpath limitation
	if(cv_value >= BATTERY_VOLT_04_300000_V)
		cv_value = 4304000;
	#endif

	//use nearest value
	if(BATTERY_VOLT_04_200000_V == cv_value)
		cv_value = 4208000;

	array_size = GETARRAYNUM(VBAT_CV_VTH);
	set_cv_voltage = bmt_find_closest_level(VBAT_CV_VTH, array_size, cv_value);
	register_value = charging_parameter_to_value(VBAT_CV_VTH, array_size, set_cv_voltage);

	//PCB workaround
	if(mt6325_upmu_get_swcid() == PMIC6325_E1_CID_CODE)
	{
		#if defined(CV_E1_INTERNAL)
		bq24196_set_vreg(0x1F);//4.0v
		#else
		bq24196_set_vreg(0x14);//3.8v
		#endif
		battery_xlog_printk(BAT_LOG_CRTI,"[charging_set_cv_voltage] set low CV by 6325 E1\n");
	}
	else
	{
		if(is_mt6311_exist())
		{
			if(mt6311_get_chip_id()==PMIC6311_E1_CID_CODE)
			{
				#if defined(CV_E1_INTERNAL)
				bq24196_set_vreg(0x1F);//4.0v
				#else
				bq24196_set_vreg(0x14);//3.8v
				#endif
				battery_xlog_printk(BAT_LOG_CRTI,"[charging_set_cv_voltage] set low CV by 6311 E1\n");
			}
			else
			{
				if (pre_register_value != register_value) {
						battery_xlog_printk(BAT_LOG_CRTI,"[charging_set_cv_voltage] disable charging\n");
						bq24196_set_chg_config(0);
				}

				bq24196_set_vreg(register_value);
				if (pre_register_value != register_value)
					bq24196_set_chg_config(1);

				pre_register_value = register_value;
			}
		}
		else
		{
			bq24196_set_vreg(register_value);
		}
	}

	return status;
}

static kal_uint32 charging_get_current(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 array_size;
	kal_uint8 reg_value;
	kal_uint8 ret_val=0;    
	kal_uint8 ret_force_20pct=0;

	//Get current level
	bq24196_read_interface(bq24196_CON2, &ret_val, CON2_ICHG_MASK, CON2_ICHG_SHIFT);

	//Get Force 20% option
	bq24196_read_interface(bq24196_CON2, &ret_force_20pct, CON2_FORCE_20PCT_MASK, CON2_FORCE_20PCT_SHIFT);

	//Parsing
	ret_val = (ret_val*64) + 512;

	if (ret_force_20pct == 0)
	{
		//Get current level
		//array_size = GETARRAYNUM(CS_VTH);
		//*(kal_uint32 *)data = charging_value_to_parameter(CS_VTH,array_size,reg_value);
		*(kal_uint32 *)data = ret_val;
	}   
	else
	{
		//Get current level
		//array_size = GETARRAYNUM(CS_VTH_20PCT);
		//*(kal_uint32 *)data = charging_value_to_parameter(CS_VTH,array_size,reg_value);
		//return (int)(ret_val<<1)/10;
		*(kal_uint32 *)data = (int)(ret_val<<1)/10;
	}

	return status;
}

static kal_uint32 charging_set_current(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;
	kal_uint32 current_value = *(kal_uint32 *)data;

	array_size = GETARRAYNUM(CS_VTH);
	set_chr_current = bmt_find_closest_level(CS_VTH, array_size, current_value);
	register_value = charging_parameter_to_value(CS_VTH, array_size ,set_chr_current);
	bq24196_set_ichg(register_value);		

	return status;
} 	

static kal_uint32 charging_set_input_current(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 current_value = *(kal_uint32 *)data;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;

	array_size = GETARRAYNUM(INPUT_CS_VTH);
	set_chr_current = bmt_find_closest_level(INPUT_CS_VTH, array_size, current_value);
	register_value = charging_parameter_to_value(INPUT_CS_VTH, array_size ,set_chr_current);

	bq24196_set_iinlim(register_value);	

	return status;
} 	

static kal_uint32 charging_get_charging_status(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 ret_val;

	ret_val = bq24196_get_chrg_stat();

	if(ret_val == 0x3)
		*(kal_uint32 *)data = KAL_TRUE;
	else
		*(kal_uint32 *)data = KAL_FALSE;

	return status;
} 	

static kal_uint32 charging_reset_watch_dog_timer(void *data)
{
	kal_uint32 status = STATUS_OK;

	battery_xlog_printk(BAT_LOG_FULL, "charging_reset_watch_dog_timer\r\n");

	bq24196_set_wdt_rst(0x1); //Kick watchdog

	return status;
}
 
static kal_uint32 charging_set_hv_threshold(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 set_hv_voltage;
	kal_uint32 array_size;
	kal_uint16 register_value;
	kal_uint32 voltage = *(kal_uint32*)(data);

	array_size = GETARRAYNUM(VCDT_HV_VTH);
	set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
	register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size ,set_hv_voltage);
	mt6325_upmu_set_rg_vcdt_hv_vth(register_value);

	return status;
}

static kal_uint32 charging_get_hv_status(void *data)
{
	kal_uint32 status = STATUS_OK;
	#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool*)(data) = 0;
	battery_xlog_printk(BAT_LOG_CRTI,"[charging_get_hv_status] charger ok for bring up.\n");
	#else
	*(kal_bool*)(data) = mt6325_upmu_get_rgs_vcdt_hv_det();
	#endif

	return status;
}

static kal_uint32 charging_get_battery_status(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 val = 0;

	#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool*)(data) = 0; // battery exist
	battery_xlog_printk(BAT_LOG_CRTI,"[charging_get_battery_status] battery exist for bring up.\n");
	#else
	pmic_read_interface( MT6325_CHR_CON7, &val, MT6325_PMIC_BATON_TDET_EN_MASK, MT6325_PMIC_BATON_TDET_EN_SHIFT);
	battery_xlog_printk(BAT_LOG_FULL,"[charging_get_battery_status] BATON_TDET_EN = %d\n", val);

	if (val) {
		mt6325_upmu_set_baton_tdet_en(1);	
		mt6325_upmu_set_rg_baton_en(1);
		*(kal_bool*)(data) = mt6325_upmu_get_rgs_baton_undet();
	} else {
		*(kal_bool*)(data) =  KAL_FALSE;
	}
	#endif  

	return status;
}

static kal_uint32 charging_get_charger_det_status(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 val = 0;

	#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
		val = 1;
		battery_xlog_printk(BAT_LOG_CRTI,"[charging_get_charger_det_status] charger exist for bring up.\n"); 
	#else    
		#if !defined(MTK_DUAL_INPUT_CHARGER_SUPPORT)
			val = mt6325_upmu_get_rgs_chrdet();
		#else
			if(((g_diso_state >> 1) & 0x3) != 0x0 || mt6325_upmu_get_rgs_chrdet())
				val = KAL_TRUE;
			else
				val = KAL_FALSE;
		#endif
	#endif

	*(kal_bool*)(data) = val;
	if(val == 0)
		g_charger_type = CHARGER_UNKNOWN;

	return status;
}

kal_bool charging_type_detection_done(void)
{
	return charging_type_det_done;
}

static kal_uint32 charging_get_charger_type(void *data)
{
	kal_uint32 status = STATUS_OK;

	#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
		*(CHARGER_TYPE*)(data) = STANDARD_HOST;
	#else
		#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
			int wireless_state = 0;
			if(wireless_charger_gpio_number!=0)
			{
				wireless_state = mt_get_gpio_in(wireless_charger_gpio_number);
				if(wireless_state == WIRELESS_CHARGER_EXIST_STATE)
				{
					*(CHARGER_TYPE*)(data) = WIRELESS_CHARGER;
					battery_xlog_printk(BAT_LOG_CRTI, "WIRELESS_CHARGER!\n");
					return status;
				}
			}
			else
			{
				battery_xlog_printk(BAT_LOG_CRTI, "wireless_charger_gpio_number=%d\n", wireless_charger_gpio_number);
			}

			if(g_charger_type!=CHARGER_UNKNOWN && g_charger_type!=WIRELESS_CHARGER)
			{
				*(CHARGER_TYPE*)(data) = g_charger_type;
				battery_xlog_printk(BAT_LOG_CRTI, "return %d!\n", g_charger_type);
				return status;
			}
		#endif

		if(is_chr_det()==0)
		{
			g_charger_type = CHARGER_UNKNOWN; 
			*(CHARGER_TYPE*)(data) = CHARGER_UNKNOWN;
			battery_xlog_printk(BAT_LOG_CRTI, "[charging_get_charger_type] return CHARGER_UNKNOWN\n");
			return status;
		}

		charging_type_det_done = KAL_FALSE;
		*(CHARGER_TYPE*)(data) = hw_charging_get_charger_type();
		charging_type_det_done = KAL_TRUE;
		g_charger_type = *(CHARGER_TYPE*)(data);

	#endif

	return status;
}

static kal_uint32 charging_get_is_pcm_timer_trigger(void *data)
{
    kal_uint32 status = STATUS_OK;
	#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
		*(kal_bool*)(data) = KAL_FALSE;
	#else 

	    if(slp_get_wake_reason() == WR_PCM_TIMER)
	        *(kal_bool*)(data) = KAL_TRUE;
	    else
	        *(kal_bool*)(data) = KAL_FALSE;

	    battery_xlog_printk(BAT_LOG_CRTI, "slp_get_wake_reason=%d\n", slp_get_wake_reason());
    #endif
    return status;
}

static kal_uint32 charging_set_platform_reset(void *data)
{
	kal_uint32 status = STATUS_OK;
	#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)	 
	#else 
		battery_xlog_printk(BAT_LOG_CRTI, "charging_set_platform_reset\n");
		arch_reset(0,NULL);
	#endif
	return status;
}

static kal_uint32 charging_get_platfrom_boot_mode(void *data)
{
    kal_uint32 status = STATUS_OK;
    #if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)   
    #else  
        *(kal_uint32*)(data) = get_boot_mode();
        battery_xlog_printk(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
    #endif
    return status;
}

static kal_uint32 charging_set_power_off(void *data)
{
    kal_uint32 status = STATUS_OK;
    #if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
    #else
        battery_xlog_printk(BAT_LOG_CRTI, "charging_set_power_off\n");
   	    mt_power_off();
    #endif

    return status;
}

static kal_uint32 charging_get_power_source(void *data)
{
	kal_uint32 status = STATUS_OK;

#if 0	//#if defined(MTK_POWER_EXT_DETECT)
	if (MT_BOARD_PHONE == mt_get_board_type())
		*(kal_bool *)data = KAL_FALSE;
	else
		*(kal_bool *)data = KAL_TRUE;
#else
	*(kal_bool *)data = KAL_FALSE;
#endif

	return status;
}

static kal_uint32 charging_get_csdac_full_flag(void *data)
{
	return STATUS_UNSUPPORTED;
}

static kal_uint32 charging_set_ta_current_pattern(void *data)
{
    kal_uint32 increase = *(kal_uint32*)(data);
    kal_uint32 charging_status = KAL_FALSE;

    #if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
    BATTERY_VOLTAGE_ENUM cv_voltage = BATTERY_VOLT_04_340000_V;
    #else
    BATTERY_VOLTAGE_ENUM cv_voltage = BATTERY_VOLT_04_200000_V;
    #endif

    charging_get_charging_status(&charging_status);
    if(KAL_FALSE == charging_status)
    {
        charging_set_cv_voltage(&cv_voltage);  //Set CV 
        bq24196_set_ichg(0x0);  //Set charging current 500ma
        bq24196_set_chg_config(0x1);  //Enable Charging
    }

    if(increase == KAL_TRUE)
    {
        bq24196_set_iinlim(0x0); /* 100mA */
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() on 1");
        msleep(85);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() off 1");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() on 2");
        msleep(85);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() off 2");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() on 3");
        msleep(281);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() off 3");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() on 4");
        msleep(281);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() off 4");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() on 5");
        msleep(281);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() off 5");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() on 6");
        msleep(485);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_increase() off 6");
        msleep(50);
        
        battery_xlog_printk(BAT_LOG_CRTI, "mtk_ta_increase() end \n");
        
        bq24196_set_iinlim(0x2); /* 500mA */
        msleep(200);
    }
    else
    {
        bq24196_set_iinlim(0x0); /* 100mA */
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() on 1");
        msleep(281);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() off 1");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() on 2");
        msleep(281);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() off 2");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() on 3");
        msleep(281);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() off 3");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() on 4");
        msleep(85);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() off 4");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() on 5");
        msleep(85);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() off 5");
        msleep(85);
        
        bq24196_set_iinlim(0x2); /* 500mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() on 6");
        msleep(485);
        
        bq24196_set_iinlim(0x0); /* 100mA */
        battery_xlog_printk(BAT_LOG_FULL, "mtk_ta_decrease() off 6");
        msleep(50);
        
        battery_xlog_printk(BAT_LOG_CRTI, "mtk_ta_decrease() end \n");
        
        bq24196_set_iinlim(0x2); /* 500mA */
    }

    return STATUS_OK;
}

#if defined(MTK_DUAL_INPUT_CHARGER_SUPPORT)
void set_vusb_auxadc_irq(bool enable, bool flag)
{
#if !defined(MTK_AUXADC_IRQ_SUPPORT)
	hrtimer_cancel(&diso_kthread_timer);

	DISO_Polling.reset_polling = KAL_TRUE;
	DISO_Polling.vusb_polling_measure.notify_irq_en = enable;
	DISO_Polling.vusb_polling_measure.notify_irq = flag;

	hrtimer_start(&diso_kthread_timer, ktime_set(0, MSEC_TO_NSEC(SW_POLLING_PERIOD)), HRTIMER_MODE_REL);
#else
	kal_uint16 threshold = 0;
	if (enable) {
		if (flag == 0)
			threshold = DISO_IRQ.vusb_measure_channel.falling_threshold;
		else
			threshold = DISO_IRQ.vusb_measure_channel.rising_threshold;

		threshold = (threshold *R_DISO_VBUS_PULL_DOWN)/(R_DISO_VBUS_PULL_DOWN + R_DISO_VBUS_PULL_UP);
		mt_auxadc_enableBackgroundDection(DISO_IRQ.vusb_measure_channel.number, threshold, \
					DISO_IRQ.vusb_measure_channel.period, DISO_IRQ.vusb_measure_channel.debounce, flag);
	} else {
		mt_auxadc_disableBackgroundDection(DISO_IRQ.vusb_measure_channel.number);
	}
#endif
	battery_xlog_printk(BAT_LOG_FULL, " [%s] enable: %d, flag: %d!\n", __func__, enable, flag);
}

void set_vdc_auxadc_irq(bool enable, bool flag)
{
#if !defined(MTK_AUXADC_IRQ_SUPPORT)
	hrtimer_cancel(&diso_kthread_timer);

	DISO_Polling.reset_polling = KAL_TRUE;
	DISO_Polling.vdc_polling_measure.notify_irq_en = enable;
	DISO_Polling.vdc_polling_measure.notify_irq = flag;

	hrtimer_start(&diso_kthread_timer, ktime_set(0, MSEC_TO_NSEC(SW_POLLING_PERIOD)), HRTIMER_MODE_REL);
#else
	kal_uint16 threshold = 0;
	if(enable) {
		if(flag == 0)
			threshold = DISO_IRQ.vdc_measure_channel.falling_threshold;
		else
			threshold = DISO_IRQ.vdc_measure_channel.rising_threshold;

		threshold = (threshold *R_DISO_DC_PULL_DOWN)/(R_DISO_DC_PULL_DOWN + R_DISO_DC_PULL_UP);
		mt_auxadc_enableBackgroundDection(DISO_IRQ.vdc_measure_channel.number, threshold, \
					DISO_IRQ.vdc_measure_channel.period, DISO_IRQ.vdc_measure_channel.debounce, flag);
	} else {
		mt_auxadc_disableBackgroundDection(DISO_IRQ.vdc_measure_channel.number);
	}
#endif
	battery_xlog_printk(BAT_LOG_FULL, " [%s] enable: %d, flag: %d!\n", __func__, enable, flag);
}

#if !defined(MTK_AUXADC_IRQ_SUPPORT)
static void diso_polling_handler(struct work_struct *work)
{
	int trigger_channel = -1;
	int trigger_flag = -1;

	if(DISO_Polling.vdc_polling_measure.notify_irq_en)
		trigger_channel = AP_AUXADC_DISO_VDC_CHANNEL;
	else if(DISO_Polling.vusb_polling_measure.notify_irq_en)
		trigger_channel = AP_AUXADC_DISO_VUSB_CHANNEL;

	battery_xlog_printk(BAT_LOG_CRTI, "[DISO]auxadc handler triggered\n" );
	switch(trigger_channel)
	{
		case AP_AUXADC_DISO_VDC_CHANNEL:
			trigger_flag = DISO_Polling.vdc_polling_measure.notify_irq;
			battery_xlog_printk(BAT_LOG_CRTI, "[DISO]VDC IRQ triggered, channel ==%d, flag ==%d\n", trigger_channel, trigger_flag );
			#ifdef MTK_DISCRETE_SWITCH /*for DSC DC plugin handle */
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_FALLING);
			if (trigger_flag == DISO_IRQ_RISING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[2]);
			}
			#else //for load switch OTG leakage handle
			set_vdc_auxadc_irq(DISO_IRQ_ENABLE, (~trigger_flag) & 0x1);
			if (trigger_flag == DISO_IRQ_RISING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_ONLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[5]);
			} else if (trigger_flag == DISO_IRQ_FALLING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_ONLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[1]);
			}
			else
				battery_xlog_printk(BAT_LOG_CRTI, "[%s] wrong trigger flag!\n",__func__);
			#endif
			break;
		case AP_AUXADC_DISO_VUSB_CHANNEL:
			trigger_flag = DISO_Polling.vusb_polling_measure.notify_irq;
			battery_xlog_printk(BAT_LOG_CRTI, "[DISO]VUSB IRQ triggered, channel ==%d, flag ==%d\n", trigger_channel, trigger_flag);
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			if(trigger_flag == DISO_IRQ_FALLING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[4]);
			} else if (trigger_flag == DISO_IRQ_RISING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[6]);
			}
			else
				battery_xlog_printk(BAT_LOG_CRTI, "[%s] wrong trigger flag!\n",__func__);
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, (~trigger_flag)&0x1);
			break;
		default:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			battery_xlog_printk(BAT_LOG_CRTI, "[DISO]VUSB auxadc IRQ triggered ERROR OR TEST\n", trigger_channel);
			return; /* in error or unexecpt state just return */
	}

	g_diso_state = *(int*)&DISO_data.diso_state;
	battery_xlog_printk(BAT_LOG_CRTI, "[DISO]g_diso_state: 0x%x\n", g_diso_state);
	DISO_data.irq_callback_func(0, NULL);

	return ;
}
#else
static irqreturn_t diso_auxadc_irq_handler(int irq, void *dev_id)
{
	int trigger_channel = -1;
	int trigger_flag = -1;
	trigger_channel = mt_auxadc_getCurrentChannel();
	battery_xlog_printk(BAT_LOG_CRTI, "[DISO]auxadc handler triggered\n" );
	switch(trigger_channel)
	{
		case AP_AUXADC_DISO_VDC_CHANNEL:
			trigger_flag = mt_auxadc_getCurrentTrigger();
			battery_xlog_printk(BAT_LOG_CRTI, "[DISO]VDC IRQ triggered, channel ==%d, flag ==%d\n", trigger_channel, trigger_flag );
			#ifdef MTK_DISCRETE_SWITCH /*for DSC DC plugin handle */
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_FALLING);
			if (trigger_flag == DISO_IRQ_RISING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[2]);
			}
			#else //for load switch OTG leakage handle
			set_vdc_auxadc_irq(DISO_IRQ_ENABLE, (~trigger_flag) & 0x1);
			if (trigger_flag == DISO_IRQ_RISING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_ONLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[5]);
			} else {
				DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_ONLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[1]);
			}
			#endif
			break;
		case AP_AUXADC_DISO_VUSB_CHANNEL:
			trigger_flag = mt_auxadc_getCurrentTrigger();
			battery_xlog_printk(BAT_LOG_CRTI, "[DISO]VUSB IRQ triggered, channel ==%d, flag ==%d\n", trigger_channel, trigger_flag);
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			if(trigger_flag == DISO_IRQ_FALLING) {
				DISO_data.diso_state.pre_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[4]);
			} else {
				DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
				DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
				DISO_data.diso_state.cur_vusb_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
				DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " cur diso_state is %s!\n",DISO_state_s[6]);
			}

			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, (~trigger_flag)&0x1);
			break;
		default:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			battery_xlog_printk(BAT_LOG_CRTI, "[DISO]VUSB auxadc IRQ triggered ERROR OR TEST\n", trigger_channel);
			return IRQ_HANDLED; /* in error or unexecpt state just return */
	}
	g_diso_state = *(int*)&DISO_data.diso_state;
	return IRQ_WAKE_THREAD;
}
#endif

#if defined(MTK_DISCRETE_SWITCH) && defined(MTK_DSC_USE_EINT)
void vdc_eint_handler()
{
	battery_xlog_printk(BAT_LOG_CRTI, "[diso_eint] vdc eint irq triger\n");
	DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
	mt_eint_mask(CUST_EINT_VDC_NUM); 
	do_chrdet_int_task();
}
#endif

static kal_uint32 diso_get_current_voltage(int Channel)
{
    int ret = 0, data[4], i, ret_value = 0, ret_temp = 0, times = 5;

    if( IMM_IsAdcInitReady() == 0 )
    {
        battery_xlog_printk(BAT_LOG_CRTI, "[DISO] AUXADC is not ready");
        return 0;
    }

    i = times;
    while (i--)
    {
        ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);

        if(ret_value == 0) {
            ret += ret_temp;
        } else {
            times = times > 1 ? times - 1 : 1;
            battery_xlog_printk(BAT_LOG_CRTI, "[diso_get_current_voltage] ret_value=%d, times=%d\n",
                ret_value, times);
        }
    }

    ret = ret*1500/4096 ;
    ret = ret/times;

    return  ret;
}

static void _get_diso_interrupt_state(void)
{
	int vol = 0;
	int diso_state =0;
	int check_times = 30;
	kal_bool vin_state = KAL_FALSE;

	#ifndef VIN_SEL_FLAG
	mdelay(AUXADC_CHANNEL_DELAY_PERIOD);
	#endif

	vol = diso_get_current_voltage(AP_AUXADC_DISO_VDC_CHANNEL);
	vol =(R_DISO_DC_PULL_UP + R_DISO_DC_PULL_DOWN)*100*vol/(R_DISO_DC_PULL_DOWN)/100;
	battery_xlog_printk(BAT_LOG_CRTI, "[DISO]  Current DC voltage mV = %d\n", vol);

	#ifdef VIN_SEL_FLAG
	/* set gpio mode for kpoc issue as DWS has no default setting */
	mt_set_gpio_mode(vin_sel_gpio_number,0); // 0:GPIO mode
	mt_set_gpio_dir(vin_sel_gpio_number,0); // 0: input, 1: output

	if (vol > VDC_MIN_VOLTAGE/1000 && vol < VDC_MAX_VOLTAGE/1000) {
		/* make sure load switch already switch done */
		do{
			check_times--;
			#ifdef VIN_SEL_FLAG_DEFAULT_LOW
			vin_state = mt_get_gpio_in(vin_sel_gpio_number);
			#else
			vin_state = mt_get_gpio_in(vin_sel_gpio_number);
			vin_state = (~vin_state) & 0x1;
			#endif
			if(!vin_state)
				mdelay(5);
		} while ((!vin_state) && check_times);
		battery_xlog_printk(BAT_LOG_CRTI, "[DISO] i==%d  gpio_state= %d\n",
			check_times, mt_get_gpio_in(vin_sel_gpio_number));

		if (0 == check_times)
			diso_state &= ~0x4; //SET DC bit as 0
		else
			diso_state |= 0x4; //SET DC bit as 1
	} else {
		diso_state &= ~0x4; //SET DC bit as 0
	}
	#else
	mdelay(SWITCH_RISING_TIMING + LOAD_SWITCH_TIMING_MARGIN); /* force delay for switching as no flag for check switching done */
	if (vol > VDC_MIN_VOLTAGE/1000 && vol < VDC_MAX_VOLTAGE/1000)
			diso_state |= 0x4; //SET DC bit as 1
	else
			diso_state &= ~0x4; //SET DC bit as 0
	#endif


	vol = diso_get_current_voltage(AP_AUXADC_DISO_VUSB_CHANNEL);
	vol =(R_DISO_VBUS_PULL_UP + R_DISO_VBUS_PULL_DOWN)*100*vol/(R_DISO_VBUS_PULL_DOWN)/100;
	battery_xlog_printk(BAT_LOG_CRTI, "[DISO]  Current VBUS voltage  mV = %d\n",vol);

	if (vol > VBUS_MIN_VOLTAGE/1000 && vol < VBUS_MAX_VOLTAGE/1000) {
		if(!mt_usb_is_device())	{
			diso_state |= 0x1; //SET OTG bit as 1
			diso_state &= ~0x2; //SET VBUS bit as 0
		} else {
			diso_state &= ~0x1; //SET OTG bit as 0
			diso_state |= 0x2; //SET VBUS bit as 1;
		}

	} else {
		diso_state &= 0x4; //SET OTG and VBUS bit as 0
	}
	battery_xlog_printk(BAT_LOG_CRTI, "[DISO] DISO_STATE==0x%x \n",diso_state);
	g_diso_state = diso_state;
	return;
}
#if !defined(MTK_AUXADC_IRQ_SUPPORT)
int _get_irq_direction(int pre_vol, int cur_vol)
{
	int ret = -1;

	//threshold 1000mv
	if((cur_vol - pre_vol) > 1000)
		ret = DISO_IRQ_RISING;
	else if((pre_vol - cur_vol) > 1000)	
		ret = DISO_IRQ_FALLING;

	return ret;
}

static void _get_polling_state(void)
{
	int vdc_vol = 0, vusb_vol = 0;
	int vdc_vol_dir = -1;
	int vusb_vol_dir = -1;

	DISO_polling_channel* VDC_Polling = &DISO_Polling.vdc_polling_measure;
	DISO_polling_channel* VUSB_Polling = &DISO_Polling.vusb_polling_measure;

	vdc_vol = diso_get_current_voltage(AP_AUXADC_DISO_VDC_CHANNEL);
	vdc_vol =(R_DISO_DC_PULL_UP + R_DISO_DC_PULL_DOWN)*100*vdc_vol/(R_DISO_DC_PULL_DOWN)/100;

	vusb_vol = diso_get_current_voltage(AP_AUXADC_DISO_VUSB_CHANNEL);
	vusb_vol =(R_DISO_VBUS_PULL_UP + R_DISO_VBUS_PULL_DOWN)*100*vusb_vol/(R_DISO_VBUS_PULL_DOWN)/100;

	VDC_Polling->preVoltage = VDC_Polling->curVoltage;
	VUSB_Polling->preVoltage = VUSB_Polling->curVoltage;
	VDC_Polling->curVoltage = vdc_vol;
	VUSB_Polling->curVoltage = vusb_vol;

	if (DISO_Polling.reset_polling)
	{
		DISO_Polling.reset_polling = KAL_FALSE;
		VDC_Polling->preVoltage = vdc_vol;
		VUSB_Polling->preVoltage = vusb_vol;

		if(vdc_vol > 1000)
			vdc_vol_dir = DISO_IRQ_RISING;
		else
			vdc_vol_dir = DISO_IRQ_FALLING;

		if(vusb_vol > 1000)
			vusb_vol_dir = DISO_IRQ_RISING;
		else
			vusb_vol_dir = DISO_IRQ_FALLING;
	}
	else
	{
		//get voltage direction
		vdc_vol_dir = _get_irq_direction(VDC_Polling->preVoltage, VDC_Polling->curVoltage);
		vusb_vol_dir = _get_irq_direction(VUSB_Polling->preVoltage, VUSB_Polling->curVoltage);
	}

	if(VDC_Polling->notify_irq_en && 
		(vdc_vol_dir == VDC_Polling->notify_irq)) {
		schedule_delayed_work(&diso_polling_work, 10*HZ/1000); //10ms
		battery_xlog_printk(BAT_LOG_CRTI, "[%s] ready to trig VDC irq, irq: %d\n",
			__func__,VDC_Polling->notify_irq);
	} else if(VUSB_Polling->notify_irq_en && 
		(vusb_vol_dir == VUSB_Polling->notify_irq)) {
		schedule_delayed_work(&diso_polling_work, 10*HZ/1000);
		battery_xlog_printk(BAT_LOG_CRTI, "[%s] ready to trig VUSB irq, irq: %d\n",
			__func__, VUSB_Polling->notify_irq);
	} else if((vdc_vol == 0) && (vusb_vol == 0)) {
		VDC_Polling->notify_irq_en = 0;
		VUSB_Polling->notify_irq_en = 0;
	}
		
	return;
}

enum hrtimer_restart diso_kthread_hrtimer_func(struct hrtimer *timer)
{
	diso_thread_timeout = KAL_TRUE;
	wake_up(&diso_polling_thread_wq);

	return HRTIMER_NORESTART;
}

int diso_thread_kthread(void *x)
{
    /* Run on a process content */
    while (1) {
		wait_event(diso_polling_thread_wq, (diso_thread_timeout == KAL_TRUE));

		diso_thread_timeout = KAL_FALSE;

		mutex_lock(&diso_polling_mutex);

		_get_polling_state();

		if (DISO_Polling.vdc_polling_measure.notify_irq_en ||
			DISO_Polling.vusb_polling_measure.notify_irq_en)
			hrtimer_start(&diso_kthread_timer,ktime_set(0, MSEC_TO_NSEC(SW_POLLING_PERIOD)),HRTIMER_MODE_REL);
		else
			hrtimer_cancel(&diso_kthread_timer);

		mutex_unlock(&diso_polling_mutex);
	}

	return 0;
}
#endif
#endif

static kal_uint32 charging_diso_init(void *data)
{
	kal_uint32 status = STATUS_OK;

#if defined(MTK_DUAL_INPUT_CHARGER_SUPPORT)
	DISO_ChargerStruct *pDISO_data = (DISO_ChargerStruct *)data;

	/* Initialization DISO Struct */
	pDISO_data->diso_state.cur_otg_state	= DISO_OFFLINE;
	pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
	pDISO_data->diso_state.cur_vdc_state	= DISO_OFFLINE;

	pDISO_data->diso_state.pre_otg_state	= DISO_OFFLINE;
	pDISO_data->diso_state.pre_vusb_state = DISO_OFFLINE;
	pDISO_data->diso_state.pre_vdc_state	= DISO_OFFLINE;

	pDISO_data->chr_get_diso_state = KAL_FALSE;
	pDISO_data->hv_voltage = VBUS_MAX_VOLTAGE;

	#if !defined(MTK_AUXADC_IRQ_SUPPORT)
	hrtimer_init(&diso_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	diso_kthread_timer.function = diso_kthread_hrtimer_func;
	INIT_DELAYED_WORK(&diso_polling_work, diso_polling_handler);

	kthread_run(diso_thread_kthread, NULL, "diso_thread_kthread");
	battery_xlog_printk(BAT_LOG_CRTI, "[%s] done\n", __func__);
	#else
	struct device_node *node;
	int ret;

	//Initial AuxADC IRQ
	DISO_IRQ.vdc_measure_channel.number   = AP_AUXADC_DISO_VDC_CHANNEL;
	DISO_IRQ.vusb_measure_channel.number  = AP_AUXADC_DISO_VUSB_CHANNEL;
	DISO_IRQ.vdc_measure_channel.period   = AUXADC_CHANNEL_DELAY_PERIOD;
	DISO_IRQ.vusb_measure_channel.period  = AUXADC_CHANNEL_DELAY_PERIOD;
	DISO_IRQ.vdc_measure_channel.debounce	 = AUXADC_CHANNEL_DEBOUNCE;
	DISO_IRQ.vusb_measure_channel.debounce  = AUXADC_CHANNEL_DEBOUNCE;

	/* use default threshold voltage, if use high voltage,maybe refine*/
	DISO_IRQ.vusb_measure_channel.falling_threshold  = VBUS_MIN_VOLTAGE/1000;
	DISO_IRQ.vdc_measure_channel.falling_threshold   = VDC_MIN_VOLTAGE/1000;
	DISO_IRQ.vusb_measure_channel.rising_threshold  = VBUS_MIN_VOLTAGE/1000;
	DISO_IRQ.vdc_measure_channel.rising_threshold	 = VDC_MIN_VOLTAGE/1000;

	node = of_find_compatible_node(NULL, NULL, "mediatek,AUXADC");
	if (!node) {
		battery_xlog_printk(BAT_LOG_CRTI, "[diso_adc]: of_find_compatible_node failed!!\n");
	} else {
		pDISO_data->irq_line_number = irq_of_parse_and_map(node, 0);
		battery_xlog_printk(BAT_LOG_CRTI, "[diso_adc]: IRQ Number: 0x%x\n", pDISO_data->irq_line_number);
	}

	mt_irq_set_sens(pDISO_data->irq_line_number, MT_EDGE_SENSITIVE);
	mt_irq_set_polarity(pDISO_data->irq_line_number, MT_POLARITY_LOW);

	ret = request_threaded_irq(pDISO_data->irq_line_number, diso_auxadc_irq_handler, \
		pDISO_data->irq_callback_func, IRQF_ONESHOT  , "DISO_ADC_IRQ", NULL);

	if (ret) {
		battery_xlog_printk(BAT_LOG_CRTI, "[diso_adc]: request_irq failed.\n");
	} else {
		set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
		set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
		battery_xlog_printk(BAT_LOG_CRTI, "[diso_adc]: diso_init success.\n");
	}
	#endif

	#if defined(MTK_DISCRETE_SWITCH) && defined(MTK_DSC_USE_EINT)
	battery_xlog_printk(BAT_LOG_CRTI, "[diso_eint]vdc eint irq registitation\n");
	mt_eint_set_hw_debounce(CUST_EINT_VDC_NUM, CUST_EINT_VDC_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_VDC_NUM, CUST_EINTF_TRIGGER_LOW, vdc_eint_handler, 0);
	mt_eint_mask(CUST_EINT_VDC_NUM); 
	#endif
#endif

	return status;	
}

static kal_uint32 charging_get_diso_state(void *data)
{
	kal_uint32 status = STATUS_OK;

#if defined(MTK_DUAL_INPUT_CHARGER_SUPPORT)
	int diso_state = 0x0;
	DISO_ChargerStruct *pDISO_data = (DISO_ChargerStruct *)data;

	_get_diso_interrupt_state();
	diso_state = g_diso_state;
	battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] current diso state is %s!\n", DISO_state_s[diso_state]);
	if(((diso_state >> 1) & 0x3) != 0x0)
	{
		switch (diso_state){
			case USB_ONLY:
				set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
				set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
				#ifdef MTK_DISCRETE_SWITCH
				#ifdef MTK_DSC_USE_EINT
				mt_eint_unmask(CUST_EINT_VDC_NUM); 
				#else
				set_vdc_auxadc_irq(DISO_IRQ_ENABLE, 1);
				#endif
				#endif
				pDISO_data->diso_state.cur_vusb_state  = DISO_ONLINE;
				pDISO_data->diso_state.cur_vdc_state	= DISO_OFFLINE;
				pDISO_data->diso_state.cur_otg_state	= DISO_OFFLINE;
				break;
			case DC_ONLY:
				set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
				set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
				set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_RISING);
				pDISO_data->diso_state.cur_vusb_state  = DISO_OFFLINE;
				pDISO_data->diso_state.cur_vdc_state	= DISO_ONLINE;
				pDISO_data->diso_state.cur_otg_state	= DISO_OFFLINE;
				break;
			case DC_WITH_USB:
				set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
				set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
				set_vusb_auxadc_irq(DISO_IRQ_ENABLE,DISO_IRQ_FALLING);
				pDISO_data->diso_state.cur_vusb_state  = DISO_ONLINE;
				pDISO_data->diso_state.cur_vdc_state	= DISO_ONLINE;
				pDISO_data->diso_state.cur_otg_state	= DISO_OFFLINE;
				break;
			case DC_WITH_OTG:
				set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
				set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
				pDISO_data->diso_state.cur_vusb_state  = DISO_OFFLINE;
				pDISO_data->diso_state.cur_vdc_state	= DISO_ONLINE;
				pDISO_data->diso_state.cur_otg_state	= DISO_ONLINE;
				break;
			default: // OTG only also can trigger vcdt IRQ
				pDISO_data->diso_state.cur_vusb_state  = DISO_OFFLINE;
				pDISO_data->diso_state.cur_vdc_state	= DISO_OFFLINE;
				pDISO_data->diso_state.cur_otg_state	= DISO_ONLINE;
				battery_xlog_printk(BAT_LOG_CRTI, " switch load vcdt irq triggerd by OTG Boost!\n");
				break; // OTG plugin no need battery sync action
		}
	}

	if (DISO_ONLINE == pDISO_data->diso_state.cur_vdc_state)
		pDISO_data->hv_voltage = VDC_MAX_VOLTAGE;
	else
		pDISO_data->hv_voltage = VBUS_MAX_VOLTAGE;
#endif

	return status;
}


static kal_uint32 charging_get_error_state(void)
{
	return charging_error;
}

static kal_uint32 charging_set_error_state(void *data)
{
	kal_uint32 status = STATUS_OK;
	charging_error = *(kal_uint32*)(data);

	return status;
}

static kal_uint32 (* const charging_func[CHARGING_CMD_NUMBER])(void *data)=
{
	charging_hw_init
	,charging_dump_register  	
	,charging_enable
	,charging_set_cv_voltage
	,charging_get_current
	,charging_set_current
	,charging_set_input_current
	,charging_get_charging_status
	,charging_reset_watch_dog_timer
	,charging_set_hv_threshold
	,charging_get_hv_status
	,charging_get_battery_status
	,charging_get_charger_det_status
	,charging_get_charger_type
	,charging_get_is_pcm_timer_trigger
	,charging_set_platform_reset
	,charging_get_platfrom_boot_mode
	,charging_set_power_off
	,charging_get_power_source
	,charging_get_csdac_full_flag
	,charging_set_ta_current_pattern
	,charging_diso_init
	,charging_get_diso_state
	,charging_set_error_state
};
 
/*
* FUNCTION
*		Internal_chr_control_handler
*
* DESCRIPTION															 
*		 This function is called to set the charger hw
*
* CALLS  
*
* PARAMETERS
*		None
*	 
* RETURNS
*		
*
* GLOBALS AFFECTED
*	   None
*/
kal_int32 chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
{
	kal_int32 status;
	if(cmd < CHARGING_CMD_NUMBER)
		status = charging_func[cmd](data);
	else
		return STATUS_UNSUPPORTED;

	return status;
}
