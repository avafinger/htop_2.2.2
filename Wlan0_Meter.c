/*
htop - Wlan0_Meter.c
(C) 2020 @lex
*/

#include "Wlan0_Meter.h"
#include "Platform.h"
#include "CRT.h"

/*{
#include "Meter.h"
}*/


int Wlan0_Meter_attributes[] = {
   WLAN0_INTERFACE
};

static void Wlan0_Meter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    static char szIP[48];
    static unsigned int use_cached = 0;
    Settings* settings = this->pl->settings;
    
    if ((use_cached++ % 2)) {
        xSnprintf(buffer, len, "%s", szIP);
        return;
    }
    
    if (settings->wlan0_alias[0] != 0) {
        ret = findIP_interface(settings->wlan0_alias, szIP, sizeof(szIP));
    } else {
        ret = findIP_interface("wlan0", szIP, sizeof(szIP));
    }
    
    if (ret) {
        xSnprintf(buffer, len, "%s", szIP);
    } else {
        xSnprintf(buffer, len, "%s", "down");
    }
}

MeterClass Wlan0_Meter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Wlan0_Meter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Wlan0_Meter_attributes,
   .name = "Wlan0",
   .uiName = "Wlan0 IP",
   .caption = "Wlan0 IP: ",
};


