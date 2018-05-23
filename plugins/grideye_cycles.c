/* Code thanks to Torbjörn Granlund tg@gmplib.org
 *
 * This is a basic BogoMips test running a specified number of empty loops
 *
 * compile:
 *   gcc -O2 -Wall -o cycles cycles.c cycles_test.s
 * run: 
 *   ./cycles
 */
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "grideye_plugin.h"

/* warmup loop counter, see init function */
int _warmup=0;

/* Forward */
int cycles_test(char *instr, char **outstr);

/*
 * This is the API declaration
 */
static const struct grideye_plugin_api api = {
    2,       /* version */
    GRIDEYE_PLUGIN_MAGIC,
    "cycles",
    NULL,          /* input format */
    "xml",         /* output format */
    NULL,        
    NULL,        
    (grideye_plugin_test_t *)cycles_test,   /* actual test */
    NULL
};

int 
cycles_exit(void)
{
  return 0;
}

void cycles_test1(void);

static int 
cycles_test2(uint64_t *t_us)
{
    struct timespec ts;
    uint64_t        t0;
    uint64_t        t1;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    t0 = 1000000000ul * ts.tv_sec + ts.tv_nsec;

    cycles_test1();

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    t1 = 1000000000ul * ts.tv_sec + ts.tv_nsec;

    // 1000000000 comes from the loop count in foo
    // The result is billions as the time in ns is inverted here.
    //  fprintf(stderr, "%.2f billion insns/sec\n", (double) 13 * 1000000000 / (t1 - t0));
    //  fprintf(stderr, "%.2f billion insns/sec\n", (double) 13 * 10000000 / (t1 - t0));
    *t_us = (t1-t0)/1000;
    return 0;
}

int 
cycles_test(char  *instr, 
	    char **outstr)
{
    int             retval = -1;
    uint64_t        t_us;
    char           *str = NULL;
    size_t          slen;
    int             j;

    for (j=0;j<_warmup;j++)
    	cycles_test1();
    if (cycles_test2(&t_us) < 0)
	goto done;
    if ((slen = snprintf(NULL, 0, "<tcyc>%" PRIu64 "</tcyc>", t_us)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1, "<tcyc>%" PRIu64 "</tcyc>", t_us);
    *outstr = str;
    retval = 0;
 done:
    return retval;
}


/* Grideye agent plugin init function must be called grideye_plugin_init 
 * The init function contains an adaptive init function.
 * Some processors (eg Intel Haswell) needs to be warmed up before test
 * The init function runs a number of tests and notes when the diff is 
 * bounded, eg < x%.
 * For example: 17631,14708,12871, then run more than 3 warmup tests
*/
void *
grideye_plugin_init(int version)
{
    int      i;
    uint64_t t, tp = 0;
    int      dt;

    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;
    for (i=0; i<25; i++){
	cycles_test2(&t);
	if (i==0){
	    tp = t;
	    continue;
	}
	dt = (int)((100*(tp-t))/t);
	if (dt<2)
	    break;
	tp = t;
    }
    _warmup = i-1;
    return (void*)&api;
}


#ifndef _NOMAIN
int main() 
{
    char   *str = NULL;

    if (grideye_plugin_init(2) == NULL)
	return -1;
    fprintf(stdout, "warmup: %d\n", _warmup);
    if (cycles_test(NULL, &str) < 0)
	return -1;
    fprintf(stdout, "%s\n", str);
    free(str);
    if (cycles_exit() < 0)
	return -1;
    return 0;
}
#endif
