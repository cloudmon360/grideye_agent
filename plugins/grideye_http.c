/* 
 * 
 * compile:
 *   gcc -O2 -Wall -o grideye_http grideye_http.c
 * run: 
 *   ./grideye_http www.youtube.com
 */
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "grideye_plugin.h"

#define _PROGRAM "/usr/bin/curl"
#define _CURLARGS "%{http_code} %{size_download} %{time_total}"


/* Forward */
int http_test(char *instr, char **outstr);

/*
 * This is the API declaration
 */
static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "http",
    "str",         /* input format */
    "xml",         /* output format */
    NULL,
    http_test,      /* actual test */
    NULL
};

/*! Run Nagios check_http plugin
 * @param[out]  outstr  XML string with three parameters described below

HTTP OK: HTTP/1.1 200 OK - 518853 bytes in 1.418 second response time |time=1.417916s;;;0.000000 size=518853B;;;0

 */
int  
http_test(char      *instr,
	  char     **outstr)
{
    int    retval = -1;
    char   buf[1024] = {0,};
    int    buflen = sizeof(buf);
    char   code0[64];
    int    size;
    double time;
    char  *str = NULL;
    size_t slen;
    char   *host = instr;
    
    if (fork_exec_read(buf, buflen, _PROGRAM, "-s", "-o", "/dev/null", "-w", _CURLARGS, host, NULL) < 0){
        if (strlen(buf))
            fprintf(stderr, "%s\n", buf);
        goto done;
    }
    
    sscanf(buf, "%s %d %lf\n",  code0, &size, &time);
    
    if ((slen = snprintf(NULL, 0, 
			 "<hstatus>\"%s\"</hstatus>"
			 "<htime>%d</htime>"
			 "<hsize>%d</hsize>",
			 code0,
			 (int)(time*1000),
			 size)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    if ((slen = snprintf(str, slen+1, 
			 "<hstatus>\"%s\"</hstatus>"
			 "<htime>%d</htime>"
			 "<hsize>%d</hsize>",
			 code0,
			 (int)(time*1000),
			 size)) <= 0)
	goto done;
    *outstr = str;
    retval = 0;
 done:
    return retval;
}

/* Grideye agent plugin init function must be called grideye_plugin_init 
 */
void *
grideye_plugin_init(int version)
{
    struct stat st;
    
    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;
    if (stat(_PROGRAM, &st) < 0) {
	fprintf(stderr, "stat(%s): %s\n", _PROGRAM, strerror(errno));
	return NULL;
    }
    return (void*)&api;
}


#ifndef _NOMAIN
int main(int   argc, 
	 char *argv[]) 
{
    char   *str = NULL;
    
    if (argc != 2){
	fprintf(stderr, "usage %s <host>\n", argv[0]);
	return -1;
    }
    if (grideye_plugin_init(2) == NULL)
	return -1;
    if (http_test(argv[1], &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    return 0;
}
#endif
