#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/statvfs.h>

#include "Platform.h"
#include "CRT.h"

#include "mountpoint.h"
#include "Settings.h"

#define BLEN 256
/*
    cat /proc/mounts
	/dev/sda6 / ext4 rw,relatime,errors=remount-ro 0 0
	/dev/sda8 /usr ext4 rw,relatime,data=ordered 0 0
	/dev/sdb1 /arm ext4 rw,relatime,data=ordered 0 0
	/dev/sdc1 /apps ext4 rw,relatime,data=ordered 0 0
	/dev/sda11 /srv ext4 rw,relatime,data=ordered 0 0
	/dev/sda14 /home ext4 rw,relatime,data=ordered 0 0
	/dev/sda9 /usr/local ext4 rw,relatime,data=ordered 0 0
	/dev/sda5 /boot ext4 rw,relatime,data=ordered 0 0
	/dev/sda7 /tmp ext4 rw,relatime,data=ordered 0 0
	/dev/sda12 /opt ext4 rw,relatime,data=ordered 0 0
	/dev/sda13 /svn ext4 rw,relatime,data=ordered 0 0
	/dev/sda10 /var ext4 rw,relatime,data=ordered 0 0
*/

/*{
typedef struct MountPoint_ {

   char device[64];
   char subdir[64];

} MountPoint;
}*/

#define MAX_MOUNTS 32
int mountCount;
MountPoint *aMountPoint;

static char *fs_error = "Error!";

int ReadMountPoints( char *fname ) {
    FILE* fp;
    char buffer[BLEN];
    int fd = 0;
    int idx;
    MountPoint *saveMountPoint;
    
    fp = fopen(fname, "r");
    if (fp == NULL)
        return 0;

	// K.I.S.S
    if (aMountPoint == NULL) {
		aMountPoint = calloc(MAX_MOUNTS, sizeof(MountPoint));
		if (aMountPoint == NULL) {
			return 0;
		}
	}

	idx = 0;
    while (fgets(buffer, BLEN, fp) != NULL) {
		if (buffer[0] != '/' || buffer[1] != 'd' || buffer[2] != 'e' || buffer[3] != 'v' || buffer[4] != '/') {
			continue;
		}
		int ok = sscanf(buffer, "%s %s", &aMountPoint[idx].device, &aMountPoint[idx].subdir);
		if (!ok) {
			continue;
		}
		idx++;
		mountCount++;
    }
    fclose(fp);
    if (mountCount > 100) {
		saveMountPoint = aMountPoint;
		aMountPoint = realloc(aMountPoint, mountCount * sizeof(MountPoint));
		if (aMountPoint == NULL) {
			aMountPoint = saveMountPoint;
		}
	}
    return mountCount;
}

MountPoint *GetMountPoints( void ) {
	return aMountPoint;
}

char *GetMountPointSubDirFromIndex( int fs ) {
    char *subdir;
    if (fs < 0 || fs > mountCount) {
        return fs_error;
    }
    subdir = &aMountPoint[fs].subdir[0];
    /* printf("subdir: %s - %p\n", subdir, (void *)subdir); */
    return subdir;
}

void FreeMountPoints( void ) {
	free(aMountPoint);
}

#define KB  1024
#define KiB 1000

int fs_get_stat( int fs, char *out, int sz ) {
	struct statvfs vfs;
	// char buf[128];
	char buf1[32];
	char buf2[32];
	char buf3[32];
	char buf4[32];
	double total;
    double available;
    double free;
    double used;
    double perc;

    *out = 0;
    
	if (fs < 0 || fs >= mountCount) {
		return 0;
	}
	int ret = statvfs(aMountPoint[fs].subdir, &vfs);
	if (!ret) {
        total = (double)(vfs.f_blocks * vfs.f_frsize) / KB;
        available = (double)(vfs.f_bavail * vfs.f_frsize) / KB;
        free = (double)(vfs.f_bfree * vfs.f_frsize) / KB;
        used = total - free;

		if (total >= KiB) {
			total = total / KB;
			if (total >= KiB) {
				total = total / KB;
				if (total >= KiB) {
					total = total / KB;
					sprintf(buf1, "%.1fTB", total);
				} else {
					sprintf(buf1, "%.1fGB", total);
				}
			} else {
				sprintf(buf1, "%.1fMB", total);
			}
		} else {
			sprintf(buf1, "%.1fKB", total);
		}
        if (used >= KiB) {
			used = used / KB;
			if (used >= KiB) {
				used = used / KB;
				if (used >= KiB) {
					used = used / KB;
					sprintf(buf2, "%.1fTB", used);
				} else {
					sprintf(buf2, "%.1fGB", used);
				}
			} else {
				sprintf(buf2, "%.1fMB", used);
			}
		} else {
			sprintf(buf2, "%.1fKB", used);
		}
		if (available >= KiB) {
			available = available / KB;
			if (available >= KiB) {
				available = available / KB;
				if (available >= KiB) {
					available = available / KB;
					sprintf(buf3, "%.1fTB", available);
				} else {
					sprintf(buf3, "%.1fGB", available);
				}
			} else {
				sprintf(buf3, "%.1fMB", available);
			}
		} else {
			sprintf(buf3, "%.1fKB", available);
		}
        perc = 100.0 * (double) (vfs.f_blocks - vfs.f_bfree) / (double) (vfs.f_blocks - vfs.f_bfree + vfs.f_bavail);
        sprintf(buf4, "%.1f%%", perc);
        xSnprintf(out, sz, "%s %s %s %s", buf1, buf2, buf3, buf4);
    }
    return ret;
}
