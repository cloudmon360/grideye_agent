/* Code thanks to Torbjörn Granlund tg@gmplib.org
 * 
 * This is a basic memory read test: Create a large chunk of memory,
 * the read from that memory from random locations and measure latency
 *
 * compile:
 *   gcc -O2 -Wall -o mem_read mem_read.c mem_read_test.c
 * run: 
 *   ./mem_read 
 */
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "grideye_plugin.h"

/*
 * Constants
 */
#define LOOPCOUNT 1000000

static int debug = 0;

/* Compute a 2^log that is highest number not more than 50% of memory */
static int _log = 0;

/* Forward */
int mem_read_test(char *instr, char **outstr);


static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "mem_read",
    NULL,            /* input format */
    "xml",            /* output format */
    NULL,
    NULL,
    mem_read_test,
    NULL
};

long mem_read_test1(int logsize);

int  
mem_read_test(char    *instr, 
	      char   **outstr)
{
    int             retval = -1;
    struct timeval  t0;
    struct timeval  t1;
    struct timeval  dt;  /* t1-t0 */
    uint64_t        t_us;
    char           *str = NULL;
    size_t          slen;

    if (debug){
	if ((1L<<_log) > LOOPCOUNT)
	    fprintf(stderr, "size:%luM\n", (1L<<_log)/LOOPCOUNT);
    }
    gettimeofday(&t0, NULL);
    if (mem_read_test1(_log) < 0)
	goto done;
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &dt);
    t_us = dt.tv_usec+dt.tv_sec*1000000;
    t_us = t_us*250/LOOPCOUNT; /* Bring it down to 1K accesses (each loop x 4) */
    if ((slen = snprintf(NULL, 0, "<tmr>%" PRIu64 "</tmr>", t_us)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1, "<tmr>%" PRIu64 "</tmr>", t_us);
    *outstr = str;
    retval = 0;
 done:
    return retval;
}


/* Grideye agent plugin init function must be called grideye_plugin_init */
void *
grideye_plugin_init(int version)
{
    long     page_size;
    long     num_pages;
    uint64_t ram;
    int      i;

    if (version != GRIDEYE_PLUGIN_VERSION)
	goto done;
    if ((page_size = sysconf(_SC_PAGESIZE)) < 0)
	goto done;
    if ((num_pages = sysconf(_SC_PHYS_PAGES)) < 0)
	goto done;
    ram = page_size;
    ram *= num_pages;
    /* Find ram size between 25-50% of total ram */
    for (i=40;i>0;i--){
	if ((1LL<<i) < ram)
	    break;
    }
    _log = i-1; /* divide by 2 */
    if (debug)
	fprintf(stderr, "ram:%" PRIu64 " i:%d %lu\n", 
		ram, _log, (1L<<_log));
    return (void*)&api;
 done:
    return NULL;
}

#ifndef _NOMAIN
int main() 
{
    char   *str = NULL;

    if (grideye_plugin_init(2) == NULL)
	return -1;
    if (mem_read_test(0, &str) < 0)
	return -1;
    fprintf(stdout, "%s\n", str);
    free(str);
    if (mem_read_exit() < 0)
	return -1;
    return 0;
}
#endif
