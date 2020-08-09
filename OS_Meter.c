/*
htop - OSversion_Meter.c
(C) 2018 @lex
*/

#include "OS_Meter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int OSversion_Meter_attributes[] = {
   OS_VERSION
};

static void OSversion_Meter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    static char version[256];
    static int use_cached = 0;
    
    if ((use_cached++ % 10)) {
        xSnprintf(buffer, len, "%s", version);
        return;
    }
    
    ret = ReadKeyValue( "/etc/os-release", "PRETTY_NAME=", version);
    
    if (ret) {
        xSnprintf(buffer, len, "%s", version);
    } else {
        xSnprintf(buffer, len, "%s", "down");
    }
}

MeterClass OSversion_Meter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = OSversion_Meter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = OSversion_Meter_attributes,
   .name = "OSversion",
   .uiName = "OS version",
   .caption = " OS vers: ",
};


