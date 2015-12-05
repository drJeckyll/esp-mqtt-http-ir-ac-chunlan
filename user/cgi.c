/*
Some random cgi routines.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include "dht22.h"
#include "espmissingincludes.h"
#include "debug.h"
#include "config.h"

static char ac_cmd[100];

//cause I can't be bothered to write an ioGetLed()
//static char currLedState=0;

/*void ICACHE_FLASH_ATTR to_bin(int value, int bitsCount, char* output) {
	int i;

	output[bitsCount] = '\0';
	for (i = bitsCount - 1; i >= 0; --i, value >>= 1)
	{
		output[i] = (value & 1) + '0';
	}
}*/

// CGI to set settings
int ICACHE_FLASH_ATTR saveCGI(HttpdConnData *connData) {
	int len;
	char buff[10];
	//char bin[10];

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len = httpdFindArg(connData->getArgs, "power", buff, sizeof(buff));
	if (len != 0) ac_set_power(buff);

	len = httpdFindArg(connData->getArgs, "mode", buff, sizeof(buff));
	if (len != 0) ac_set_mode(buff);

	len = httpdFindArg(connData->getArgs, "temp", buff, sizeof(buff));
	if (len != 0) ac_set_temp(buff);

	len = httpdFindArg(connData->getArgs, "fan", buff, sizeof(buff));
	if (len != 0) ac_set_fan(buff);

	len = httpdFindArg(connData->getArgs, "swing", buff, sizeof(buff));
	if (len != 0) ac_set_swing(buff);

	ir_send();
	mqttPublishSettings(0);
	CFG_Save();

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/html");
	httpdEndHeaders(connData);

	os_strcpy(buff, "OK");
	httpdSend(connData, buff, -1);

	return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR settingsCGI(HttpdConnData *connData) {
	char buff[256];
	float * r = readDHT();
	float lastTemp = r[0];
	float lastHum = r[1];

	os_sprintf(buff, 
		"{\"power\":\"%s\",\"mode\":\"%s\",\"temp\":\"%d\",\"fan\":\"%s\",\"swing\":\"%s\",\"dht_temp\":\"%d\",\"dht_humid\":\"%d\"}",
		sysCfg.power,
		sysCfg.mode,
		sysCfg.temp,
		sysCfg.fan,
		sysCfg.swing,
		(int)(lastTemp * 100), 
		(int)(lastHum * 100)
	);

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/html");
	httpdEndHeaders(connData);

	httpdSend(connData, buff, -1);

	return HTTPD_CGI_DONE;
}

// CGI to load settings
/*int ICACHE_FLASH_ATTR loadTpl(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];

	if (token == NULL) return ;

	os_strcpy(buff, "");
	if (os_strcmp(token, "power") == 0) {
		os_sprintf(buff, sysCfg.power);
	}
	if (os_strcmp(token, "mode") == 0) {
		os_sprintf(buff, sysCfg.mode);
	}
	if (os_strcmp(token, "temp") == 0) {
		os_sprintf(buff, "%i", sysCfg.temp);
	}
	if (os_strcmp(token, "fan") == 0) {
		os_sprintf(buff, sysCfg.fan);
	}
	if (os_strcmp(token, "swing") == 0) {
		os_sprintf(buff, sysCfg.swing);
	}
	if (os_strcmp(token, "DHT") == 0) {
		float * r = readDHT();
		float lastTemp = r[0];
		float lastHum = r[1];

		os_sprintf(buff, "\"dht_temp\":\"%d\",\"dht_humid\":\"%d\"", (int)(lastTemp * 100), (int)(lastHum * 100));
	}

	httpdSend(connData, buff, -1);
}*/

//Cgi that turns the LED on or off according to the 'led' param in the GET data
/*int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->getArgs, "led", buff, sizeof(buff));
	if (len!=0) {
		currLedState=atoi(buff);
		ioLed(currLedState);
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}*/

//Template code for the led page.
/*void ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
		if (currLedState) {
			os_strcpy(buff, "on");
		} else {
			os_strcpy(buff, "off");
		}
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}*/

/*static long hitCounter=0;
//Template code for the counter on the index page.
void ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	if (os_strcmp(token, "counter")==0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}*/

//Cgi that reads the SPI flash. Assumes 512KByte flash.
/*int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnData *connData) {
	int *pos=(int *)&connData->cgiData;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	if (*pos==0) {
		os_printf("Start flash download.\n");
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "application/bin");
		httpdEndHeaders(connData);
		*pos=0x40200000;
		return HTTPD_CGI_MORE;
	}
	espconn_sent(connData->conn, (uint8 *)(*pos), 1024);
	*pos+=1024;
	if (*pos>=0x40200000+(512*1024)) return HTTPD_CGI_DONE; else return HTTPD_CGI_MORE;
}*/

//Template code for the DHT 22 page.
/*void ICACHE_FLASH_ATTR tplDHT(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	float * r = readDHT();
	float lastTemp=r[0];
	float lastHum=r[1];
	
	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "temperature")==0) {
			os_sprintf(buff, "%d.%d", (int)(lastTemp),(int)((lastTemp - (int)lastTemp)*100) );		
	}
	if (os_strcmp(token, "humidity")==0) {
			os_sprintf(buff, "%d.%d", (int)(lastHum),(int)((lastHum - (int)lastHum)*100) );		
	}	
	
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}*/
