
#include "ac.h"
#include <string.h>
#include <osapi.h>
#include "c_types.h"
#include "gpio.h"
#include "debug.h"
#include "config.h"
#include "dht22.h"

static char ac_cmd[100];

void ICACHE_FLASH_ATTR to_bin(int value, int bitsCount, char *output) {
	int i;

	output[bitsCount] = '\0';
	for (i = bitsCount - 1; i >= 0; --i, value >>= 1)
	{
		output[i] = (value & 1) + '0';
	}
}

void ICACHE_FLASH_ATTR ac_init() {
	os_strcpy(ac_cmd, AC_CMD);
}

void ICACHE_FLASH_ATTR ac_get_settings(char *settings, int temp_humid) {
	if (temp_humid) {
		float * r = readDHT();
		float lastTemp = r[0];
		float lastHum = r[1];

		os_sprintf(
			settings, 
			"{\"power\":\"%s\",\"mode\":\"%s\",\"temp\":\"%d\",\"fan\":\"%s\",\"swing\":\"%s\",\"dht_temp\":\"%d\",\"dht_humid\":\"%d\"}", 
			sysCfg.power, 
			sysCfg.mode, 
			sysCfg.temp, 
			sysCfg.fan, 
			sysCfg.swing,
			(int)(lastTemp * 100), 
			(int)(lastHum * 100)
		);
	} else
	{
		os_sprintf(
			settings, 
			"{\"power\":\"%s\",\"mode\":\"%s\",\"temp\":\"%d\",\"fan\":\"%s\",\"swing\":\"%s\"}", 
			sysCfg.power, 
			sysCfg.mode, 
			sysCfg.temp, 
			sysCfg.fan, 
			sysCfg.swing
		);
	}
}

void ICACHE_FLASH_ATTR ac_set_power(char *power) {
	if (os_strcmp(power, "on") == 0) {
		ac_cmd[3] = '1';
		os_strcpy(sysCfg.power, "on");
	} else 
	if (os_strcmp(power, "off") == 0) {
		ac_cmd[3] = '0';
		os_strcpy(sysCfg.power, "off");
	}
}

void ICACHE_FLASH_ATTR ac_set_mode(char *mode) {
	if (os_strcmp(mode, "heat") == 0) {
		ac_cmd[0] = '1'; ac_cmd[1] = '1';
		os_strcpy(sysCfg.mode, "heat");
	} else 
	if (os_strcmp(mode, "cool") == 0) {
		ac_cmd[0] = '0'; ac_cmd[1] = '1';
		os_strcpy(sysCfg.mode, "cool");
	}
}

void ICACHE_FLASH_ATTR ac_set_temp(char *temp) {
	char bin[10];
	int tmp = atoi(temp);
	if (tmp < 15) tmp = 15;
	if (tmp > 30) tmp = 30;
	to_bin(tmp + 9, 6, bin);
	int c = 8, i;
	for (i = 5; i >= 0; i--) {
		ac_cmd[c] = bin[i];
		c++;
	}
	sysCfg.temp = tmp;
}

void ICACHE_FLASH_ATTR ac_set_fan(char *fan) {
	if (os_strcmp(fan, "1") == 0) {
		ac_cmd[16] = '1'; ac_cmd[17] = '1';
		os_strcpy(sysCfg.fan, "1");
	} else
	if (os_strcmp(fan, "2") == 0) {
		ac_cmd[16] = '0'; ac_cmd[17] = '1';
		os_strcpy(sysCfg.fan, "2");
	} else
	if (os_strcmp(fan, "3") == 0) {
		ac_cmd[16] = '1'; ac_cmd[17] = '0';
		os_strcpy(sysCfg.fan, "3");
	} else 
	if (os_strcmp(fan, "auto") == 0) {
		ac_cmd[16] = '0'; ac_cmd[17] = '0';
		os_strcpy(sysCfg.fan, "auto");
	}
}

void ICACHE_FLASH_ATTR ac_set_swing(char *swing) {
	if (os_strcmp(swing, "on") == 0) {
		ac_cmd[19] = '1';
		os_strcpy(sysCfg.swing, "on");
	} else 
	if (os_strcmp(swing, "off") == 0) {
		ac_cmd[19] = '0';
		os_strcpy(sysCfg.swing, "off");
	}
}

int in_ir_send = 0;
void ICACHE_FLASH_ATTR ir_send()
{
	int t, c;

	int lead_pulse = 8380 / (1000000 / IR_FREQ);
	int lead_space = 4500;

	int pulse_pulse = 500 / (1000000 / IR_FREQ);
	int pulse_space = 1750;

	int space_pulse = 500 / (1000000 / IR_FREQ);
	int space_space = 638;

	int end = 500 / (1000000 / IR_FREQ);
	
	int cycle = ((1000000 / IR_FREQ) / 2);

	if (in_ir_send) return ;

	in_ir_send = 1;

	ets_wdt_disable();
	os_intr_lock();

	// lead
	for (t = 0; t < lead_pulse; t++)
	{
		GPIO_OUTPUT_SET(IR_PIN, 1);
		os_delay_us(cycle);
		GPIO_OUTPUT_SET(IR_PIN, 0);
		os_delay_us(cycle);
	}
	GPIO_OUTPUT_SET(IR_PIN, 0);
	os_delay_us(lead_space);

	// info
	for (c = 0; c < 72; c++)
	{
		if (ac_cmd[c] == '0') {
			for (t = 0; t < space_pulse; t++)
			{
				GPIO_OUTPUT_SET(IR_PIN, 1);
				os_delay_us(cycle);
				GPIO_OUTPUT_SET(IR_PIN, 0);
				os_delay_us(cycle);
			}
			GPIO_OUTPUT_SET(IR_PIN, 0);
			os_delay_us(space_space);
		}
		if (ac_cmd[c] == '1') {
			for (t = 0; t < pulse_pulse; t++)
			{
				GPIO_OUTPUT_SET(IR_PIN, 1);
				os_delay_us(cycle);
				GPIO_OUTPUT_SET(IR_PIN, 0);
				os_delay_us(cycle);
			}
			GPIO_OUTPUT_SET(IR_PIN, 0);
			os_delay_us(pulse_space);
		}
	}

	// end
	for (t = 0; t < end; t++)
	{
		GPIO_OUTPUT_SET(IR_PIN, 1);
		os_delay_us(cycle);
		GPIO_OUTPUT_SET(IR_PIN, 0);
		os_delay_us(cycle);
	}
	GPIO_OUTPUT_SET(IR_PIN, 0);

	os_intr_unlock();
	ets_wdt_enable();

	in_ir_send = 0;

	INFO("IR SEND: %s\r\n", ac_cmd);
}
