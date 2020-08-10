/*
htop - Eth0_Meter.c
(C) 2018 @lex
*/

#include "Wlan1_Meter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int Wlan1_Meter_attributes[] = {
   WLAN1_INTERFACE
};

static void Wlan1_Meter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    static char szIP[48];
    static unsigned int use_cached = 0;
    Settings* settings = this->pl->settings;
    
    if ((use_cached++ % 2)) {
        xSnprintf(buffer, len, "%s", szIP);
        return;
    }
    
    if (settings->wlan1_alias[0] != 0) {
        ret = findIP_interface(settings->wlan1_alias, szIP, sizeof(szIP));
    } else {
        ret = findIP_interface("wlan1", szIP, sizeof(szIP));
    }
    
    if (ret) {
        xSnprintf(buffer, len, "%s", szIP);
    } else {
        xSnprintf(buffer, len, "%s", "down");
    }
}

MeterClass Wlan1_Meter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Wlan1_Meter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Wlan1_Meter_attributes,
   .name = "Wlan1",
   .uiName = "Wlan1 IP",
   .caption = "Wlan1 IP: ",
};


