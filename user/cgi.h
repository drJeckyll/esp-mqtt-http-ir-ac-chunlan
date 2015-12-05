#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int cgiLed(HttpdConnData *connData);
int saveCGI(HttpdConnData *connData);
int settingsCGI(HttpdConnData *connData);
//int loadTpl(HttpdConnData *connData, char *token, void **arg);
//void tplLed(HttpdConnData *connData, char *token, void **arg);
//void tplDHT(HttpdConnData *connData, char *token, void **arg);
//int cgiReadFlash(HttpdConnData *connData);
//void tplCounter(HttpdConnData *connData, char *token, void **arg);

#endif