/*
htop - Eth0_StatsMeter.c
(C) 2020 @lex
*/

#include "Eth0_StatsMeter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int Eth0_StatsMeter_attributes[] = {
   ETH0_INTERFACE
};

/* borrowed from slurm */
static void Eth0_StatsMeter_setValues(Meter* this, char* buffer, int len) {
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

    if (settings->eth0_alias[0] != 0) {
        ret = Platform_getEth_stats(settings->eth0_alias, 0, 0);
    } else {
        ret = Platform_getEth_stats("eth0", 0, 0);
    }
    
    if (ret) {
        if (Platform_Eth0_stats.rx_bytes_comp > Platform_Eth0_stats.rx_bytes)
            Platform_Eth0_stats.rx_bytes_comp = 0;
        if (Platform_Eth0_stats.tx_bytes_comp > Platform_Eth0_stats.tx_bytes)
            Platform_Eth0_stats.tx_bytes_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        rxspeed = (Platform_Eth0_stats.rx_bytes - Platform_Eth0_stats.rx_bytes_comp) / refreshdelay;
        txspeed = (Platform_Eth0_stats.tx_bytes - Platform_Eth0_stats.tx_bytes_comp) / refreshdelay;

        txspeed = (float) txspeed / 1000.;
        rxspeed = (float) rxspeed / 1000.;
        if (rxspeed < 1000. && txspeed < 1000.) {
	   xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (RX/TX)", (float) rxspeed, (float) txspeed);
	} else	 {
	    txspeed = (float) txspeed / 1000.;
	    rxspeed = (float) rxspeed / 1000.;
	    if (rxspeed < 1000. && txspeed < 1000.) {
		xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (RX/TX)", (float) rxspeed, (float) txspeed);
	    } else   {
		xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (RX/TX)", (float) rxspeed, (float) txspeed);
	    }
	}
        Platform_Eth0_stats.rx_bytes_comp = Platform_Eth0_stats.rx_bytes;
        Platform_Eth0_stats.tx_bytes_comp = Platform_Eth0_stats.tx_bytes;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavail");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass Eth0_StatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Eth0_StatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Eth0_StatsMeter_attributes,
   .name = "Eth0stat",
   .uiName = "Eth0 stat",
   .caption = " Eth0 stat: ",
};


