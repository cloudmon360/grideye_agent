/*! Sequential disk write
 * Provide a filename
 * For every test:
 *  - Create and open the file
 *  - Write 'iow' bytes to the file sequentially.
 *  - Close and unlink the file.
 * compile:
 *   gcc -O2 -Wall -o diskio_write diskio_write.c
 * run: 
 *   diskio_write GRIDEYE_WRITEFILE 1024
 *  Copyright (C) 2015-2016 Olof Hagsand
 */
#define _GNU_SOURCE /* This is to enable mkostemp */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/param.h>

#include "grideye_plugin.h"

/* Use global variables, could encapsulate in handle */
static char *_filename = NULL;

/* Forward */
int diskio_write_exit(void);
int diskio_write_test(int argc, char *argv[], char **outstr);
int diskio_write_setopt(const char *optname, char *value);

/*
 * This is the API declaration
 */
static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "diskio_write",
    "str",            /* input format */
    "xml",            /* output format */
    NULL,
    diskio_write_setopt,
    diskio_write_test,
    diskio_write_exit,
};


/*! IO write init code. Called when agent starts
 * @param[in]  filename  writeable file
 */
int
diskio_write_setopt(const char *optname,
		    char       *value)
{
    char    *writefile;

    if (strcmp(optname, "writefile"))
	return 0;
    writefile = value;
    if (writefile == NULL || !strlen(writefile)){
	errno = EINVAL;
	return -1;
    }
    if ((_filename = strdup(writefile)) == NULL)
	return -1;
    return 0;
}

int
diskio_write_exit(void)
{
    if (_filename)
	free(_filename);
    return 0;
}

/*
 * @param[in]   len   Nr of bytes to write to a file
 * @param[out]  t_us  Latency in micro-seconds
 */
int
diskio_write_test(int        argc,
		  char      *argv[],
		  char     **outstr)
{
    int            retval = -1;
    int            fd;
    char          *buf = NULL;
    struct timeval t0;
    struct timeval t1;
    struct timeval dt;  /* t1-t0 */
    uint64_t       t_us;
    char          *str = NULL;
    size_t         slen;
    int            len = 0;

    if (argc > 0)
	len = atoi(argv[0]);
    if (len == 0){
	retval = 0;
	goto done;
    }
    if ((buf = malloc(len)) == NULL)
	goto done;
    gettimeofday(&t0, NULL);
    if ((fd = open(_filename, O_SYNC|O_CREAT|O_WRONLY,S_IWUSR)) < 0){
	fprintf(stderr, "open(%s) %s\n", _filename,  strerror(errno));
	goto done;
    }
    memset(buf, 0, len);
    if (write(fd, buf, len) < 0){ 
	close(fd);
	unlink(_filename);
	goto done;
    }
    close(fd);
    unlink(_filename);
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &dt);
    t_us = dt.tv_usec+dt.tv_sec*1000000;
    if ((slen = snprintf(NULL, 0, "<tiow>%" PRIu64 "</tiow>", t_us)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1, "<tiow>%" PRIu64 "</tiow>", t_us);
    *outstr = str;
    retval = 0;
 done:
    if (buf)
	free(buf);
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

int 
main(int   argc, 
     char *argv[])
{
    char   *f;
    char   *str = NULL;

    if (argc != 3){
	fprintf(stderr, "usage %s <file> <bytes>\n", argv[0]);
	return -1;
    }
    f = argv[1];
    if (grideye_plugin_init(2) == NULL)
	return -1;
    if (diskio_write_setopt("writefile", f) < 0)
	return -1;
    if (diskio_write_test(argv[2], &str) < 0)
	return -1;
    fprintf(stdout, "%s\n", str);
    free(str);
    diskio_write_exit();
    return 0;
}
#endif
