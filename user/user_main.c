/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"

#include "ac.h"

#include "httpd.h"
#include "io.h"
#include "dht22.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "auth.h"

#define IR_TOPIC "/office/service/ac"

int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, MQTT_USER);
		os_strcpy(pass, MQTT_PASS);
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}

HttpdBuiltInUrl builtInUrls[]={
	{"/*", authBasic, myPassFn},
	{"/", cgiRedirect, "/index.html"},
	{"/save.cgi", saveCGI, NULL},
	{"/settings.cgi", settingsCGI, NULL},
	//{"/load.tpl", cgiEspFsTemplate, loadTpl},
	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

MQTT_Client mqttClient;

void ICACHE_FLASH_ATTR wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	}
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
	//MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");

	MQTT_Subscribe(&mqttClient, IR_TOPIC);
}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
	//MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void ICACHE_FLASH_ATTR mqttPublishedCb(uint32_t *args)
{
	//MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}

void ICACHE_FLASH_ATTR mqttPublishSettings(temp_humid) {
	char buff[100] = "";
	char topic[100];
	
	os_sprintf(topic, "%s/%s", IR_TOPIC, "settings");
	ac_get_settings(buff, temp_humid);
	MQTT_Publish(&mqttClient, topic, buff, os_strlen(buff), 0, 0);
	INFO("MQTT send topic: %s, data: %s \r\n", topic, buff);
}

void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char topicBuf[64], dataBuf[100];

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("MQTT topic: %s, data: %s \r\n", topicBuf, dataBuf);

	// send settings
	if (os_strcmp(topicBuf, IR_TOPIC) == 0)
	{
		if (os_strcmp(dataBuf, "settings") == 0) {
			mqttPublishSettings(1);
		} else {
			// try to parse and set
			// shift 1 byte left
			char *token, *endtoken;
			int t, apply = 0;
			char cmd[20], val[20];
			for (t = 1; t < os_strlen(dataBuf); t++) dataBuf[t - 1] = dataBuf[t];
			dataBuf[os_strlen(dataBuf) - 2] = '\0';
			token = strtok_r(dataBuf, ",", &endtoken);
			while (token != NULL) {
				int i = 0;
				char *tmp, *endtmp;
				//INFO("> %s\n", token);
				tmp = strtok_r(token, ":", &endtmp);
				while (tmp != NULL) {
					//INFO(">> %s\n", tmp);
					// trim "
					for (t = 1; t < os_strlen(tmp); t++) tmp[t - 1] = tmp[t];
					tmp[os_strlen(tmp) - 2] = '\0';
					if (i == 0) os_strcpy(cmd, tmp);
					if (i == 1) os_strcpy(val, tmp);

					tmp = strtok_r(NULL, ":", &endtmp);
					i++;
				}
				//INFO(">>> %s: %s\n", cmd, val);
				
				if (os_strcmp(cmd, "power") == 0) {
					apply = 1;
					ac_set_power(val);	
				} else
				if (os_strcmp(cmd, "mode") == 0) {
					apply = 1;
					ac_set_mode(val);
				} else
				if (os_strcmp(cmd, "temp") == 0) {
					apply = 1;
					ac_set_temp(val);
				} else
				if (os_strcmp(cmd, "fan") == 0) {
					apply = 1;
					ac_set_fan(val);
				} else
				if (os_strcmp(cmd, "swing") == 0) {
					apply = 1;
					ac_set_swing(val);
				}

				token = strtok_r(NULL, ",", &endtoken);
			}
			if (apply) {
				ir_send();
				mqttPublishSettings(0);
				CFG_Save();
			}
		}
	}
}

void user_init(void)
{
	gpio_init();

	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	CFG_Load();

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	INFO("\r\nSystem started ...\r\n");

	// ir
	ac_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	gpio_output_set(0, BIT4, BIT4, 0);
	// ir

	// web
	stdoutInit();
	ioInit();
	DHTInit();
	httpdInit(builtInUrls, 80);
	os_printf("\nReady\n");
}
