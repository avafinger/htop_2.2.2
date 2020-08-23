/*
htop - Wlan0_StatsMeter.c
(C) 2020 @lex
*/

#include "Wlan0_StatsMeter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int Wlan0_StatsMeter_attributes[] = {
   ETH0_INTERFACE
};

/* borrowed from slurm */
static void Wlan0_StatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float rxspeed;
    float txspeed;
    Settings* settings = this->pl->settings;
    double refreshdelay;
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;

    if (settings->wlan0_alias[0] != 0) {
        ret = Platform_getEth_stats(settings->wlan0_alias, 2, 0);
    } else {
        ret = Platform_getEth_stats("wlan0", 0, 0);
    }
    
    if (ret) {
        if (Platform_Wlan0_stats.rx_bytes_comp > Platform_Wlan0_stats.rx_bytes)
            Platform_Wlan0_stats.rx_bytes_comp = 0;
        if (Platform_Wlan0_stats.tx_bytes_comp > Platform_Wlan0_stats.tx_bytes)
            Platform_Wlan0_stats.tx_bytes_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        rxspeed = (Platform_Wlan0_stats.rx_bytes - Platform_Wlan0_stats.rx_bytes_comp) / refreshdelay;
        txspeed = (Platform_Wlan0_stats.tx_bytes - Platform_Wlan0_stats.tx_bytes_comp) / refreshdelay;

        xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (TX/RX)", (float) txspeed / 1024, (float) rxspeed / 1024);

        Platform_Wlan0_stats.rx_bytes_comp = Platform_Wlan0_stats.rx_bytes;
        Platform_Wlan0_stats.tx_bytes_comp = Platform_Wlan0_stats.tx_bytes;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavail");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass Wlan0_StatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Wlan0_StatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Wlan0_StatsMeter_attributes,
   .name = "Wlan0stat",
   .uiName = "Wlan0 stat",
   .caption = "Wlan0 stat: ",
};


