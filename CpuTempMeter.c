/*
htop - CpuTempMeter.c
(C) 2018 @lex
*/

#include "CpuTempMeter.h"
#include "Platform.h"
#include "CRT.h"

/*{
#include "Meter.h"
}*/

int CpuTempMeter_attributes[] = {
   CPU_TEMP
};

static void CpuTempMeter_setValues(Meter* this, char* buffer, int len) {
   int Temp = Platform_getCpuTemp();
   if (Temp > 1000) {
       Temp /= 1000;
   }
   xSnprintf(buffer, len, "%4d C", Temp);
}

MeterClass CpuTempMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = CpuTempMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 1,
   .total = 100.0,
   .attributes = CpuTempMeter_attributes,
   .name = "CpuTemp",
   .uiName = "CpuTemp",
   .caption = "Cpu Temp: "
};
