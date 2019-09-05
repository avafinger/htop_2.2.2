/*
htop - CpuVcoreMeter.c
(C) 2018 @lex
*/

#include "CpuVcoreMeter.h"
#include "Platform.h"
#include "CRT.h"

/*{
#include "Meter.h"
}*/


int CpuVcoreMeter_attributes[] = {
   CPU_VCORE
};

static void CpuVcoreMeter_setValues(Meter* this, char* buffer, int len) {
   int v1,v2;
   int Vcore, Vcore_l, Vcore_b;
   char buf1[80], buf2[80];
   int h;
   
   if (Platform_getCpuBigLITTLE()) {
       h = len / 2;
       if (h > 79)
           h = 79;
       Vcore_b = Platform_getCpuVcore_b();
       if (Vcore_b > 1000) {
           Vcore_b /= 1000;
       }
       if (Vcore_b >= 1000) {
           Vcore_b /= 10;
           v1 = Vcore_b / 100;
           v2 = Vcore_b % 100;
           xSnprintf(buf1, h, "%d.%02d V ", v1, v2);
       } else {
           xSnprintf(buf1, h, "%4d mV", Vcore_b);
       }
       Vcore_l = Platform_getCpuVcore_l();
       if (Vcore_l > 1000) {
           Vcore_l /= 1000;
       }
       if (Vcore_l >= 1000) {
           Vcore_l /= 10;
           v1 = Vcore_l / 100;
           v2 = Vcore_l % 100;
           xSnprintf(buf2, h, "%d.%02d V ", v1, v2);
       } else {
           xSnprintf(buf2, h, "%4d mV", Vcore_l);
       }
       xSnprintf(buffer, len, "%s  %s  (big.LITTLE)", buf1, buf2);
       return;
   }
   
   Vcore = Platform_getCpuVcore();
   if (Vcore > 1000) {
       Vcore /= 1000;
   }
   if (Vcore >= 1000) {
       Vcore /= 10;
       v1 = Vcore / 100;
       v2 = Vcore % 100;
       xSnprintf(buffer, len, "%d.%02d V", v1, v2);
   } else {
       xSnprintf(buffer, len, "%4d mV", Vcore);
   }
}

MeterClass CpuVcoreMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = CpuVcoreMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 1,
   .total = 100.0,
   .attributes = CpuVcoreMeter_attributes,
   .name = "CpuVcore",
   .uiName = "CpuVcore",
   .caption = "Cpu Vcor: "
};
