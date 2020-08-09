#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <linux/if_link.h>
#include <signal.h>

#include "interfaces.h"
#include "Settings.h"

#define BLEN 256
/*
    /etc/armbian-release
    VERSION=20.02.1

    /etc/debian_version
    buster/sid

    cat /etc/lsb-release
    DISTRIB_DESCRIPTION="Ubuntu 18.04.5 LTS"

    cat /etc/os-release
    PRETTY_NAME="Ubuntu 18.04.5 LTS"
    VERSION="18.04.5 LTS (Bionic Beaver)"
    ID=ubuntu

    cat /proc/version
    Linux version 5.3.0-52-generic (buildd@lgw01-amd64-037) (gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04)) #46~18.04.1-Ubuntu SMP Thu Apr 30 16:13:51 UTC 2020
*/


int ReadKeyValue( char *fname, char *key, char *value) {
    FILE* fp;
    char buffer[BLEN];
    int fd = 0;
    int len = strlen(key);
    char *str;
    char *str_end;
    
    fp = fopen(fname, "r");
    if (fp == NULL)
        return fd;

    while (fgets(buffer, BLEN, fp) != NULL) {
        str = strstr(buffer, key);
        if (str != NULL) {
            fd = 1;
            str += len;
            if (*str == '\"') {
                str++;
                len = strlen(str);
                str_end = str + len;
                while (*str_end != '\"') {
                        *str_end = 0;
                        str_end--;
                        if (str_end == str)
                            break;
                }
                *str_end = 0;
                strcpy(value, str);
                fd = 1;
            } else {
                strcpy(value, str);
                fd = 1;
            }
        }
    }
    fclose(fp);
    return fd;
}

int ReadTokenValue( char *fname, char *key, char *value) {
    FILE* fp;
    char buffer[BLEN];
    int fd = 0;
    int len = strlen(key);
    char *str;
    char *str_end;
    
    fp = fopen(fname, "r");
    if (fp == NULL)
        return fd;

    while (fgets(buffer, BLEN, fp) != NULL) {
        str = strstr(buffer, key);
        if (str != NULL) {
            fd = 1;
            str += len;
            str++;
            str_end = str;
            while (*str_end != ' ' && *str_end != 0) {
                    str_end++;
            }
            *str_end = 0;
            strcpy(value, str);
            fd = 1;
        }
    }
    fclose(fp);
    return fd;
}


int findIP_interface(char *dev, char *szIP, int bufsz) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;
    char host[NI_MAXHOST];
    int found_if = 0;
    int len = strlen(dev);
    
    /* get interface information */
    if (getifaddrs(&ifaddr) != -1)  {
        for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
            if (ifa->ifa_addr == NULL)
                continue;
            
            family = ifa->ifa_addr->sa_family;
            if ((strncmp (dev, ifa->ifa_name, len ) == 0) && (family == AF_INET || family == AF_INET6)) {
                s = getnameinfo(ifa->ifa_addr,
                       (ifa->ifa_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :
                                             sizeof(struct sockaddr_in6),
                       host, NI_MAXHOST,
                       NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                   break;
                }
                found_if = 1;
                break;
            } 
        }
        if (found_if) {
            snprintf ( szIP, bufsz, "%s", host );
        }
        freeifaddrs(ifaddr);
    }
    return found_if;
}
