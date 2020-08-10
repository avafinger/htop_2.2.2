/*
htop - Eth1_Meter.c
(C) 2018 @lex
*/

#include "Eth1_Meter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int Eth1_Meter_attributes[] = {
   ETH1_INTERFACE
};

static void Eth1_Meter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    static char szIP[48];
    static unsigned int use_cached = 0;
    Settings* settings = this->pl->settings;
    
    if ((use_cached++ % 2)) {
        xSnprintf(buffer, len, "%s", szIP);
        return;
    }
    
    if (settings->eth1_alias[0] != 0) {
        ret = findIP_interface(settings->eth1_alias, szIP, sizeof(szIP));
    } else {
        ret = findIP_interface("eth1", szIP, sizeof(szIP));
    }
    
    if (ret) {
        xSnprintf(buffer, len, "%s", szIP);
    } else {
        xSnprintf(buffer, len, "%s", "down");
    }
}

MeterClass Eth1_Meter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Eth1_Meter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Eth1_Meter_attributes,
   .name = "Eth1",
   .uiName = "Eth1 IP",
   .caption = " Eth1 IP: ",
};


