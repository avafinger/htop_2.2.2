/*
htop - Users_Meter.c
(C) 2020 @lex
*/

#include <utmpx.h>
#include <sys/types.h>
#include "Users_Meter.h"
#include "Platform.h"
#include "CRT.h"


/*{
#include "Meter.h"
}*/

#define UTMP_STRUCT_NAME utmpx
typedef struct UTMP_STRUCT_NAME STRUCT_UTMP;  

#define UTMP_FILE "/var/run/utmp"
#define BLEN 512

int Users_Meter_attributes[] = {
   ETH1_INTERFACE
};

int ReadUsersLoggedIn( void ) {
    FILE* fp;
    char buffer[BLEN];
    int fd = 0;
    int n;
    STRUCT_UTMP *u = (STRUCT_UTMP *)buffer;
    
    fp = fopen(UTMP_FILE, "r");
    if (fp == NULL)
        return 0;

	n = 0;
    while (fread(buffer, sizeof (*u), 1, fp) != 0) {
		if (u->ut_type == USER_PROCESS) {
			// printf("%d %s %s %s\n", u->ut_type, u->ut_line, u->ut_user, u->ut_host);		
		    n++;
		}
    }
    fclose(fp);

    return n;
}

static void Users_Meter_setValues(Meter* this, char* buffer, int len) {
	int n = ReadUsersLoggedIn();
	if (n <= 1) {
		xSnprintf(buffer, len, "%d logged in user", n);
	} else {
		xSnprintf(buffer, len, "%d logged in users", n);
	}
}


MeterClass Users_Meter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete
   },
   .updateValues = Users_Meter_setValues, 
   .defaultMode = TEXT_METERMODE,
   .maxItems = 8,
   .total = 100.0,
   .attributes = Users_Meter_attributes,
   .name = "UserLoggedIn",
   .uiName = "Users Logged in",
   .caption = "Users: "
};


