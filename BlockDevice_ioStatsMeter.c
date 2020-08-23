/*
htop - BlockDevice_ioStatsMeter.c
(C) 2020 @lex
*/

#include "BlockDevice_ioStatsMeter.h"
#include "Platform.h"
#include "CRT.h"

#include "interfaces.h"

/*{
#include "Meter.h"
}*/


int BlockDevice_ioStatsMeter_attributes[] = {
   ETH1_INTERFACE
};

static void BlockDevice_sda_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "sda");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("sda", 0, 0);
    if (ret) {
        if (Platform_BlockDevice_sda_stats.read_sectors_comp > Platform_BlockDevice_sda_stats.read_sectors)
            Platform_BlockDevice_sda_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_sda_stats.write_sectors_comp > Platform_BlockDevice_sda_stats.write_sectors)
            Platform_BlockDevice_sda_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_sda_stats.read_sectors - Platform_BlockDevice_sda_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_sda_stats.write_sectors - Platform_BlockDevice_sda_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_sda_stats.read_sectors_comp = Platform_BlockDevice_sda_stats.read_sectors;
        Platform_BlockDevice_sda_stats.write_sectors_comp = Platform_BlockDevice_sda_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_sda_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_sda_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "sdastat",
   .uiName = "sda speed stat",
   .caption = "sda stat: ",
};

static void BlockDevice_sdb_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "sdb");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("sdb", 1, 0);
    if (ret) {
        if (Platform_BlockDevice_sdb_stats.read_sectors_comp > Platform_BlockDevice_sdb_stats.read_sectors)
            Platform_BlockDevice_sdb_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_sdb_stats.write_sectors_comp > Platform_BlockDevice_sdb_stats.write_sectors)
            Platform_BlockDevice_sdb_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_sdb_stats.read_sectors - Platform_BlockDevice_sdb_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_sdb_stats.write_sectors - Platform_BlockDevice_sdb_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_sdb_stats.read_sectors_comp = Platform_BlockDevice_sdb_stats.read_sectors;
        Platform_BlockDevice_sdb_stats.write_sectors_comp = Platform_BlockDevice_sdb_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_sdb_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_sdb_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "sdbstat",
   .uiName = "sdb speed stat",
   .caption = "sdb stat: ",
};

static void BlockDevice_sdc_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "sdc");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("sdc", 2, 0);
    if (ret) {
        if (Platform_BlockDevice_sdc_stats.read_sectors_comp > Platform_BlockDevice_sdc_stats.read_sectors)
            Platform_BlockDevice_sdc_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_sdc_stats.write_sectors_comp > Platform_BlockDevice_sdc_stats.write_sectors)
            Platform_BlockDevice_sdc_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_sdc_stats.read_sectors - Platform_BlockDevice_sdc_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_sdc_stats.write_sectors - Platform_BlockDevice_sdc_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_sdc_stats.read_sectors_comp = Platform_BlockDevice_sdc_stats.read_sectors;
        Platform_BlockDevice_sdc_stats.write_sectors_comp = Platform_BlockDevice_sdc_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_sdc_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_sdc_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "sdcstat",
   .uiName = "sdc speed stat",
   .caption = "sdc stat: ",
};

static void BlockDevice_sdd_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "sdd");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("sdd", 3, 0);
    if (ret) {
        if (Platform_BlockDevice_sdd_stats.read_sectors_comp > Platform_BlockDevice_sdd_stats.read_sectors)
            Platform_BlockDevice_sdd_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_sdd_stats.write_sectors_comp > Platform_BlockDevice_sdd_stats.write_sectors)
            Platform_BlockDevice_sdd_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_sdd_stats.read_sectors - Platform_BlockDevice_sdd_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_sdd_stats.write_sectors - Platform_BlockDevice_sdd_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_sdd_stats.read_sectors_comp = Platform_BlockDevice_sdd_stats.read_sectors;
        Platform_BlockDevice_sdd_stats.write_sectors_comp = Platform_BlockDevice_sdd_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_sdd_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_sdd_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "sddstat",
   .uiName = "sdd speed stat",
   .caption = "sdd stat: ",
};

static void BlockDevice_mmcblk0_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "mmcblk0");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("mmcblk0", 4, 0);
    if (ret) {
        if (Platform_BlockDevice_mmcblk0_stats.read_sectors_comp > Platform_BlockDevice_mmcblk0_stats.read_sectors)
            Platform_BlockDevice_mmcblk0_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_mmcblk0_stats.write_sectors_comp > Platform_BlockDevice_mmcblk0_stats.write_sectors)
            Platform_BlockDevice_mmcblk0_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_mmcblk0_stats.read_sectors - Platform_BlockDevice_mmcblk0_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_mmcblk0_stats.write_sectors - Platform_BlockDevice_mmcblk0_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_mmcblk0_stats.read_sectors_comp = Platform_BlockDevice_mmcblk0_stats.read_sectors;
        Platform_BlockDevice_mmcblk0_stats.write_sectors_comp = Platform_BlockDevice_mmcblk0_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_mmcblk0_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_mmcblk0_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "mmcblk0stat",
   .uiName = "mmcblk0 speed stat",
   .caption = "mmcblk0 stat: ",
};

static void BlockDevice_mmcblk1_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "mmcblk1");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("mmcblk1", 5, 0);
    if (ret) {
        if (Platform_BlockDevice_mmcblk1_stats.read_sectors_comp > Platform_BlockDevice_mmcblk1_stats.read_sectors)
            Platform_BlockDevice_mmcblk1_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_mmcblk1_stats.write_sectors_comp > Platform_BlockDevice_mmcblk1_stats.write_sectors)
            Platform_BlockDevice_mmcblk1_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_mmcblk1_stats.read_sectors - Platform_BlockDevice_mmcblk1_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_mmcblk1_stats.write_sectors - Platform_BlockDevice_mmcblk1_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_mmcblk1_stats.read_sectors_comp = Platform_BlockDevice_mmcblk1_stats.read_sectors;
        Platform_BlockDevice_mmcblk1_stats.write_sectors_comp = Platform_BlockDevice_mmcblk1_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_mmcblk1_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_mmcblk1_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "mmcblk1stat",
   .uiName = "mmcblk1 speed stat",
   .caption = "mmcblk1 stat: ",
};

static void BlockDevice_mmcblk2_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "mmcblk2");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("mmcblk2", 6, 0);
    if (ret) {
        if (Platform_BlockDevice_mmcblk2_stats.read_sectors_comp > Platform_BlockDevice_mmcblk2_stats.read_sectors)
            Platform_BlockDevice_mmcblk2_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_mmcblk2_stats.write_sectors_comp > Platform_BlockDevice_mmcblk2_stats.write_sectors)
            Platform_BlockDevice_mmcblk2_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_mmcblk2_stats.read_sectors - Platform_BlockDevice_mmcblk2_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_mmcblk2_stats.write_sectors - Platform_BlockDevice_mmcblk2_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_mmcblk2_stats.read_sectors_comp = Platform_BlockDevice_mmcblk2_stats.read_sectors;
        Platform_BlockDevice_mmcblk2_stats.write_sectors_comp = Platform_BlockDevice_mmcblk2_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_mmcblk2_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_mmcblk2_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "mmcblk2stat",
   .uiName = "mmcblk2 speed stat",
   .caption = "mmcblk2 stat: ",
};

static void BlockDevice_mmcblk3_ioStatsMeter_setValues(Meter* this, char* buffer, int len) {
    int ret;
    float readspeed;
    float writespeed;
    double refreshdelay;
    char block_device[80];
    static double old = 0.;
    static double now = 0.;
    static int flash = 0;
    static unsigned long sec_size = 0;
    FILE *fp;

    if (sec_size == 0) {
        xSnprintf(block_device, sizeof(block_device), "/sys/block/%s/queue/hw_sector_size", "mmcblk3");
        if ((fp = fopen(block_device, "r")) == NULL) {
            xSnprintf(buffer, len, "%s",  "unavailable");
            return;
        }
        if (fgets(block_device, 79, fp) != NULL) {
            sscanf(block_device, "%lu", &sec_size);
        }
        fclose(fp);
        if (sec_size == 0)
            sec_size = 1;
    }

    now = get_wall_time();
    refreshdelay = now - old;
    if (old == 0.)
       refreshdelay = 1.;
    old = now;    

    ret = Platform_getIO_stats("mmcblk3", 7, 0);
    if (ret) {
        if (Platform_BlockDevice_mmcblk3_stats.read_sectors_comp > Platform_BlockDevice_mmcblk3_stats.read_sectors)
            Platform_BlockDevice_mmcblk3_stats.read_sectors_comp = 0;
        if (Platform_BlockDevice_mmcblk3_stats.write_sectors_comp > Platform_BlockDevice_mmcblk3_stats.write_sectors)
            Platform_BlockDevice_mmcblk3_stats.write_sectors_comp = 0;

        /* we assume here the refresdelay is 1 sec which sometimes is 1.5, so we have a peak */ 
        readspeed = (Platform_BlockDevice_mmcblk3_stats.read_sectors - Platform_BlockDevice_mmcblk3_stats.read_sectors_comp) / refreshdelay;
        writespeed = (Platform_BlockDevice_mmcblk3_stats.write_sectors - Platform_BlockDevice_mmcblk3_stats.write_sectors_comp) / refreshdelay;

        writespeed = writespeed * sec_size / 1000;
        readspeed = readspeed * sec_size / 1000;
        if (writespeed >= 1000 || readspeed >= 1000) {
            writespeed /= 1000;
            readspeed /= 1000;
            if (writespeed >= 1000 || readspeed >= 1000) {
                writespeed /= 1000;
                readspeed /= 1000;
                xSnprintf(buffer, len, "%.2f GB/s - %.2f GB/s (R/W)",  (float) readspeed, (float) writespeed);
            } else {
                xSnprintf(buffer, len, "%.2f MB/s - %.2f MB/s (R/W)",  (float) readspeed, (float) writespeed);
            }
        } else {
            xSnprintf(buffer, len, "%.2f KB/s - %.2f KB/s (R/W)",  (float) readspeed, (float) writespeed);
        }

        Platform_BlockDevice_mmcblk3_stats.read_sectors_comp = Platform_BlockDevice_mmcblk3_stats.read_sectors;
        Platform_BlockDevice_mmcblk3_stats.write_sectors_comp = Platform_BlockDevice_mmcblk3_stats.write_sectors;

    } else {
        if (!(flash % 2))
            xSnprintf(buffer, len, "%s", "unavailable");
        else
            xSnprintf(buffer, len, "%s", "");
        flash++;
    }
}

MeterClass BlockDevice_mmcblk3_ioStatsMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = BlockDevice_mmcblk3_ioStatsMeter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = BlockDevice_ioStatsMeter_attributes,
   .name = "mmcblk3stat",
   .uiName = "mmcblk3 speed stat",
   .caption = "mmcblk3 stat: ",
};




