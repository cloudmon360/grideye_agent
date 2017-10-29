/* 
 * 
 * compile:
 *   gcc -O2 -Wall -o grideye_airport grideye_airport.c -lm
 * run: 
 *   ./grideye_airport
Program att använda:
$ /System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I
    agrCtlRSSI: -44 - wlevel
    agrExtRSSI: 0
   agrCtlNoise: -92 - wnoise
   agrExtNoise: 0 
         state: running
       op mode: station
    lastTxRate: 878 - new plot
       maxRate: 217 - ~iwproto
lastAssocStatus: 0
   802.11 auth: open
     link auth: wpa2
         BSSID: 58:b6:33:79:3e:7c - iwaddr
          SSID: eduroam - iwessid?
           MCS: 7 - new plot
       channel: 124,80 - iwchan

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
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "grideye_plugin_v1.h"

static const char *_airport_prog = "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport";
static char *_device = NULL;

int 
airport_exit(void)
{
    if (_device){
	free(_device);
	_device = NULL;
    }
    return 0;
}

/*! Fork and exec a iwget process
 * @param[in]  prog   Program to exec, eg iwgetid
 * @param[in]  arg1   First argument, eg -I
 * @param[in]  buf    Buffer to store output from process
 * @param[in]  buflen Length of buffer
 * @retval -1         Error
 * @retval  n         Number of bytes read
 * @note Assume that the output (if any) is terminated by a LF/CR, or some other
 * non-printable character which can be replaced with \0 and yield a string
 */
static int 
airport_fork(const char *prog,
	     const char *arg1,
	     char       *buf,
	     int         buflen)
{
    int     retval = -1;
    int     stdin_pipe[2];
    int     stdout_pipe[2];
    int     pid;
    int     status;
    int     len;

    if (pipe(stdin_pipe) < 0){
	perror("pipe");
	goto done;
    }
    if (pipe(stdout_pipe) < 0){
	perror("pipe");
	goto done;
    }
    if ((pid = fork()) != 0){ /* parent */
	close(stdin_pipe[0]); 
	close(stdout_pipe[1]); 
    }
    else { /* child */
	close(0);
	if (dup(stdin_pipe[0]) < 0){
	    perror("dup");
	    return  -1;
	}
	close(stdin_pipe[0]); 
	close(stdin_pipe[1]); 
	close(1); 
	if (dup(stdout_pipe[1]) < 0){
	    perror("dup");
	    return  -1;
	}
	close(stdout_pipe[1]);  
	close(stdout_pipe[0]); 

	if (execl(prog, prog, 
		  arg1,
		  (void*)0) < 0){
	    perror("execl");
	    return -1;
	}
	exit(0);
    }
    if (pid < 0){
	perror("fork");
	goto done;
    }
    close(stdin_pipe[1]);
    if ((len = read(stdout_pipe[0], buf, buflen)) < 0){
	perror("read");
	goto done;
    }
    if (len>0)
	buf[len-1] = '\0';
    close(stdout_pipe[0]);
    waitpid(pid, &status, 0);
    retval = len;
 done:
    return retval;
}

/*! FInd a keyword in buf and write value after colon into string
 * @param[in]  buf    Input buffer
 * @param[out] argv  
 * @param[out] argc
 * Assume format: 
 * space* <key>: <value>
 */
static int
airport_buf2argv(char  *buf,
		 char ***argv,
		 int   *argc)
{
    int  retval = -1;
    char *p=buf;
    char *line;
    char *key;
    char *val;

    while ((line = strsep(&p, "\n")) != NULL){
      if ((val = index(line, ':')) != NULL){
	*val++ = '\0';
	/* trim spaces */
	while (strlen(val) && isblank(*val))
	  val++;
	key = line;
	while (strlen(key) && isblank(*key))
	  key++;
	*argc += 2;
	if ((*argv = realloc(*argv, (*argc)*sizeof(char*))) == NULL){
 	   perror("realloc");
	   goto done;
	}
	(*argv)[*argc-2] = key;
	(*argv)[*argc-1] = val;
      }
    }
    retval = 0;
 done:
    return retval;
}

char *
find_key(char **argv, 
	 int    argc, 
	 char  *key)
{
  for (;(argc>0)&& *argv; argc-=2, argv+=2){
      if (strcmp(key, argv[0])==0)
	return argv[1];
  }
    return NULL;
}

/*! Poll /proc wireless file for device status 
 * @param[out]  outstr  XML string with result parameters 
 * The string contains the following parameters (or possibly a subset):
 */
int  
airport_test(int     dummy,
	  char     **outstr)
{
    int         retval = -1;
    char        buf[1024];
    int         buflen = sizeof(buf);
    int         len;
    char      **argv=NULL;
    int         argc = 0;
    char       *str = NULL;
    int         slen;
    char       *wlevel;
    char       *wnoise;
    char       *wlastTxRate;
    char       *iwproto;
    char       *iwaddr;
    char       *iwessid;
    char       *wMCS;
    char       *iwchan;

    /* The fork code executes airport and returns a string
     */
    buflen = sizeof(buf);
    if ((len = airport_fork(_airport_prog, "-I", buf, buflen)) < 0)
	goto done;
    if (len==0)
      goto ok;
    if (airport_buf2argv(buf, &argv, &argc) < 0)
	goto done;
    wlevel = find_key(argv, argc, "agrCtlRSSI");
    wnoise = find_key(argv, argc, "agrCtlNoise");
    wlastTxRate = find_key(argv, argc, "lastTxRate");
    iwproto = find_key(argv, argc, "maxRate");
    iwaddr = find_key(argv, argc, "BSSID");
    iwessid = find_key(argv, argc, "SSID");
    wMCS = find_key(argv, argc, "MCS");
    iwchan = find_key(argv, argc, "channel");
    if ((slen = snprintf(NULL, 0, 
			 "<wlevel>%s</wlevel>"
			 "<wnoise>%s</wnoise>"
			 "<wlastTxRate>%s</wlastTxRate>"
			 "<iwproto>%s</iwproto>"
			 "<iwaddr>\"%s\"</iwaddr>"
			 "<iwessid>\"%s\"</iwessid>"
			 "<wMCS>%s</wMCS>"
			 "<iwchan>%s</iwchan>",
			 wlevel, wnoise, wlastTxRate,
			 iwproto, iwaddr, iwessid,
			 wMCS, iwchan)) <= 0)
       goto done;
    if ((str = malloc(slen+1)) == NULL){
       perror("malloc");
       goto done;
    }
    if ((slen = snprintf(str, slen+1, 
			 "<wlevel>%s</wlevel>"
			 "<wnoise>%s</wnoise>"
			 "<wlastTxRate>%s</wlastTxRate>"
			 "<iwproto>%s</iwproto>"
			 "<iwaddr>\"%s\"</iwaddr>"
			 "<iwessid>\"%s\"</iwessid>"
			 "<wMCS>%s</wMCS>"
			 "<iwchan>%s</iwchan>",
			 wlevel, wnoise, wlastTxRate,
			 iwproto, iwaddr, iwessid,
			 wMCS, iwchan)) <= 0)
       goto done;
    *outstr = str;
 ok:
    retval = 0;
 done:
    return retval;
}


static const struct grideye_plugin_api_v1 api = {
    1,
    GRIDEYE_PLUGIN_MAGIC,
    airport_exit,
    airport_test,
    NULL,       /* file callback */
    NULL,       /* input param */
    "xml"       /* output format */
};

/* Grideye agent plugin init function must be called grideye_plugin_init */
void *
grideye_plugin_init_v1(int version)
{
    struct stat st;

    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;
    if (stat(_airport_prog, &st) < 0)
	return NULL;
    return (void*)&api;
}

#ifndef _NOMAIN
int main() 
{
    char   *str = NULL;

    if (grideye_plugin_init_v1(1) < 0)
	return -1;
    if (airport_test(0, &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    airport_exit();
    return 0;
}
#endif
