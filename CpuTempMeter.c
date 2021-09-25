/*
htop - CpuTempMeter.c
(C) 2020 @lex
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
   int p1;
   int p2;
   int Temp = Platform_getCpuTemp(this);
   if (Temp > 1000) {
       p1 =  Temp / 1000;
       p2 = (Temp % 1000) / 10;
   }  else {
      if (Temp > 100) {
          p1 = Temp / 100;
          p2 = (Temp % 100) / 10;
      } else {
          p1 = Temp;
          p2 = 0;
      }
   }
   // note: UTF8 string will not be displayd in ASCII cli
   xSnprintf(buffer, len, "%2d.%02d °C", p1, p2);
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
   .caption = "CPU Temp: "
};
