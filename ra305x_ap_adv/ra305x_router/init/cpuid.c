#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_if.h>
#include <cyg/hal/plf_io.h>
#include <cpuid.h>
#include <gpio.h>
#include <boards.h>

int cpu_info(void)
{
	unsigned long int  n0,n1,id;
	int cpu_type;
	char soc_id[SYS_TYPE_LEN] = {0};

	n0 = HAL_REG32(RT305X_SYSC_BASE+SYSC_REG_CHIP_NAME0);
	n1 = HAL_REG32(RT305X_SYSC_BASE+SYSC_REG_CHIP_NAME1);
	id = HAL_REG32(RT305X_SYSC_BASE+SYSC_REG_CHIP_ID);

	if (n0 == RT3052_CHIP_NAME0 && n1 == RT3052_CHIP_NAME1) {
			board->cpu_name = "Ralink RT3052";
	} else if (n0 == RT3350_CHIP_NAME0 && n1 == RT3350_CHIP_NAME1) {
		board->cpu_name = "Ralink RT3350";
		board->cpu_freq	= 320000000; 
		cpu_type=SOC_RT3052;
	} else if (n0 == RT3352_CHIP_NAME0 && n1 == RT3352_CHIP_NAME1) {
		board->cpu_name = "Ralink RT3352";
		board->cpu_freq	= 400000000; 
		cpu_type=SOC_RT3352;
	} else if (n0 == RT5350_CHIP_NAME0 && n1 == RT5350_CHIP_NAME1) {
		board->cpu_name = "Ralink RT5350";
		board->cpu_freq	= 360000000; 
		cpu_type=SOC_RT5350;
	} else if (n0 == RT3883_CHIP_NAME0 && n1 == RT3883_CHIP_NAME1) {
		board->cpu_name = "Ralink RT3883";
		board->cpu_freq	= 500000000; 
		cpu_type=SOC_RT3883;
	} else if (n0 == MT7620_CHIP_NAME0 && n1 == MT7620_CHIP_NAME1) {
		board->cpu_name = "MediaTek MT7620";
		board->cpu_freq	= 580000000; 
		cpu_type=SOC_MT7620;
	} else if (n0 == MT7628_CHIP_NAME0 && n1 == MT7628_CHIP_NAME1) {
		board->cpu_name = "MediaTek MT7628";
		board->cpu_freq	= 580000000; 
		cpu_type=SOC_MT7628;
	} else {
		diag_printf("unsupport SoC, n0:%08x n1:%08x\n", n0, n1);
		snprintf(soc_id, SYS_TYPE_LEN,
		"Ralink %c%c%c%c%c%c%c%c id:%u rev:%u",
		(char) (n0 & 0xff), (char) ((n0 >> 8) & 0xff),
		(char) ((n0 >> 16) & 0xff), (char) ((n0 >> 24) & 0xff),
		(char) (n1 & 0xff), (char) ((n1 >> 8) & 0xff),
		(char) ((n1 >> 16) & 0xff), (char) ((n1 >> 24) & 0xff),
		(id >> CHIP_ID_ID_SHIFT) & CHIP_ID_ID_MASK,
		(id & CHIP_ID_REV_MASK));
		diag_printf("%s \n",soc_id);
		cpu_type=SOC_UNKNOWN;
	}

#if 0
	//board->cpu_name=name;
	diag_printf("%s id:%u rev:%u running at %d Mhz\n",
		board->cpu_name,
		(id >> CHIP_ID_ID_SHIFT) & CHIP_ID_ID_MASK,
		(id & CHIP_ID_REV_MASK),board->cpu_freq/1000/1000);
#endif

	return cpu_type;
}
