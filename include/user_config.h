#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define CFG_HOLDER	0x00FF55A2
#define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */

/*DEFAULT CONFIGURATIONS*/

#define MQTT_HOST			"1.2.3.4" //or "192.168.11.1"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		120	 /*second*/

#define MQTT_CLIENT_ID		"DVES_%08X"
#define MQTT_USER			"user"
#define MQTT_PASS			"pass"

#define AC_POWER			"off"
#define AC_MODE				"heat"
#define AC_TEMP				18
#define AC_FAN				"3"
#define AC_SWING			"off"

#define STA_SSID "ssid"
#define STA_PASS "pass"
//#define STA_TYPE AUTH_WPA2_PSK
#define STA_TYPE AUTH_OPEN

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

//#define CLIENT_SSL_ENABLE


#endif
