/*
htop - LinuxProcessList.c
(C) 2014 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "LinuxProcessList.h"
#include "LinuxProcess.h"
#include "CRT.h"
#include "StringUtils.h"
#include <errno.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS) || \
   (defined(HAVE_SYS_SYSMACROS_H) && HAVE_SYS_SYSMACROS_H)
#include <sys/sysmacros.h>
#endif

#ifdef HAVE_DELAYACCT
#include <netlink/attr.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/socket.h>
#include <netlink/msg.h>
#include <linux/taskstats.h>
#endif

#include "mountpoint.h"

/*{

#include "ProcessList.h"

typedef struct CPUData_ {
   unsigned long long int totalTime;
   unsigned long long int userTime;
   unsigned long long int systemTime;
   unsigned long long int systemAllTime;
   unsigned long long int idleAllTime;
   unsigned long long int idleTime;
   unsigned long long int niceTime;
   unsigned long long int ioWaitTime;
   unsigned long long int irqTime;
   unsigned long long int softIrqTime;
   unsigned long long int stealTime;
   unsigned long long int guestTime;

   unsigned long long int totalPeriod;
   unsigned long long int userPeriod;
   unsigned long long int systemPeriod;
   unsigned long long int systemAllPeriod;
   unsigned long long int idleAllPeriod;
   unsigned long long int idlePeriod;
   unsigned long long int nicePeriod;
   unsigned long long int ioWaitPeriod;
   unsigned long long int irqPeriod;
   unsigned long long int softIrqPeriod;
   unsigned long long int stealPeriod;
   unsigned long long int guestPeriod;
} CPUData;

typedef struct TtyDriver_ {
   char* path;
   unsigned int major;
   unsigned int minorFrom;
   unsigned int minorTo;
} TtyDriver;

typedef struct LinuxProcessList_ {
   ProcessList super;

   CPUData* cpus;
   TtyDriver* ttyDrivers;

   #ifdef HAVE_DELAYACCT
   struct nl_sock *netlink_socket;
   int netlink_family;
   #endif
} LinuxProcessList;

#ifndef PROCDIR
#define PROCDIR "/proc"
#endif

#ifndef PROCSTATFILE
#define PROCSTATFILE PROCDIR "/stat"
#endif

#ifndef PROCMEMINFOFILE
#define PROCMEMINFOFILE PROCDIR "/meminfo"
#endif

#ifndef PROCTTYDRIVERSFILE
#define PROCTTYDRIVERSFILE PROCDIR "/tty/drivers"
#endif

#ifndef PROC_LINE_LENGTH
#define PROC_LINE_LENGTH 512
#endif

}*/

#ifndef CLAMP
#define CLAMP(x,low,high) (((x)>(high))?(high):(((x)<(low))?(low):(x)))
#endif

static ssize_t xread(int fd, void *buf, size_t count) {
  // Read some bytes. Retry on EINTR and when we don't get as many bytes as we requested.
  size_t alreadyRead = 0;
  for(;;) {
     ssize_t res = read(fd, buf, count);
     if (res == -1 && errno == EINTR) continue;
     if (res > 0) {
       buf = ((char*)buf)+res;
       count -= res;
       alreadyRead += res;
     }
     if (res == -1) return -1;
     if (count == 0 || res == 0) return alreadyRead;
  }
}

static int sortTtyDrivers(const void* va, const void* vb) {
   TtyDriver* a = (TtyDriver*) va;
   TtyDriver* b = (TtyDriver*) vb;
   return (a->major == b->major) ? (a->minorFrom - b->minorFrom) : (a->major - b->major);
}

static void LinuxProcessList_initTtyDrivers(LinuxProcessList* this) {
   TtyDriver* ttyDrivers;
   int fd = open(PROCTTYDRIVERSFILE, O_RDONLY);
   if (fd == -1)
      return;
   char* buf = NULL;
   int bufSize = MAX_READ;
   int bufLen = 0;
   for(;;) {
      buf = realloc(buf, bufSize);
      int size = xread(fd, buf + bufLen, MAX_READ);
      if (size <= 0) {
         buf[bufLen] = '\0';
         close(fd);
         break;
      }
      bufLen += size;
      bufSize += MAX_READ;
   }
   if (bufLen == 0) {
      free(buf);
      return;
   }
   int numDrivers = 0;
   int allocd = 10;
   ttyDrivers = malloc(sizeof(TtyDriver) * allocd);
   char* at = buf;
   while (*at != '\0') {
      at = strchr(at, ' ');    // skip first token
      while (*at == ' ') at++; // skip spaces
      char* token = at;        // mark beginning of path
      at = strchr(at, ' ');    // find end of path
      *at = '\0'; at++;        // clear and skip
      ttyDrivers[numDrivers].path = strdup(token); // save
      while (*at == ' ') at++; // skip spaces
      token = at;              // mark beginning of major
      at = strchr(at, ' ');    // find end of major
      *at = '\0'; at++;        // clear and skip
      ttyDrivers[numDrivers].major = atoi(token); // save
      while (*at == ' ') at++; // skip spaces
      token = at;              // mark beginning of minorFrom
      while (*at >= '0' && *at <= '9') at++; //find end of minorFrom
      if (*at == '-') {        // if has range
         *at = '\0'; at++;        // clear and skip
         ttyDrivers[numDrivers].minorFrom = atoi(token); // save
         token = at;              // mark beginning of minorTo
         at = strchr(at, ' ');    // find end of minorTo
         *at = '\0'; at++;        // clear and skip
         ttyDrivers[numDrivers].minorTo = atoi(token); // save
      } else {                 // no range
         *at = '\0'; at++;        // clear and skip
         ttyDrivers[numDrivers].minorFrom = atoi(token); // save
         ttyDrivers[numDrivers].minorTo = atoi(token); // save
      }
      at = strchr(at, '\n');   // go to end of line
      at++;                    // skip
      numDrivers++;
      if (numDrivers == allocd) {
         allocd += 10;
         ttyDrivers = realloc(ttyDrivers, sizeof(TtyDriver) * allocd);
      }
   }
   free(buf);
   numDrivers++;
   ttyDrivers = realloc(ttyDrivers, sizeof(TtyDriver) * numDrivers);
   ttyDrivers[numDrivers - 1].path = NULL;
   qsort(ttyDrivers, numDrivers - 1, sizeof(TtyDriver), sortTtyDrivers);
   this->ttyDrivers = ttyDrivers;
}

#ifdef HAVE_DELAYACCT

static void LinuxProcessList_initNetlinkSocket(LinuxProcessList* this) {
   this->netlink_socket = nl_socket_alloc();
   if (this->netlink_socket == NULL) {
      return;
   }
   if (nl_connect(this->netlink_socket, NETLINK_GENERIC) < 0) {
      return;
   }
   this->netlink_family = genl_ctrl_resolve(this->netlink_socket, TASKSTATS_GENL_NAME);
}

#endif

ProcessList* ProcessList_new(UsersTable* usersTable, Hashtable* pidWhiteList, uid_t userId) {
   LinuxProcessList* this = xCalloc(1, sizeof(LinuxProcessList));
   ProcessList* pl = &(this->super);
   ProcessList_init(pl, Class(LinuxProcess), usersTable, pidWhiteList, userId);

   LinuxProcessList_initTtyDrivers(this);

   #ifdef HAVE_DELAYACCT
   LinuxProcessList_initNetlinkSocket(this);
   #endif

   // Update CPU count:
   FILE* file = fopen(PROCSTATFILE, "r");
   if (file == NULL) {
      CRT_fatalError("Cannot open " PROCSTATFILE);
   }
   char buffer[PROC_LINE_LENGTH + 1];
   int cpus = -1;
   do {
      cpus++;
      char * s = fgets(buffer, PROC_LINE_LENGTH, file);
      (void) s;
   } while (String_startsWith(buffer, "cpu"));
   fclose(file);

   pl->cpuCount = MAX(cpus - 1, 1);
   this->cpus = xCalloc(cpus, sizeof(CPUData));

   for (int i = 0; i < cpus; i++) {
      this->cpus[i].totalTime = 1;
      this->cpus[i].totalPeriod = 1;
   }

   // Update fs mount count;
   pl->mountCount = ReadMountPoints( "/proc/mounts" );

   return pl;
}

void ProcessList_delete(ProcessList* pl) {
   LinuxProcessList* this = (LinuxProcessList*) pl;
   ProcessList_done(pl);
   free(this->cpus);
   if (this->ttyDrivers) {
      for(int i = 0; this->ttyDrivers[i].path; i++) {
         free(this->ttyDrivers[i].path);
      }
      free(this->ttyDrivers);
   }
   #ifdef HAVE_DELAYACCT
   if (this->netlink_socket) {
      nl_close(this->netlink_socket);
      nl_socket_free(this->netlink_socket);
   }
   #endif
   free(this);
}

static double jiffy = 0.0;

static inline unsigned long long LinuxProcess_adjustTime(unsigned long long t) {
   if(jiffy == 0.0) jiffy = sysconf(_SC_CLK_TCK);
   double jiffytime = 1.0 / jiffy;
   return (unsigned long long) t * jiffytime * 100;
}

static bool LinuxProcessList_readStatFile(Process *process, const char* dirname, const char* name, char* command, int* commLen) {
   LinuxProcess* lp = (LinuxProcess*) process;
   char filename[MAX_NAME+1];
   xSnprintf(filename, MAX_NAME, "%s/%s/stat", dirname, name);
   int fd = open(filename, O_RDONLY);
   if (fd == -1)
      return false;

   static char buf[MAX_READ+1];

   int size = xread(fd, buf, MAX_READ);
   close(fd);
   if (size <= 0) return false;
   buf[size] = '\0';

   assert(process->pid == atoi(buf));
   char *location = strchr(buf, ' ');
   if (!location) return false;

   location += 2;
   char *end = strrchr(location, ')');
   if (!end) return false;

   int commsize = end - location;
   memcpy(command, location, commsize);
   command[commsize] = '\0';
   *commLen = commsize;
   location = end + 2;

   process->state = location[0];
   location += 2;
   process->ppid = strtol(location, &location, 10);
   location += 1;
   process->pgrp = strtoul(location, &location, 10);
   location += 1;
   process->session = strtoul(location, &location, 10);
   location += 1;
   process->tty_nr = strtoul(location, &location, 10);
   location += 1;
   process->tpgid = strtol(location, &location, 10);
   location += 1;
   process->flags = strtoul(location, &location, 10);
   location += 1;
   process->minflt = strtoull(location, &location, 10);
   location += 1;
   lp->cminflt = strtoull(location, &location, 10);
   location += 1;
   process->majflt = strtoull(location, &location, 10);
   location += 1;
   lp->cmajflt = strtoull(location, &location, 10);
   location += 1;
   lp->utime = LinuxProcess_adjustTime(strtoull(location, &location, 10));
   location += 1;
   lp->stime = LinuxProcess_adjustTime(strtoull(location, &location, 10));
   location += 1;
   lp->cutime = LinuxProcess_adjustTime(strtoull(location, &location, 10));
   location += 1;
   lp->cstime = LinuxProcess_adjustTime(strtoull(location, &location, 10));
   location += 1;
   process->priority = strtol(location, &location, 10);
   location += 1;
   process->nice = strtol(location, &location, 10);
   location += 1;
   process->nlwp = strtol(location, &location, 10);
   location += 1;
   for (int i=0; i<17; i++) location = strchr(location, ' ')+1;
   process->exit_signal = strtol(location, &location, 10);
   location += 1;
   assert(location != NULL);
   process->processor = strtol(location, &location, 10);

   process->time = lp->utime + lp->stime;

   return true;
}


static bool LinuxProcessList_statProcessDir(Process* process, const char* dirname, char* name, time_t curTime) {
   char filename[MAX_NAME+1];
   filename[MAX_NAME] = '\0';

   xSnprintf(filename, MAX_NAME, "%s/%s", dirname, name);
   struct stat sstat;
   int statok = stat(filename, &sstat);
   if (statok == -1)
      return false;
   process->st_uid = sstat.st_uid;

   struct tm date;
   time_t ctime = sstat.st_ctime;
   process->starttime_ctime = ctime;
   (void) localtime_r((time_t*) &ctime, &date);
   strftime(process->starttime_show, 7, ((ctime > curTime - 86400) ? "%R " : "%b%d "), &date);

   return true;
}

#ifdef HAVE_TASKSTATS

static void LinuxProcessList_readIoFile(LinuxProcess* process, const char* dirname, char* name, unsigned long long now) {
   char filename[MAX_NAME+1];
   filename[MAX_NAME] = '\0';

   xSnprintf(filename, MAX_NAME, "%s/%s/io", dirname, name);
   int fd = open(filename, O_RDONLY);
   if (fd == -1) {
      process->io_rate_read_bps = -1;
      process->io_rate_write_bps = -1;
      process->io_rchar = -1LL;
      process->io_wchar = -1LL;
      process->io_syscr = -1LL;
      process->io_syscw = -1LL;
      process->io_read_bytes = -1LL;
      process->io_write_bytes = -1LL;
      process->io_cancelled_write_bytes = -1LL;
      process->io_rate_read_time = -1LL;
      process->io_rate_write_time = -1LL;
      return;
   }

   char buffer[1024];
   ssize_t buflen = xread(fd, buffer, 1023);
   close(fd);
   if (buflen < 1) return;
   buffer[buflen] = '\0';
   unsigned long long last_read = process->io_read_bytes;
   unsigned long long last_write = process->io_write_bytes;
   char *buf = buffer;
   char *line = NULL;
   while ((line = strsep(&buf, "\n")) != NULL) {
      switch (line[0]) {
      case 'r':
         if (line[1] == 'c' && strncmp(line+2, "har: ", 5) == 0)
            process->io_rchar = strtoull(line+7, NULL, 10);
         else if (strncmp(line+1, "ead_bytes: ", 11) == 0) {
            process->io_read_bytes = strtoull(line+12, NULL, 10);
            process->io_rate_read_bps =
               ((double)(process->io_read_bytes - last_read))/(((double)(now - process->io_rate_read_time))/1000);
            process->io_rate_read_time = now;
         }
         break;
      case 'w':
         if (line[1] == 'c' && strncmp(line+2, "har: ", 5) == 0)
            process->io_wchar = strtoull(line+7, NULL, 10);
         else if (strncmp(line+1, "rite_bytes: ", 12) == 0) {
            process->io_write_bytes = strtoull(line+13, NULL, 10);
            process->io_rate_write_bps =
               ((double)(process->io_write_bytes - last_write))/(((double)(now - process->io_rate_write_time))/1000);
            process->io_rate_write_time = now;
         }
         break;
      case 's':
         if (line[4] == 'r' && strncmp(line+1, "yscr: ", 6) == 0) {
            process->io_syscr = strtoull(line+7, NULL, 10);
         } else if (strncmp(line+1, "yscw: ", 6) == 0) {
            process->io_syscw = strtoull(line+7, NULL, 10);
         }
         break;
      case 'c':
         if (strncmp(line+1, "ancelled_write_bytes: ", 22) == 0) {
           process->io_cancelled_write_bytes = strtoull(line+23, NULL, 10);
        }
      }
   }
}

#endif



static bool LinuxProcessList_readStatmFile(LinuxProcess* process, const char* dirname, const char* name) {
   char filename[MAX_NAME+1];
   xSnprintf(filename, MAX_NAME, "%s/%s/statm", dirname, name);
   int fd = open(filename, O_RDONLY);
   if (fd == -1)
      return false;
   char buf[PROC_LINE_LENGTH + 1];
   ssize_t rres = xread(fd, buf, PROC_LINE_LENGTH);
   close(fd);
   if (rres < 1) return false;

   char *p = buf;
   errno = 0;
   process->super.m_size = strtol(p, &p, 10); if (*p == ' ') p++;
   if (process->super.m_size_prev == 0 || process->super.m_size < process->super.m_size_prev) {
       process->super.m_size_prev = process->super.m_size;
   }

   process->super.m_memleak = process->super.m_size - process->super.m_size_prev;
   if (process->super.m_memleak < 0)
       process->super.m_memleak = 0;

   process->super.m_resident = strtol(p, &p, 10); if (*p == ' ') p++;
   process->m_share = strtol(p, &p, 10); if (*p == ' ') p++;
   process->m_trs = strtol(p, &p, 10); if (*p == ' ') p++;
   process->m_lrs = strtol(p, &p, 10); if (*p == ' ') p++;
   process->m_drs = strtol(p, &p, 10); if (*p == ' ') p++;
   process->m_dt = strtol(p, &p, 10);
   return (errno == 0);
}

#ifdef HAVE_OPENVZ

static void LinuxProcessList_readOpenVZData(LinuxProcess* process, const char* dirname, const char* name) {
   if ( (access("/proc/vz", R_OK) != 0)) {
      process->vpid = process->super.pid;
      process->ctid = 0;
      return;
   }
   char filename[MAX_NAME+1];
   xSnprintf(filename, MAX_NAME, "%s/%s/stat", dirname, name);
   FILE* file = fopen(filename, "r");
   if (!file)
      return;
   (void) fscanf(file,
      "%*32u %*32s %*1c %*32u %*32u %*32u %*32u %*32u %*32u %*32u "
      "%*32u %*32u %*32u %*32u %*32u %*32u %*32u %*32u "
      "%*32u %*32u %*32u %*32u %*32u %*32u %*32u %*32u "
      "%*32u %*32u %*32u %*32u %*32u %*32u %*32u %*32u "
      "%*32u %*32u %*32u %*32u %*32u %*32u %*32u %*32u "
      "%*32u %*32u %*32u %*32u %*32u %*32u %*32u "
      "%*32u %*32u %32u %32u",
      &process->vpid, &process->ctid);
   fclose(file);
   return;
}

#endif

#ifdef HAVE_CGROUP

static void LinuxProcessList_readCGroupFile(LinuxProcess* process, const char* dirname, const char* name) {
   char filename[MAX_NAME+1];
   xSnprintf(filename, MAX_NAME, "%s/%s/cgroup", dirname, name);
   FILE* file = fopen(filename, "r");
   if (!file) {
      process->cgroup = xStrdup("");
      return;
   }
   char output[PROC_LINE_LENGTH + 1];
   output[0] = '\0';
   char* at = output;
   int left = PROC_LINE_LENGTH;
   while (!feof(file) && left > 0) {
      char buffer[PROC_LINE_LENGTH + 1];
      char *ok = fgets(buffer, PROC_LINE_LENGTH, file);
      if (!ok) break;
      char* group = strchr(buffer, ':');
      if (!group) break;
      if (at != output) {
         *at = ';';
         at++;
         left--;
      }
      int wrote = snprintf(at, left, "%s", group);
      left -= wrote;
   }
   fclose(file);
   free(process->cgroup);
   process->cgroup = xStrdup(output);
}

#endif

#ifdef HAVE_VSERVER

static void LinuxProcessList_readVServerData(LinuxProcess* process, const char* dirname, const char* name) {
   char filename[MAX_NAME+1];
   xSnprintf(filename, MAX_NAME, "%s/%s/status", dirname, name);
   FILE* file = fopen(filename, "r");
   if (!file)
      return;
   char buffer[PROC_LINE_LENGTH + 1];
   process->vxid = 0;
   while (fgets(buffer, PROC_LINE_LENGTH, file)) {
      if (String_startsWith(buffer, "VxID:")) {
         int vxid;
         int ok = sscanf(buffer, "VxID:\t%32d", &vxid);
         if (ok >= 1) {
            process->vxid = vxid;
         }
      }
      #if defined HAVE_ANCIENT_VSERVER
      else if (String_startsWith(buffer, "s_context:")) {
         int vxid;
         int ok = sscanf(buffer, "s_context:\t%32d", &vxid);
         if (ok >= 1) {
            process->vxid = vxid;
         }
      }
      #endif
   }
   fclose(file);
}

#endif

static void LinuxProcessList_readOomData(LinuxProcess* process, const char* dirname, const char* name) {
   char filename[MAX_NAME+1];
   xSnprintf(filename, MAX_NAME, "%s/%s/oom_score", dirname, name);
   FILE* file = fopen(filename, "r");
   if (!file) {
      return;
   }
   char buffer[PROC_LINE_LENGTH + 1];
   if (fgets(buffer, PROC_LINE_LENGTH, file)) {
      unsigned int oom;
      int ok = sscanf(buffer, "%32u", &oom);
      if (ok >= 1) {
         process->oom = oom;
      }
   }
   fclose(file);
}

#ifdef HAVE_DELAYACCT

static int handleNetlinkMsg(struct nl_msg *nlmsg, void *linuxProcess) {
   struct nlmsghdr *nlhdr;
   struct nlattr *nlattrs[TASKSTATS_TYPE_MAX + 1];
   struct nlattr *nlattr;
   struct taskstats *stats;
   int rem;
   unsigned long long int timeDelta;
   LinuxProcess* lp = (LinuxProcess*) linuxProcess;

   nlhdr = nlmsg_hdr(nlmsg);

   if (genlmsg_parse(nlhdr, 0, nlattrs, TASKSTATS_TYPE_MAX, NULL) < 0) {
      return NL_SKIP;
   }

   if ((nlattr = nlattrs[TASKSTATS_TYPE_AGGR_PID]) || (nlattr = nlattrs[TASKSTATS_TYPE_NULL])) {
      stats = nla_data(nla_next(nla_data(nlattr), &rem));
      assert(lp->super.pid == stats->ac_pid);
      timeDelta = (stats->ac_etime*1000 - lp->delay_read_time);
      #define BOUNDS(x) isnan(x) ? 0.0 : (x > 100) ? 100.0 : x;
      #define DELTAPERC(x,y) BOUNDS((float) (x - y) / timeDelta * 100);
      lp->cpu_delay_percent = DELTAPERC(stats->cpu_delay_total, lp->cpu_delay_total);
      lp->blkio_delay_percent = DELTAPERC(stats->blkio_delay_total, lp->blkio_delay_total);
      lp->swapin_delay_percent = DELTAPERC(stats->swapin_delay_total, lp->swapin_delay_total);
      #undef DELTAPERC
      #undef BOUNDS
      lp->swapin_delay_total = stats->swapin_delay_total;
      lp->blkio_delay_total = stats->blkio_delay_total;
      lp->cpu_delay_total = stats->cpu_delay_total;
      lp->delay_read_time = stats->ac_etime*1000;
   }
   return NL_OK;
}

static void LinuxProcessList_readDelayAcctData(LinuxProcessList* this, LinuxProcess* process) {
   struct nl_msg *msg;

   if (nl_socket_modify_cb(this->netlink_socket, NL_CB_VALID, NL_CB_CUSTOM, handleNetlinkMsg, process) < 0) {
      return;
   }

   if (! (msg = nlmsg_alloc())) {
      return;
   }

   if (! genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, this->netlink_family, 0, NLM_F_REQUEST, TASKSTATS_CMD_GET, TASKSTATS_VERSION)) {
      nlmsg_free(msg);
   }

   if (nla_put_u32(msg, TASKSTATS_CMD_ATTR_PID, process->super.pid) < 0) {
      nlmsg_free(msg);
   }

   if (nl_send_sync(this->netlink_socket, msg) < 0) {
      process->swapin_delay_percent = -1LL;
      process->blkio_delay_percent = -1LL;
      process->cpu_delay_percent = -1LL;
      return;
   }

   if (nl_recvmsgs_default(this->netlink_socket) < 0) {
      return;
   }
}

#endif

static void setCommand(Process* process, const char* command, int len) {
   if (process->comm && process->commLen >= len) {
      strncpy(process->comm, command, len + 1);
   } else {
      free(process->comm);
      process->comm = xStrdup(command);
   }
   process->commLen = len;
}

static bool LinuxProcessList_readCmdlineFile(Process* process, const char* dirname, const char* name) {
   char filename[MAX_NAME+1];
   xSnprintf(filename, MAX_NAME, "%s/%s/cmdline", dirname, name);
   int fd = open(filename, O_RDONLY);
   if (fd == -1)
      return false;

   char command[4096+1]; // max cmdline length on Linux
   int amtRead = xread(fd, command, sizeof(command) - 1);
   close(fd);
   int tokenEnd = 0;
   int lastChar = 0;
   if (amtRead == 0) {
      ((LinuxProcess*)process)->isKernelThread = true;
      return true;
   } else if (amtRead < 0) {
      return false;
   }
   for (int i = 0; i < amtRead; i++) {
      if (command[i] == '\0' || command[i] == '\n') {
         if (tokenEnd == 0) {
            tokenEnd = i;
         }
         command[i] = ' ';
      } else {
         lastChar = i;
      }
   }
   if (tokenEnd == 0) {
      tokenEnd = amtRead;
   }
   command[lastChar + 1] = '\0';
   process->basenameOffset = tokenEnd;
   setCommand(process, command, lastChar);

   return true;
}

static char* LinuxProcessList_updateTtyDevice(TtyDriver* ttyDrivers, unsigned int tty_nr) {
   unsigned int maj = major(tty_nr);
   unsigned int min = minor(tty_nr);

   int i = -1;
   for (;;) {
      i++;
      if ((!ttyDrivers[i].path) || maj < ttyDrivers[i].major) {
         break;
      }
      if (maj > ttyDrivers[i].major) {
         continue;
      }
      if (min < ttyDrivers[i].minorFrom) {
         break;
      }
      if (min > ttyDrivers[i].minorTo) {
         continue;
      }
      unsigned int idx = min - ttyDrivers[i].minorFrom;
      struct stat sstat;
      char* fullPath;
      for(;;) {
         asprintf(&fullPath, "%s/%d", ttyDrivers[i].path, idx);
         int err = stat(fullPath, &sstat);
         if (err == 0 && major(sstat.st_rdev) == maj && minor(sstat.st_rdev) == min) return fullPath;
         free(fullPath);
         asprintf(&fullPath, "%s%d", ttyDrivers[i].path, idx);
         err = stat(fullPath, &sstat);
         if (err == 0 && major(sstat.st_rdev) == maj && minor(sstat.st_rdev) == min) return fullPath;
         free(fullPath);
         if (idx == min) break;
         idx = min;
      }
      int err = stat(ttyDrivers[i].path, &sstat);
      if (err == 0 && tty_nr == sstat.st_rdev) return strdup(ttyDrivers[i].path);
   }
   char* out;
   asprintf(&out, "/dev/%u:%u", maj, min);
   return out;
}

static bool LinuxProcessList_recurseProcTree(LinuxProcessList* this, const char* dirname, Process* parent, double period, struct timeval tv) {
   ProcessList* pl = (ProcessList*) this;
   DIR* dir;
   struct dirent* entry;
   Settings* settings = pl->settings;

   time_t curTime = tv.tv_sec;
   #ifdef HAVE_TASKSTATS
   unsigned long long now = tv.tv_sec*1000LL+tv.tv_usec/1000LL;
   #endif

   dir = opendir(dirname);
   if (!dir) return false;
   int cpus = pl->cpuCount;
   bool hideKernelThreads = settings->hideKernelThreads;
   bool hideUserlandThreads = settings->hideUserlandThreads;
   while ((entry = readdir(dir)) != NULL) {
      char* name = entry->d_name;

      // The RedHat kernel hides threads with a dot.
      // I believe this is non-standard.
      if ((!settings->hideThreads) && name[0] == '.') {
         name++;
      }

      // Just skip all non-number directories.
      if (name[0] < '0' || name[0] > '9') {
         continue;
      }

      // filename is a number: process directory
      int pid = atoi(name);

      if (parent && pid == parent->pid)
         continue;

      if (pid <= 0)
         continue;

      bool preExisting = false;
      Process* proc = ProcessList_getProcess(pl, pid, &preExisting, (Process_New) LinuxProcess_new);
      proc->tgid = parent ? parent->pid : pid;

      LinuxProcess* lp = (LinuxProcess*) proc;

      char subdirname[MAX_NAME+1];
      xSnprintf(subdirname, MAX_NAME, "%s/%s/task", dirname, name);
      LinuxProcessList_recurseProcTree(this, subdirname, proc, period, tv);

      #ifdef HAVE_TASKSTATS
      if (settings->flags & PROCESS_FLAG_IO)
         LinuxProcessList_readIoFile(lp, dirname, name, now);
      #endif

      if (! LinuxProcessList_readStatmFile(lp, dirname, name))
         goto errorReadingProcess;

      proc->show = ! ((hideKernelThreads && Process_isKernelThread(proc)) || (hideUserlandThreads && Process_isUserlandThread(proc)));

      char command[MAX_NAME+1];
      unsigned long long int lasttimes = (lp->utime + lp->stime);
      int commLen = 0;
      unsigned int tty_nr = proc->tty_nr;
      if (! LinuxProcessList_readStatFile(proc, dirname, name, command, &commLen))
         goto errorReadingProcess;
      if (tty_nr != proc->tty_nr && this->ttyDrivers) {
         free(lp->ttyDevice);
         lp->ttyDevice = LinuxProcessList_updateTtyDevice(this->ttyDrivers, proc->tty_nr);
      }
      if (settings->flags & PROCESS_FLAG_LINUX_IOPRIO)
         LinuxProcess_updateIOPriority(lp);
      float percent_cpu = (lp->utime + lp->stime - lasttimes) / period * 100.0;
      proc->percent_cpu = CLAMP(percent_cpu, 0.0, cpus * 100.0);
      if (isnan(proc->percent_cpu)) proc->percent_cpu = 0.0;
      proc->percent_mem = (proc->m_resident * PAGE_SIZE_KB) / (double)(pl->totalMem) * 100.0;

      if(!preExisting) {

         if (! LinuxProcessList_statProcessDir(proc, dirname, name, curTime))
            goto errorReadingProcess;

         proc->user = UsersTable_getRef(pl->usersTable, proc->st_uid);

         #ifdef HAVE_OPENVZ
         if (settings->flags & PROCESS_FLAG_LINUX_OPENVZ) {
            LinuxProcessList_readOpenVZData(lp, dirname, name);
         }
         #endif

         #ifdef HAVE_VSERVER
         if (settings->flags & PROCESS_FLAG_LINUX_VSERVER) {
            LinuxProcessList_readVServerData(lp, dirname, name);
         }
         #endif

         if (! LinuxProcessList_readCmdlineFile(proc, dirname, name)) {
            goto errorReadingProcess;
         }

         ProcessList_add(pl, proc);
      } else {
         if (settings->updateProcessNames && proc->state != 'Z') {
            if (! LinuxProcessList_readCmdlineFile(proc, dirname, name)) {
               goto errorReadingProcess;
            }
         }
      }

      #ifdef HAVE_DELAYACCT
      LinuxProcessList_readDelayAcctData(this, lp);
      #endif

      #ifdef HAVE_CGROUP
      if (settings->flags & PROCESS_FLAG_LINUX_CGROUP)
         LinuxProcessList_readCGroupFile(lp, dirname, name);
      #endif

      if (settings->flags & PROCESS_FLAG_LINUX_OOM)
         LinuxProcessList_readOomData(lp, dirname, name);

      if (proc->state == 'Z' && (proc->basenameOffset == 0)) {
         proc->basenameOffset = -1;
         setCommand(proc, command, commLen);
      } else if (Process_isThread(proc)) {
         if (settings->showThreadNames || Process_isKernelThread(proc) || (proc->state == 'Z' && proc->basenameOffset == 0)) {
            proc->basenameOffset = -1;
            setCommand(proc, command, commLen);
         } else if (settings->showThreadNames) {
            if (! LinuxProcessList_readCmdlineFile(proc, dirname, name))
               goto errorReadingProcess;
         }
         if (Process_isKernelThread(proc)) {
            pl->kernelThreads++;
         } else {
            pl->userlandThreads++;
         }
      }

      pl->totalTasks++;
      if (proc->state == 'R')
         pl->runningTasks++;
      proc->updated = true;
      continue;

      // Exception handler.
      errorReadingProcess: {
         if (preExisting) {
            ProcessList_remove(pl, proc);
         } else {
            Process_delete((Object*)proc);
         }
      }
   }
   closedir(dir);
   return true;
}

static inline void LinuxProcessList_scanMemoryInfo(ProcessList* this) {
   unsigned long long int swapFree = 0;
   unsigned long long int shmem = 0;
   unsigned long long int sreclaimable = 0;

   FILE* file = fopen(PROCMEMINFOFILE, "r");
   if (file == NULL) {
      CRT_fatalError("Cannot open " PROCMEMINFOFILE);
   }
   char buffer[128];
   while (fgets(buffer, 128, file)) {

      #define tryRead(label, variable) (String_startsWith(buffer, label) && sscanf(buffer + strlen(label), " %32llu kB", variable))
      switch (buffer[0]) {
      case 'M':
         if (tryRead("MemTotal:", &this->totalMem)) {}
         else if (tryRead("MemFree:", &this->freeMem)) {}
         else if (tryRead("MemShared:", &this->sharedMem)) {}
         break;
      case 'B':
         if (tryRead("Buffers:", &this->buffersMem)) {}
         break;
      case 'C':
         if (tryRead("Cached:", &this->cachedMem)) {}
         break;
      case 'S':
         switch (buffer[1]) {
         case 'w':
            if (tryRead("SwapTotal:", &this->totalSwap)) {}
            else if (tryRead("SwapFree:", &swapFree)) {}
            break;
         case 'h':
            if (tryRead("Shmem:", &shmem)) {}
            break;
         case 'R':
            if (tryRead("SReclaimable:", &sreclaimable)) {}
            break;
         }
         break;
      }
      #undef tryRead
   }

   this->usedMem = this->totalMem - this->freeMem;
   this->cachedMem = this->cachedMem + sreclaimable - shmem;
   this->usedSwap = this->totalSwap - swapFree;
   fclose(file);
}

static inline double LinuxProcessList_scanCPUTime(LinuxProcessList* this) {

   FILE* file = fopen(PROCSTATFILE, "r");
   if (file == NULL) {
      CRT_fatalError("Cannot open " PROCSTATFILE);
   }
   int cpus = this->super.cpuCount;
   assert(cpus > 0);
   for (int i = 0; i <= cpus; i++) {
      char buffer[PROC_LINE_LENGTH + 1];
      unsigned long long int usertime, nicetime, systemtime, idletime;
      unsigned long long int ioWait, irq, softIrq, steal, guest, guestnice;
      ioWait = irq = softIrq = steal = guest = guestnice = 0;
      // Depending on your kernel version,
      // 5, 7, 8 or 9 of these fields will be set.
      // The rest will remain at zero.
      char* ok = fgets(buffer, PROC_LINE_LENGTH, file);
      if (!ok) buffer[0] = '\0';
      if (i == 0)
         sscanf(buffer,   "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",         &usertime, &nicetime, &systemtime, &idletime, &ioWait, &irq, &softIrq, &steal, &guest, &guestnice);
      else {
         int cpuid;
         sscanf(buffer, "cpu%4d %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu", &cpuid, &usertime, &nicetime, &systemtime, &idletime, &ioWait, &irq, &softIrq, &steal, &guest, &guestnice);
         assert(cpuid == i - 1);
      }
      // Guest time is already accounted in usertime
      usertime = usertime - guest;
      nicetime = nicetime - guestnice;
      // Fields existing on kernels >= 2.6
      // (and RHEL's patched kernel 2.4...)
      unsigned long long int idlealltime = idletime + ioWait;
      unsigned long long int systemalltime = systemtime + irq + softIrq;
      unsigned long long int virtalltime = guest + guestnice;
      unsigned long long int totaltime = usertime + nicetime + systemalltime + idlealltime + steal + virtalltime;
      CPUData* cpuData = &(this->cpus[i]);
      // Since we do a subtraction (usertime - guest) and cputime64_to_clock_t()
      // used in /proc/stat rounds down numbers, it can lead to a case where the
      // integer overflow.
      #define WRAP_SUBTRACT(a,b) (a > b) ? a - b : 0
      cpuData->userPeriod = WRAP_SUBTRACT(usertime, cpuData->userTime);
      cpuData->nicePeriod = WRAP_SUBTRACT(nicetime, cpuData->niceTime);
      cpuData->systemPeriod = WRAP_SUBTRACT(systemtime, cpuData->systemTime);
      cpuData->systemAllPeriod = WRAP_SUBTRACT(systemalltime, cpuData->systemAllTime);
      cpuData->idleAllPeriod = WRAP_SUBTRACT(idlealltime, cpuData->idleAllTime);
      cpuData->idlePeriod = WRAP_SUBTRACT(idletime, cpuData->idleTime);
      cpuData->ioWaitPeriod = WRAP_SUBTRACT(ioWait, cpuData->ioWaitTime);
      cpuData->irqPeriod = WRAP_SUBTRACT(irq, cpuData->irqTime);
      cpuData->softIrqPeriod = WRAP_SUBTRACT(softIrq, cpuData->softIrqTime);
      cpuData->stealPeriod = WRAP_SUBTRACT(steal, cpuData->stealTime);
      cpuData->guestPeriod = WRAP_SUBTRACT(virtalltime, cpuData->guestTime);
      cpuData->totalPeriod = WRAP_SUBTRACT(totaltime, cpuData->totalTime);
      #undef WRAP_SUBTRACT
      cpuData->userTime = usertime;
      cpuData->niceTime = nicetime;
      cpuData->systemTime = systemtime;
      cpuData->systemAllTime = systemalltime;
      cpuData->idleAllTime = idlealltime;
      cpuData->idleTime = idletime;
      cpuData->ioWaitTime = ioWait;
      cpuData->irqTime = irq;
      cpuData->softIrqTime = softIrq;
      cpuData->stealTime = steal;
      cpuData->guestTime = virtalltime;
      cpuData->totalTime = totaltime;
   }
   double period = (double)this->cpus[0].totalPeriod / cpus;
   fclose(file);
   return period;
}

void ProcessList_goThroughEntries(ProcessList* super) {
   LinuxProcessList* this = (LinuxProcessList*) super;

   LinuxProcessList_scanMemoryInfo(super);
   double period = LinuxProcessList_scanCPUTime(this);

   struct timeval tv;
   gettimeofday(&tv, NULL);
   LinuxProcessList_recurseProcTree(this, PROCDIR, NULL, period, tv);
}
