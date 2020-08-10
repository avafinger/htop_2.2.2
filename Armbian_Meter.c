/*
htop - Armbianversion_Meter.c
(C) 2018 @lex
*/

#include "Armbian_Meter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int Armbianversion_Meter_attributes[] = {
   ARMBIAN_VERSION
};

static void Armbianversion_Meter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    static char version[256];
    static unsigned int use_cached = 0;
    
    if ((use_cached++ % 10)) {
        xSnprintf(buffer, len, "%s", version);
        return;
    }
    
    ret = ReadKeyValue( "/etc/armbian-release", "VERSION=", version);
    
    if (ret) {
        xSnprintf(buffer, len, "%s", version);
    } else {
        xSnprintf(buffer, len, "%s", "unknown");
    }
}

MeterClass Armbianversion_Meter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Armbianversion_Meter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Armbianversion_Meter_attributes,
   .name = "Armbianversion",
   .uiName = "Armbian version",
   .caption = "Armbian : ",
};


