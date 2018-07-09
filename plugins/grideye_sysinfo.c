/* 
 * 
 * compile:
 *   gcc -O2 -Wall -DHAVE_SYS_SYSINFO_H=1 -o grideye_sysinfo grideye_sysinfo.c
 * run: 
 *   ./grideye_sysinfo
 */
#ifdef HAVE_SYS_SYSINFO_H
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/sysinfo.h>

#include "grideye_plugin.h"

#define LINUX_SYSINFO_LOADS_SCALE (65536)
#define PERCENT (100)
#define DEC_3 (1000)
#define BUF_MAX 1024

/* Forward */
int sysinfo_test(int argc, char *argv[], char **outstr);
int sysinfo_getopt(const char *optname, char **value);

/*
 * This is the API declaration
 */
static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "sysinfo",
    "json",        /* input format */
    "xml",         /* output format */
    sysinfo_getopt,/* getopt yangmetrics */
    NULL,
    sysinfo_test,  /* actual test */
    NULL
};

/*! Get extra yang metrics (not in main grideye yang file). 
 * Called when agent starts
 * @param[in]  optname "yangmetric", if not ignore
 * @param[out] value   Yang metric definition. Malloced. 
 */
int
sysinfo_getopt(const char *optname, 
	       char      **value)
{
    if (strcmp(optname, "yangmetric"))
	return 0;
    if ((*value = strdup("{\"metrics\":{\"name\":\"loadcore\",\"description\":\"CPU core load\", \"type\":\"decimal64-6\",\"units\":\"percent\"}}")) == NULL)
	return -1;
    return 0;
}

/*
 * Get the CPU we're executing on
 */
int
get_cpu(pid_t pid)
{
    int cpu = -1;
    char pidstat[128];

    FILE *fd;
    char buffer[BUF_MAX];
    int retval = 0;
    
    sprintf(pidstat, "/proc/%d/stat", pid);

    if ((fd = fopen(pidstat, "r")) == NULL)
	return -1;
    if (!fgets (buffer, BUF_MAX, fd)) {
	perror ("Error"); }
    if (sscanf (buffer, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s" \
		"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s" \
		"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %d", &cpu) == -1) {
	    retval = -1;
	    goto done;
    }

    retval = cpu;
    
done:
    fclose(fd);

    return retval;
}

/*
 * Get CPU load in percent
 */
float
get_cpu_load(int cpu)
{
    FILE *fp;
    char buffer[1024];
    char command[128];
    float usr = 0;
    float guest = 0;

    if (cpu == -1) {
	sprintf(command, "mpstat 1 1");
    } else {
	sprintf(command, "mpstat -P %d 1 1", cpu);
    }

    if (access("/usr/bin/mpstat", 0) == -1)
      return -1;

    if ((fp = popen(command, "r")) == NULL) {
	fprintf(stderr, "Failed to run command mpstat\n" );
	return -1;
    }

    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
	    if (cpu != -1)
		    sscanf(buffer, "Average:\t%*d\t%f\t%*f\t%*f\t%*f\t%*f\t%*f\t%*f\t%f", &usr, &guest);
	    else
		    sscanf(buffer, "Average:\tall\t%f\t%*f\t%*f\t%*f\t%*f\t%*f\t%*f\t%f", &usr, &guest);
    }

    pclose(fp);

    if (guest > usr)
	    return guest;
    return usr;
}

/*! Poll sysinfo command for system status
 * @param[out]  outstr  XML string with three parameters described below
 * The string contains the following parameters (or possibly a subset):
 * iwessid
 * iwaddr
 * iwchan
 * iwfreq
 * iwproto
 */
int
sysinfo_test(int    argc,
	     char  *argv[],
	     char **outstr)
{
    int             retval = -1;
    struct sysinfo  info;
    unsigned long   uptime;
    double          load_core = 0;
    unsigned long   loads;
    unsigned long   freeram;
    unsigned long   usedram;
    unsigned long   bufferram;
    unsigned long   procs;
    unsigned long   freeswap;
    unsigned long   usedswap;
    int             memunit;
    char            *str = NULL;
    size_t          slen;
    pid_t           pid;

    if (sysinfo(&info) < 0){
	perror("sysinfo");
	goto done;
    }

    memunit = info.mem_unit;
    uptime = info.uptime;
    pid = getpid();

    loads = ((info.loads[0]*PERCENT*DEC_3)/LINUX_SYSINFO_LOADS_SCALE);
    load_core = get_cpu_load(get_cpu(pid));

    if (load_core == -1)
      load_core = 0.0;

    freeram = info.freeram*memunit;
    usedram = info.totalram*memunit - info.freeram*memunit;
    bufferram = info.bufferram*memunit;
    procs = info.procs;
    freeswap = info.freeswap*memunit;
    usedswap = info.totalswap*memunit - info.freeswap*memunit;

    if ((slen = snprintf(NULL, 0,
			 "<uptime>%lu</uptime>"
			 "<loads>%lu.%lu</loads>"
			 "<loadcore>%f</loadcore>"
			 "<freeram>%lu</freeram>"
			 "<usedram>%lu</usedram>"
			 "<bufferram>%lu</bufferram>"
			 "<procs>%lu</procs>"
			 "<freeswap>%lu</freeswap>"
			 "<usedswap>%lu</usedswap>",
			 uptime,
			 loads/DEC_3, loads%DEC_3,
			 load_core,
			 freeram,
			 usedram,
			 bufferram,
			 procs,
			 freeswap,
			 usedswap)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1,
	     "<uptime>%lu</uptime>"
	     "<loads>%lu.%lu</loads>"
	     "<loadcore>%f</loadcore>"
	     "<freeram>%lu</freeram>"
	     "<usedram>%lu</usedram>"
	     "<bufferram>%lu</bufferram>"
	     "<procs>%lu</procs>"
	     "<freeswap>%lu</freeswap>"
	     "<usedswap>%lu</usedswap>",
	     uptime,
	     loads/DEC_3, loads%DEC_3,
	     load_core,
	     freeram,
	     usedram,
	     bufferram,
	     procs,
	     freeswap,
	     usedswap);
    *outstr = str;
    retval = 0;
 done:
    return retval;
}

/* Grideye agent plugin init function must be called grideye_plugin_init */
void *
grideye_plugin_init(int version)
{
    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;
    return (void*)&api;
}



#ifndef _NOMAIN
int main(int argc, char **argv)
{
    char   *str = NULL;

    if (grideye_plugin_init(2) == NULL)
	return -1;

    if (sysinfo_test(NULL, &str) < 0)
	    return -1;

    if (str) {
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    return 0;
}
#endif
#endif /* HAVE_SYS_SYSINFO_H */
