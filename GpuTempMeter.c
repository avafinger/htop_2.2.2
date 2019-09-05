/*
htop - GpuTempMeter.c
(C) 2018 @lex
*/

#include "GpuTempMeter.h"
#include "Platform.h"
#include "CRT.h"

/*{
#include "Meter.h"
}*/

int GpuTempMeter_attributes[] = {
   GPU_TEMP
};

static void GpuTempMeter_setValues(Meter* this, char* buffer, int len) {
   int Temp = Platform_getGpuTemp();
   if (Temp > 1000) {
       Temp /= 1000;
   }
   xSnprintf(buffer, len, "%4d C", Temp);
}

MeterClass GpuTempMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = GpuTempMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 1,
   .total = 100.0,
   .attributes = GpuTempMeter_attributes,
   .name = "GpuTemp",
   .uiName = "GpuTemp",
   .caption = "Gpu Temp: "
};
