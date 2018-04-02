/* 
 * 
 * compile:
 *   gcc -O2 -Wall -o grideye_iwget grideye_iwget.c -lm
 * run: 
 *   ./grideye_iwget
 * $ iwgetid    # essid
 * wlan0     ESSID:"eduroam"
 * $ iwgetid -a # access point address
 * wlan0     Access Point/Cell: 58:B6:33:78:D4:AC
 * $ iwgetid -c # current channel
 * wlan0     Channel:56
 * $ iwgetid -f # current frequency
 * wlan0     Frequency:5.28 GHz
 * $ iwgetid -p # protocol name
 * wlan0     Protocol Name:"IEEE 802.11AC”
 *
 * XXX: Consider using --raw option to iwgetid   
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
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "grideye_plugin.h"

static const char *_iwgetprog = "/sbin/iwgetid";
static char *_device = NULL;

/* Forward */
int iwget_exit(void);
int iwget_test(char *instr, char **outstr);
int iwget_setopt(const char *optname, char *value);
/*
 * This is the API declaration
 */
static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "iwget",
    NULL,            /* input format */
    "xml",            /* output format */
    iwget_setopt,
    iwget_test,
    iwget_exit
};

int 
iwget_exit(void)
{
    if (_device){
	free(_device);
	_device = NULL;
    }
    return 0;
}

/*!
 * @param[in]  p      The string to use (+ offset)
 * @param[in]  offset Added offset on string
 * XXX: Why remove and then add double quotes?
 */
static int
stringadd(char  *p,
	  int    offset,
	  char  *keyword,
	  char **s0,
	  int   *s0len)
{
    int retval = -1;
    int slen;

    if (p==NULL)
	return 0;
    p += offset;
    if (p[0] == '\"')
	p++;
    /* Strip optional " if they exists and add them (again) below */
    if (p[strlen(p)-1] == '\"')
	p[strlen(p)-1] = '\0';
    slen = strlen(p)+2*strlen(keyword)+5+2; /* 7 is <>""</>  */
    if ((*s0 = realloc(*s0, *s0len+slen+1)) == NULL){
	perror("realloc");
	goto done;
    }	
    /* Always add double quotes (may have been removed above) */
    snprintf(*s0+*s0len, slen+1, "<%s>\"%s\"</%s>", keyword, p, keyword);
    *s0len += slen;
    retval = 0;
 done:
    return retval;
}

/*! Poll /proc wireless file for device status 
 * @param[out]  outstr  XML string with three parameters described below
 * The string contains the following parameters (or possibly a subset):
 * iwessid
 * iwaddr
 * iwchan
 * iwfreq
 * iwproto
 */
int  
iwget_test(char  *instr,
	   char **outstr)
{
    int             retval = -1;
    char            buf[256] = {0,};
    int             buflen = sizeof(buf);
    int             len;
    char           *s0=NULL; /* whole string */
    int             s0len=0; /* length of whole string */
    char           *p;

    /* 
     *  The fork code executes iwgetid and returns a string
     */
    buflen = sizeof(buf);
    if ((len = fork_exec_read(buf, buflen, _iwgetprog, _device, "-r", NULL)) < 0){
	if (strlen(buf))
	    fprintf(stderr, "%s\n",buf);
	goto done;
    }
    if (len && stringadd(buf, 0, "iwessid", &s0, &s0len) < 0)
	goto done;

    if ((len = fork_exec_read(buf, buflen, _iwgetprog, _device, "-a", NULL)) < 0){
	fprintf(stderr, "%s\n", buf);
	goto done;
    }
    if (len && stringadd(rindex(buf, ' '), 1, "iwaddr", &s0, &s0len) < 0)
	goto done;

    if ((len = fork_exec_read(buf, buflen, _iwgetprog, _device, "-c", NULL)) < 0){
	fprintf(stderr, "%s\n", buf);
	goto done;
    }
    if (len && stringadd(rindex(buf, ':'), 1, "iwchan", &s0, &s0len) < 0)
	goto done;
    
    if ((len = fork_exec_read(buf, buflen, _iwgetprog, _device, "-f", NULL)) < 0){
	fprintf(stderr, "%s\n", buf);
	goto done;
    }
    /* Strange: sometimes ':', sometimes '=' */
    if ((p = rindex(buf, ':')) == NULL)
	p = rindex(buf, '=');
    if (len && stringadd(p, 1, "iwfreq", &s0, &s0len) < 0)
	goto done;
    if ((len =  fork_exec_read(buf, buflen, _iwgetprog, _device, "-p", NULL)) < 0){
	fprintf(stderr, "%s\n", buf);
	goto done;
    }
    if (len && stringadd(rindex(buf, ':'), 1, "iwproto", &s0, &s0len) < 0)
	goto done;
    *outstr = s0;
    retval = 0;
 done:
    return retval;
}

/*! Init grideye test module. Check if file exists that is used for polling state */
int 
iwget_setopt(const char *optname,
	     char       *value)
{
    int         retval = -1;
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
    if (stat(_iwgetprog, &st) < 0)
	return NULL;
    return (void*)&api;
}

#ifndef _NOMAIN
int main() 
{
    char   *str = NULL;

    if (grideye_plugin_init(2) == NULL)
	return -1;
    if (iwget_setopt("device", "wlan0") < 0)
	return -1;
    if (iwget_test(NULL, &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    iwget_exit();
    return 0;
}
#endif
