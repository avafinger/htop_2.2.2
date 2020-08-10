/*
htop - linux/Platform.c
(C) 2014 Hisham H. Muhammad
(C) 2020 Alexander Finger
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "Platform.h"
#include "IOPriority.h"
#include "IOPriorityPanel.h"
#include "LinuxProcess.h"
#include "LinuxProcessList.h"
#include "Battery.h"

#include "Meter.h"
#include "CPUMeter.h"
#include "CpuFreqMeter.h"
#include "CpuTempMeter.h"
#include "GpuTempMeter.h"
#include "CpuVcoreMeter.h"
#include "Eth0_Meter.h"
#include "Eth1_Meter.h"
#include "Wlan0_Meter.h"
#include "Wlan1_Meter.h"
#include "Armbian_Meter.h"
#include "OS_Meter.h"
#include "Kernel_Meter.h"
#include "MemoryMeter.h"
#include "SwapMeter.h"
#include "TasksMeter.h"
#include "LoadAverageMeter.h"
#include "UptimeMeter.h"
#include "ClockMeter.h"
#include "HostnameMeter.h"
#include "LinuxProcess.h"
#include "Settings.h"

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   /* for nanosleep */
#else
#include <unistd.h> /* for usleep */
#endif

#include <math.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/*{
#include "Action.h"
#include "MainPanel.h"
#include "BatteryMeter.h"
#include "LinuxProcess.h"
#include "SignalsPanel.h"
#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   
#else
#include <unistd.h> 
#endif
#include <string.h> 
}*/

#ifndef CLAMP
#define CLAMP(x,low,high) (((x)>(high))?(high):(((x)<(low))?(low):(x)))
#endif

ProcessField Platform_defaultFields[] = { PID, USER, PRIORITY, NICE, M_SIZE, M_RESIDENT, M_SHARE, STATE, PERCENT_CPU, PERCENT_MEM, TIME, COMM, 0 };

//static ProcessField defaultIoFields[] = { PID, IO_PRIORITY, USER, IO_READ_RATE, IO_WRITE_RATE, IO_RATE, COMM, 0 };

int Platform_cpuBigLITTLE;
int Platform_numberOfFields = LAST_PROCESSFIELD;

const SignalItem Platform_signals[] = {
   { .name = " 0 Cancel",    .number = 0 },
   { .name = " 1 SIGHUP",    .number = 1 },
   { .name = " 2 SIGINT",    .number = 2 },
   { .name = " 3 SIGQUIT",   .number = 3 },
   { .name = " 4 SIGILL",    .number = 4 },
   { .name = " 5 SIGTRAP",   .number = 5 },
   { .name = " 6 SIGABRT",   .number = 6 },
   { .name = " 6 SIGIOT",    .number = 6 },
   { .name = " 7 SIGBUS",    .number = 7 },
   { .name = " 8 SIGFPE",    .number = 8 },
   { .name = " 9 SIGKILL",   .number = 9 },
   { .name = "10 SIGUSR1",   .number = 10 },
   { .name = "11 SIGSEGV",   .number = 11 },
   { .name = "12 SIGUSR2",   .number = 12 },
   { .name = "13 SIGPIPE",   .number = 13 },
   { .name = "14 SIGALRM",   .number = 14 },
   { .name = "15 SIGTERM",   .number = 15 },
   { .name = "16 SIGSTKFLT", .number = 16 },
   { .name = "17 SIGCHLD",   .number = 17 },
   { .name = "18 SIGCONT",   .number = 18 },
   { .name = "19 SIGSTOP",   .number = 19 },
   { .name = "20 SIGTSTP",   .number = 20 },
   { .name = "21 SIGTTIN",   .number = 21 },
   { .name = "22 SIGTTOU",   .number = 22 },
   { .name = "23 SIGURG",    .number = 23 },
   { .name = "24 SIGXCPU",   .number = 24 },
   { .name = "25 SIGXFSZ",   .number = 25 },
   { .name = "26 SIGVTALRM", .number = 26 },
   { .name = "27 SIGPROF",   .number = 27 },
   { .name = "28 SIGWINCH",  .number = 28 },
   { .name = "29 SIGIO",     .number = 29 },
   { .name = "29 SIGPOLL",   .number = 29 },
   { .name = "30 SIGPWR",    .number = 30 },
   { .name = "31 SIGSYS",    .number = 31 },
};

const unsigned int Platform_numberOfSignals = sizeof(Platform_signals)/sizeof(SignalItem);

static Htop_Reaction Platform_actionSetIOPriority(State* st) {
   Panel* panel = st->panel;

   LinuxProcess* p = (LinuxProcess*) Panel_getSelected(panel);
   if (!p) return HTOP_OK;
   IOPriority ioprio = p->ioPriority;
   Panel* ioprioPanel = IOPriorityPanel_new(ioprio);
   void* set = Action_pickFromVector(st, ioprioPanel, 21);
   if (set) {
      IOPriority ioprio = IOPriorityPanel_getIOPriority(ioprioPanel);
      bool ok = MainPanel_foreachProcess((MainPanel*)panel, (MainPanel_ForeachProcessFn) LinuxProcess_setIOPriority, (Arg){ .i = ioprio }, NULL);
      if (!ok)
         beep();
   }
   Panel_delete((Object*)ioprioPanel);
   return HTOP_REFRESH | HTOP_REDRAW_BAR | HTOP_UPDATE_PANELHDR;
}

void Platform_setBindings(Htop_Action* keys) {
   keys['i'] = Platform_actionSetIOPriority;
}

MeterClass* Platform_meterTypes[] = {
   &CPUMeter_class,
   &ClockMeter_class,
   &LoadAverageMeter_class,
   &LoadMeter_class,
   &MemoryMeter_class,
   &SwapMeter_class,
   &TasksMeter_class,
   &UptimeMeter_class,
   &BatteryMeter_class,
   &HostnameMeter_class,
   &AllCPUsMeter_class,
   &AllCPUs2Meter_class,
   &LeftCPUsMeter_class,
   &RightCPUsMeter_class,
   &LeftCPUs2Meter_class,
   &RightCPUs2Meter_class,
   &BlankMeter_class,
   &CpuTempMeter_class,
   &CpuFreqMeter_class,
   /* --- fix me --- &AllCpuFreqMeter_class, */
   &CpuVcoreMeter_class,
   &GpuTempMeter_class,
   /* interfaces */
   &Eth0_Meter_class,
   &Eth1_Meter_class,
   &Wlan0_Meter_class,
   &Wlan1_Meter_class,
   &OSversion_Meter_class,
   &Kernelversion_Meter_class,
   &Armbianversion_Meter_class,   
   NULL
};

/* cross-platform sleep function */
void sleep_ms(int milliseconds) {
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}

int Platform_getGpuTemp(Meter* this) {
   int Temp = 0;
   char szbuf[256];
   Settings* settings = this->pl->settings;
   char *handler;
   char *cpu_core_policy;
   
   handler = settings->GpuTemp_handler;
   if (handler[0] != 0) {
       cpu_core_policy = strchr(handler, '%');
       if (cpu_core_policy) {
           strcpy(szbuf, "/sys/class/thermal/thermal_zone1/temp");
       } else {
           strcpy(szbuf, handler);
       }
   } else {
       strcpy(szbuf, "/sys/class/thermal/thermal_zone1/temp");
   }
   
   FILE* fd = fopen(szbuf, "r");
   if (!fd) {
       fd = fopen("/sys/devices/virtual/thermal/thermal_zone1/temp", "r");
   }
   if (fd) {
      int n = fscanf(fd, "%d", &Temp);
      fclose(fd);
      if (n <= 0) return 0;
   }
   return Temp;
}


int Platform_getCpuTemp(Meter* this) {
   int Temp = 0;
   char szbuf[256];
   Settings* settings = this->pl->settings;
   char *handler;
   char *cpu_core_policy;
   
   handler = settings->CpuTemp_handler;
   if (handler[0] != 0) {
       cpu_core_policy = strchr(handler, '%');
       if (cpu_core_policy) {
           strcpy(szbuf, "/sys/class/thermal/thermal_zone0/temp");
       } else {
           strcpy(szbuf, handler);
       }
   } else {
      // sleep_ms(30);
      // xSnprintf(szbuf, sizeof(szbuf), "/sys/devices/system/cpu/cpufreq/policy%d/cpuinfo_cur_freq", cpu);
      strcpy(szbuf, "/sys/class/thermal/thermal_zone0/temp");
   }
   FILE *fd = fopen(szbuf, "r");
   if (!fd) {
       fd = fopen("/sys/devices/virtual/thermal/thermal_zone0/temp", "r");
   }
   if (fd) {
      int n = fscanf(fd, "%d", &Temp);
      fclose(fd);
      if (n <= 0) return 0;
   }
   return Temp;
}

int Platform_getCpuFreq(Meter* this, int cpu) {
   int Freq = 0;
   FILE* fd;
   char szbuf[256];
   Settings* settings = this->pl->settings;
   char *handler;
   char *cpu_core_policy;
   
   handler = settings->CpuFreq_handler;
   if (handler[0] != 0) {
       cpu_core_policy = strchr(handler, '%');
       if (cpu_core_policy) {
           xSnprintf(szbuf, sizeof(szbuf), handler, cpu);
       } else {
           strcpy(szbuf, handler);
       }
   } else {
      // sleep_ms(30);
      // xSnprintf(szbuf, sizeof(szbuf), "/sys/devices/system/cpu/cpufreq/policy%d/cpuinfo_cur_freq", cpu);
      strcpy(szbuf, "/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq");
   }
   fd = fopen(szbuf, "r");
   if (fd) {
      int n;
      n = fscanf(fd, "%d", &Freq);
      fclose(fd);
      if (n <= 0) return 0;
   }
   return Freq;
}

int Platform_getCpuVcore() {
   int Vcore = 0;
   FILE* fd;
   // sleep_ms(10);
   fd = fopen("/sys/devices/platform/soc/7081400.i2c/i2c-0/0-0036/regulator/regulator.1/microvolts", "r");
   if (!fd) {
       fd = fopen("/sys/devices/platform/soc/1f02400.i2c/i2c-4/4-0065/regulator/regulator.5/microvolts", "r");
       if (!fd) {
           fd = fopen("/sys/devices/platform/ff3c0000.i2c/i2c-0/0-001b/regulator/regulator.12/microvolts", "r");
       }
   }
   if (fd) {
      int n;
      n = fscanf(fd, "%d", &Vcore);
      fclose(fd);
      if (n <= 0) return 0;
   }
   return Vcore;
}

int Platform_getCpuVcore_l(Meter* this) {
   int Vcore = 0;
   FILE* fd;
   char szbuf[256];
   Settings* settings = this->pl->settings;
   char *handler;
   char *cpu_core_policy;
   handler = settings->CpuVCore_l_handler;
   if (handler[0] != 0) {
       cpu_core_policy = strchr(handler, '%');
       if (cpu_core_policy) {
           strcpy(szbuf, "/sys/devices/platform/ff3c0000.i2c/i2c-0/0-001b/regulator/regulator.13/microvolts");
       } else {
           strcpy(szbuf, handler);
       }
   } else {
       strcpy(szbuf, "/sys/devices/platform/ff3c0000.i2c/i2c-0/0-001b/regulator/regulator.13/microvolts");
   }
   
   // sleep_ms(10);
   fd = fopen(szbuf, "r");
   if (fd) {
      int n;
      n = fscanf(fd, "%d", &Vcore);
      fclose(fd);
      if (n <= 0) return 0;
   }
   return Vcore;
}

int Platform_getCpuVcore_b(Meter* this) {
   int Vcore = 0;
   FILE* fd;
   char szbuf[256];
   Settings* settings = this->pl->settings;
   char *handler;
   char *cpu_core_policy;
   handler = settings->CpuVCore_b_handler;
   if (handler[0] != 0) {
       cpu_core_policy = strchr(handler, '%');
       if (cpu_core_policy) {
           strcpy(szbuf, "/sys/devices/platform/ff3c0000.i2c/i2c-0/0-0040/regulator/regulator.10/microvolts");
       } else {
           strcpy(szbuf, handler);
       }
   } else {
       strcpy(szbuf, "/sys/devices/platform/ff3c0000.i2c/i2c-0/0-0040/regulator/regulator.10/microvolts");
   }
   // sleep_ms(10);
   fd = fopen(szbuf, "r");   
   if (fd) {
      int n;
      n = fscanf(fd, "%d", &Vcore);
      fclose(fd);
      if (n <= 0) return 0;
   }
   return Vcore;
}


int Platform_getCpuBigLITTLE() {
    return Platform_cpuBigLITTLE;
}

int Platform_findCpuBigLITTLE(int cpucount, int *cpuBigLITTLE) {
    char buf[256];
    int n, prev, next;
    FILE* fd;
    
    *cpuBigLITTLE = 0;
    prev = next = -1;
    int cpu = 0;
    while (cpu < cpucount) {
        xSnprintf(buf, sizeof(buf), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu);
        fd = fopen(buf, "r");
        if (fd) {
            n = fscanf(fd, "%d", &next);
            fclose(fd);
            if (n <= 0) 
                break;
            if (prev == -1) {
                prev = next;
            } else {
                if (prev != next) {
                    if (prev < next) {
                        Platform_cpuBigLITTLE = cpu;
                    } else {
                        Platform_cpuBigLITTLE = cpu * -1; /* fix me */
                    }
                    *cpuBigLITTLE = Platform_cpuBigLITTLE;
                    break;                    
                }
            }
        }
        cpu++;
    }
    return cpu;
}

int Platform_getUptime() {
   double uptime = 0;
   FILE* fd = fopen(PROCDIR "/uptime", "r");
   if (fd) {
      int n = fscanf(fd, "%64lf", &uptime);
      fclose(fd);
      if (n <= 0) return 0;
   }
   return (int) floor(uptime);
}

void Platform_getLoadAverage(double* one, double* five, double* fifteen) {
   int activeProcs, totalProcs, lastProc;
   *one = 0; *five = 0; *fifteen = 0;
   FILE *fd = fopen(PROCDIR "/loadavg", "r");
   if (fd) {
      int total = fscanf(fd, "%32lf %32lf %32lf %32d/%32d %32d", one, five, fifteen,
         &activeProcs, &totalProcs, &lastProc);
      (void) total;
      assert(total == 6);
      fclose(fd);
   }
}

int Platform_getMaxPid() {
   FILE* file = fopen(PROCDIR "/sys/kernel/pid_max", "r");
   if (!file) return -1;
   int maxPid = 4194303;
   int match = fscanf(file, "%32d", &maxPid);
   (void) match;
   fclose(file);
   return maxPid;
}

double Platform_setCPUValues(Meter* this, int cpu) {
   LinuxProcessList* pl = (LinuxProcessList*) this->pl;
   CPUData* cpuData = &(pl->cpus[cpu]);
   double total = (double) ( cpuData->totalPeriod == 0 ? 1 : cpuData->totalPeriod);
   double percent;
   double* v = this->values;
   v[CPU_METER_NICE] = cpuData->nicePeriod / total * 100.0;
   v[CPU_METER_NORMAL] = cpuData->userPeriod / total * 100.0;
   if (this->pl->settings->detailedCPUTime) {
      v[CPU_METER_KERNEL]  = cpuData->systemPeriod / total * 100.0;
      v[CPU_METER_IRQ]     = cpuData->irqPeriod / total * 100.0;
      v[CPU_METER_SOFTIRQ] = cpuData->softIrqPeriod / total * 100.0;
      v[CPU_METER_STEAL]   = cpuData->stealPeriod / total * 100.0;
      v[CPU_METER_GUEST]   = cpuData->guestPeriod / total * 100.0;
      v[CPU_METER_IOWAIT]  = cpuData->ioWaitPeriod / total * 100.0;
      Meter_setItems(this, 8);
      if (this->pl->settings->accountGuestInCPUMeter) {
         percent = v[0]+v[1]+v[2]+v[3]+v[4]+v[5]+v[6];
      } else {
         percent = v[0]+v[1]+v[2]+v[3]+v[4];
      }
   } else {
      v[2] = cpuData->systemAllPeriod / total * 100.0;
      v[3] = (cpuData->stealPeriod + cpuData->guestPeriod) / total * 100.0;
      Meter_setItems(this, 4);
      percent = v[0]+v[1]+v[2]+v[3];
   }
   percent = CLAMP(percent, 0.0, 100.0);
   if (isnan(percent)) percent = 0.0;
   return percent;
}

void Platform_setMemoryValues(Meter* this) {
   ProcessList* pl = (ProcessList*) this->pl;
   long int usedMem = pl->usedMem;
   long int buffersMem = pl->buffersMem;
   long int cachedMem = pl->cachedMem;
   usedMem -= buffersMem + cachedMem;
   this->total = pl->totalMem;
   this->values[0] = usedMem;
   this->values[1] = buffersMem;
   this->values[2] = cachedMem;
}

void Platform_setSwapValues(Meter* this) {
   ProcessList* pl = (ProcessList*) this->pl;
   this->total = pl->totalSwap;
   this->values[0] = pl->usedSwap;
}

char* Platform_getProcessEnv(pid_t pid) {
   char procname[32+1];
   xSnprintf(procname, 32, "/proc/%d/environ", pid);
   FILE* fd = fopen(procname, "r");
   char *env = NULL;
   if (fd) {
      size_t capacity = 4096, size = 0, bytes;
      env = xMalloc(capacity);
      while (env && (bytes = fread(env+size, 1, capacity-size, fd)) > 0) {
         size += bytes;
         capacity *= 2;
         env = xRealloc(env, capacity);
      }
      fclose(fd);
      if (size < 2 || env[size-1] || env[size-2]) {
         if (size + 2 < capacity) {
            env = xRealloc(env, capacity+2);
         }
         env[size] = 0;
         env[size+1] = 0;
      }
   }
   return env;
}
