/*
htop - CpuFreqMeter.c
(C) 2020 Alexander Finger
*/

#include "CpuFreqMeter.h"
#include "Platform.h"
#include "CRT.h"

/*{
#include "Meter.h"
}*/


int CpuFreqMeter_attributes[] = {
   CPU_FREQ
};

static void CpuFreqMeter_setValues(Meter* this, char* buffer, int len) {
   int ghz,mhz,roundup;
   int cpu, Freq;
   int bigLITTLE;
   char buf_b[32];
   char buf_l[32];
   int ln = sizeof(buf_l);
   static int pulse = 0;
   
   memset(buf_l, 0, sizeof(buf_l));
   memset(buf_b, 0, sizeof(buf_b));
   
   bigLITTLE = Platform_getCpuBigLITTLE();
   if (bigLITTLE)  {
       cpu = bigLITTLE;
       if (cpu < 0)
           cpu = 0;
       Freq = Platform_getCpuFreq(this, cpu);
       if (Freq > 1000) {
           Freq /= 1000;
       }
       if (Freq > 1000) {
           ghz = Freq / 1000;
           mhz = Freq % 1000;
           roundup = ((mhz % 10) > 5);
           mhz /= 10;
           mhz += roundup;
           xSnprintf(buf_b, ln, "%d.%02d GHz", ghz, mhz);
       } else {
           xSnprintf(buf_b, ln, "%4d MHz", Freq);
       }
       
       cpu--;
       if (cpu < 0)
           cpu = 0;
       Freq = Platform_getCpuFreq(this, cpu);
       if (Freq > 1000) {
           Freq /= 1000;
       }
       if (Freq > 1000) {
           ghz = Freq / 1000;
           mhz = Freq % 1000;
           roundup = ((mhz % 10) > 5);
           mhz /= 10;
           mhz += roundup;
           xSnprintf(buf_l, ln, "%d.%02d GHz", ghz, mhz);
       } else {
           xSnprintf(buf_l, ln, "%4d MHz", Freq);
       }
       
       xSnprintf(buffer, len, "%s %s (%d)", buf_b, buf_l, pulse);
       pulse = !pulse;
       return;
   }
   
   cpu = 0;
   Freq = Platform_getCpuFreq(this, cpu);
   if (Freq > 1000) {
       Freq /= 1000;
   }
   if (Freq > 1000) {
       ghz = Freq / 1000;
       mhz = Freq % 1000;
       roundup = ((mhz % 10) > 5);
       mhz /= 10;
       mhz += roundup;
       xSnprintf(buffer, len, "%d.%02d GHz", ghz, mhz);
   } else {
       xSnprintf(buffer, len, "%4d MHz", Freq);
   }
   pulse = !pulse;
}

MeterClass CpuFreqMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = CpuFreqMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 1,
   .total = 100.0,
   .attributes = CpuFreqMeter_attributes,
   .name = "CpuFreq",
   .uiName = "CpuFreq",
   .caption = "Cpu Freq: ",
};

