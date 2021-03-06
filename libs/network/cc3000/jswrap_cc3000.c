/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * Contains built-in functions for CC3000 WiFi Access
 * EXTREMELY BETA AND LIKELY TO CHANGE DRASTICALLY
 * ----------------------------------------------------------------------------
 */

#include "jswrap_cc3000.h"
#include "jshardware.h"
#include "jsinteractive.h"
#include "board_spi.h"
// ti driver
#include "wlan.h"
#include "netapp.h"
#include "hci.h"


/*JSON{ "type":"library",
        "class" : "CC3000",
        "description" : ""
}*/
/*JSON{ "type":"staticmethod", 
         "class" : "CC3000", "name" : "connect",
         "generate" : "jswrap_cc3000_connect",
         "description" : "Initialise the CC3000 and return a WLAN object",
         "params" : [ ],
         "return" : ["JsVar", "A WLAN Object"]
}*/
JsVar *jswrap_cc3000_connect() {
  JsVar *wlanObj = jspNewObject(jsiGetParser(), 0, "WLAN");
  cc3000_initialise(wlanObj);
  return wlanObj;
}


/*JSON{ "type":"class",
        "class" : "WLAN",
        "description" : "An instantiation of a WiFi network adaptor"
}*/

/*JSON{ "type":"method",
         "class" : "WLAN", "name" : "connect",
         "generate" : "jswrap_wlan_connect",
         "description" : "Connect to a wireless network",
         "params" : [ [ "ap", "JsVar", "Access point name" ],
                      [ "key", "JsVar", "WPA2 key (or undefined for unsecured connection)" ],
                      [ "callback", "JsVar", "Function to call back with connection status. It has one argument which is one of 'connect'/'disconnect'/'dhcp'" ] ],
         "return" : ["int", ""]
}*/
JsVarInt jswrap_wlan_connect(JsVar *wlanObj, JsVar *vAP, JsVar *vKey, JsVar *callback) {
  if (!(jsvIsUndefined(callback) || jsvIsFunction(callback))) {
    jsError("Expecting callback function");
    return 0;
  }
  if (jsvIsFunction(callback)) {
    jsvObjectSetChild(wlanObj, CC3000_ON_STATE_CHANGE, callback);
  }
  char ap[32];
  char key[32];
  unsigned long security = WLAN_SEC_UNSEC;
  jsvGetString(vAP, ap, sizeof(ap));
  if (jsvIsString(vKey)) {
    security = WLAN_SEC_WPA2;
    jsvGetString(vKey, key, sizeof(key));
  }
  // might want to set wlan_ioctl_set_connection_policy
  return wlan_connect(security, ap, strlen(ap), NULL, key, strlen(key));
}


void _wlan_getIP_get_address(JsVar *object, const char *name,  unsigned char *ip, int nBytes, int base, char separator) {
  char data[64] = "";
  int i, l = 0;
  for (i=nBytes-1;i>=0;i--) {
    itoa(ip[i], &data[l], base);
    l = strlen(data);
    if (i>0 && separator) {
      data[l++] = separator;
      data[l] = 0;
    }
  }

  JsVar *dataVar = jsvNewFromString(data);
  if (!dataVar) return;

  JsVar *v = jsvFindChildFromString(object, name, true);
  if (!v) {
    jsvUnLock(dataVar);
    return; // out of memory
  }
  jsvSetValueOfName(v, dataVar);
  jsvUnLock(dataVar);
  jsvUnLock(v);
}

/*JSON{ "type":"method",
         "class" : "WLAN", "name" : "getIP",
         "generate" : "jswrap_wlan_getIP",
         "description" : "Get the current IP address",
         "return" : ["JsVar", ""]
}*/
JsVar *jswrap_wlan_getIP(JsVar *wlanObj) {
  tNetappIpconfigRetArgs ipconfig;
  netapp_ipconfig(&ipconfig);
  /* If byte 1 is 0 we don't have a valid address */
  if (ipconfig.aucIP[3] == 0) return 0;
  JsVar *data = jsvNewWithFlags(JSV_OBJECT);
  _wlan_getIP_get_address(data, "ip", &ipconfig.aucIP, 4, 10, '.');
  _wlan_getIP_get_address(data, "subnet", &ipconfig.aucSubnetMask, 4, 10, '.');
  _wlan_getIP_get_address(data, "gateway", &ipconfig.aucDefaultGateway, 4, 10, '.');
  _wlan_getIP_get_address(data, "dhcp", &ipconfig.aucDHCPServer, 4, 10, '.');
  _wlan_getIP_get_address(data, "dns", &ipconfig.aucDNSServer, 4, 10, '.');
  _wlan_getIP_get_address(data, "mac", &ipconfig.uaMacAddr, 6, 16, 0);
  return data;
}
