/*
htop - Kernelversion_Meter.c
(C) 2018 @lex
*/

#include "Kernel_Meter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int Kernelversion_Meter_attributes[] = {
   KERNEL_VERSION
};

static void Kernelversion_Meter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    static char version[256];
    static int use_cached = 0;
    
    if ((use_cached++ % 10)) {
        xSnprintf(buffer, len, "%s", version);
        return;
    }
    
    ret = ReadTokenValue( "/proc/version", "Linux version", version);
    
    if (ret) {
        xSnprintf(buffer, len, "%s", version);
    } else {
        xSnprintf(buffer, len, "%s", "down");
    }
}

MeterClass Kernelversion_Meter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Kernelversion_Meter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Kernelversion_Meter_attributes,
   .name = "Kernelversion",
   .uiName = "Kernel version",
   .caption = "  Kernel: ",
};


