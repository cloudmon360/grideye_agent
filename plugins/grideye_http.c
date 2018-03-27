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

#include "grideye_plugin_v2.h"

#define _PROGRAM "/usr/bin/curl"
#define _CURLARGS "%{http_code} %{size_download} %{time_total}"


/* Forward */
int http_test(char *instr, char **outstr);

/*
 * This is the API declaration
 */
static const struct grideye_plugin_api_v2 api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "http",
    "str",         /* input format */
    "xml",         /* output format */
    NULL,
    http_test,      /* actual test */
    NULL
};

/*! Fork and exec a process and read output from stdout
 * @param[out]  buf    Buffer to store output from process
 * @param[in]   buflen Length of buffer
 * @param[in]   ...    Variable argument list
 * @retval -1          Error. Error string in buf
 * @retval  n          Number of bytes read
 * @note Assume that the output (if any) is terminated by a LF/CR, or some other
 * non-printable character which can be replaced with \0 and yield a string
 */
static int 
fork_exec_read(char  *buf,
	       int    buflen,
	       ...
	       )
{
    int     retval = -1;
    int     stdin_pipe[2];
    int     stdout_pipe[2];
    int     pid;
    int     status;
    int     len;
    va_list ap;
    char   *s;
    int     argc;
    char  **argv;
    int     i;

    /* Translate from va_list to argv */
    va_start(ap, buflen);
    argc = 0;
    while ((s = va_arg(ap, char *)) != NULL)
	argc++;
    va_end(ap);
    va_start(ap, buflen);
    if ((argv = calloc(argc+1, sizeof(char*))) == NULL){
	perror("calloc");
	goto done;
    }
    for (i=0; i<argc; i++)
	argv[i] = va_arg(ap, char *);
    argv[i] = NULL;
    va_end(ap);
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
	if (execv(argv[0], argv) < 0){
	    fprintf(stderr, "execv %s: %s\n",  argv[0], strerror(errno));
	    exit(1);
	}
	exit(0); /* Not reached */
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
    if (waitpid(pid, &status, 0) < 0){
	perror("waitpid");
	goto done;
    }
    if (status != 0)
	goto done;
    retval = len;
 done:
    return retval;
}


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
    
    sscanf(buf, "%s %s %lf\n",  code0, &size, &time);
    
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
grideye_plugin_init_v2(int version)
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
    if (grideye_plugin_init_v2(2) == NULL)
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
