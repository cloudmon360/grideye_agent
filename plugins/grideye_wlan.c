/* 
 * 
 * compile:
 *   gcc -O2 -Wall -o grideye_wlan grideye_wlan.c -lm
 * run: 
 *   ./grideye_wlan
> cat /proc/net/wireless
 Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
  face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
 wlan0: 0000   51.  -59.  -256        0      0      0      1    107        0
 */
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>

#include "grideye_plugin.h"

static const char *_filename = "/proc/net/wireless";
static char *_device = NULL;

/* Forward */
int wlan_exit(void);
int wlan_test(char *instr, char **outstr);
int wlan_setopt(const char *optname, char *value);

/*
 * This is the API declaration
 */
static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "wlan",
    NULL,            /* input format */
    "xml",            /* output format */
    NULL,
    wlan_setopt,
    wlan_test,
    wlan_exit
};

int 
wlan_exit(void)
{
    if (_device){
	free(_device);
	_device = NULL;
    }
    return 0;
}

/*! Poll /proc wireless file for device status 
 * @param[out]  outstr  XML string with three parameters described below
 * The three parameters:
 *  q_link  Wireless link quality decimal64 with three decimals (3 fraction-digits)
 *  q_level Wireless signal level
 *  q_noise Wireless noise level
 */
int  
wlan_test(char     *instr,
	  char    **outstr)
{
    int             retval = -1;
    FILE           *f = NULL;
    char            buf[1024];
    char           *line;
    char           *s;
    double          qlk, qlv, qn;
    uint32_t        qrt;
    int64_t         q0;
    int64_t         q1;
    int64_t         q2;
    char           *str = NULL;
    size_t          slen;

    if ((f = fopen(_filename, "r")) == NULL)
	goto done;
    while ((line = fgets(buf, sizeof(buf), f)) != NULL){
	if ((s = strstr(line, _device)) == NULL)
	    continue;
	s+=strlen(_device)+1;
        if (sscanf(s, "%*d %lf %lf %lf %*d %*d %*d %u", 
		   &qlk, &qlv, &qn, &qrt) != 4)
	    break;
	q0 = (uint64_t)round(qlk*1000);
	q1 = (uint64_t)round(qlv*1000);
	q2 = (uint64_t)round(qn*1000);
	if ((slen = snprintf(NULL, 0, 
			       "<wlink>%" PRId64 ".%" PRId64 "</wlink>"
			       "<wlevel>%" PRId64 ".%" PRId64 "</wlevel>"
			       "<wnoise>%" PRId64 ".%" PRId64 "</wnoise>"
			       "<wretry>%u.0</wretry>",
			       q0/1000, q0%1000,
			       q1/1000, q1%1000,
			       q2/1000, q2%1000,
			       qrt)) <= 0)
	    goto done;
	if ((str = malloc(slen+1)) == NULL)
	    goto done;
	snprintf(str, slen+1, 
		 "<wlink>%" PRId64 ".%" PRId64 "</wlink>"
		 "<wlevel>%" PRId64 ".%" PRId64 "</wlevel>"
		 "<wnoise>%" PRId64 ".%" PRId64 "</wnoise>"
		 "<wretry>%u.0</wretry>",
		 q0/1000, q0%1000,
		 q1/1000, q1%1000,
		 q2/1000, q2%1000,
		 qrt);
	*outstr = str;
	break;
    }
    retval = 0;
 done:
    if (f != NULL)
	fclose(f);
    return retval;
}

/*! Init grideye test module. Check if file exists that is used for polling state */
int 
wlan_setopt(const char *optname,
	    char       *value)
{
    int      retval = -1;
    char    *device;
    
    if (strcmp(optname, "device"))
	return 0;
    device = value;
    if (device == NULL){
	errno = EINVAL;
	goto done;
    }
    if ((_device = strdup(device)) == NULL)
	goto done;
    retval = 0;
 done:
    return retval;
}


/* Grideye agent plugin init function must be called grideye_plugin_init */
void *
grideye_plugin_init(int version)
{
    struct stat st;

    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;
    if (stat(_filename, &st) < 0)
	return NULL;
    return (void*)&api;
}

#ifndef _NOMAIN
int main() 
{
    char   *str = NULL;

    if (grideye_plugin_init(2) == NULL)
	return -1;
    if (wlan_file(NULL, NULL, "wlan0") < 0)
	return -1;
    if (wlan_test(0, &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    wlan_exit();
    return 0;
}
#endif
