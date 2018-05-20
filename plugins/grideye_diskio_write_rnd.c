/*! Random disk write
 * Precondition: a large file, typically 2*ram size
 * For every test:
 *  - Open the existing file.
 *  - Write 'iow' bytes to a random position using lseek to the file.
 *  - Close the file.
 * compile:
 *   gcc -O2 -Wall -o diskio_write_rnd diskio_write_rnd.c
 * run: 
 *   /bin/dd if=/dev/zero of=GRIDEYE_LARGEFILE bs=1M count=1K # optional
 *   diskio_write_rnd GRIDEYE_LARGEFILE 1024
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
static char    *_filename = NULL;
static int64_t _filesize = 0;

/* Forward */
int diskio_write_rnd_exit(void);
int diskio_write_rnd_test(int argc, char *argv[], char **outstr);
int diskio_write_rnd_setopt(const char *optname, char *value);

/*
 * This is the API declaration
 */
static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "diskio_write_rnd",
    "str",            /* input format */
    "xml",            /* output format */
    NULL,
    diskio_write_rnd_setopt,
    diskio_write_rnd_test,
    diskio_write_rnd_exit
};



int
diskio_write_rnd_exit(void)
{
    if (_filename)
	free(_filename);
    return 0;
}

/*
 * @param[in]   len   Nr of bytes to write to a file
 * @param[out]  str   XML result string
 */
int
diskio_write_rnd_test(int    argc,
		      char  *argv[],
		      char **outstr)
{
    int      retval = -1;
    int      fd;
    char    *buf = NULL;
    struct timeval t0;
    struct timeval t1;
    struct timeval dt;  /* t1-t0 */
    off_t    off;
    int      n;
    int      i; 
    uint64_t t_us;
    char    *str = NULL;
    size_t   slen;
    int      len = 0;

    if (argc > 0)
	len = atoi(argv[0]);
    if (len == 0){
	retval = 0;
	goto done;
    }
    if (len > _filesize)
	len = _filesize;
    if ((buf = malloc(len)) == NULL){
	retval = -2;
	goto done;
    }
    gettimeofday(&t0, NULL);
    if ((fd = open(_filename, O_SYNC|O_WRONLY)) < 0){
	fprintf(stderr, "open(%s) %s\n", _filename,  strerror(errno));
	retval = -3;
	goto done;
    }
    n = _filesize/len;
    i = random()%n;
    off = i*len;
    if (lseek(fd, off, SEEK_SET) < 0){
	fprintf(stderr, "lseek(%s) %s\n", _filename,  strerror(errno));
	retval = -4;
	goto done;
    }
    memset(buf, 0, len);
    if (write(fd, buf, len) < 0){ 
	fprintf(stderr, "write(%s) %s\n", _filename,  strerror(errno));
	retval = -5;
	close(fd);
	goto done;
    }
    close(fd);
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &dt);
    t_us = dt.tv_usec+dt.tv_sec*1000000;
    if ((slen = snprintf(NULL, 0, "<tiowr>%" PRIu64 "</tiowr>", t_us)) <= 0){
	retval = -6;
	goto done;
    }
    if ((str = malloc(slen+1)) == NULL){
	retval = -7;
	goto done;
    }
    snprintf(str, slen+1, "<tiowr>%" PRIu64 "</tiowr>", t_us);
    *outstr = str;

    retval = 0;
 done:
    if (buf)
	free(buf);
    return retval;
}

/*! IO write init code. Called when agent starts
 * @param[in]  template  filename on the form dictaed by mkstmp(): with XXXXXX
 */
int
diskio_write_rnd_setopt(const char *optname,
			char       *value)
{
    int   retval = -1;
    int   fd = -1;
    char *largefile;
    
    if (strcmp(optname, "largefile"))
	return 0;
    largefile = value;
    if (largefile == NULL || !strlen(largefile)){
	errno = EINVAL;
	return -1;
    }
    if ((_filename = strdup(largefile)) == NULL)
	return -1;
    if ((fd = open(_filename, O_RDONLY)) < 0)
	goto done;
    if ((_filesize = lseek(fd, 0, SEEK_END)) < 0)
	goto done;
    retval = 0;
 done:
    if (fd != -1)
	close(fd);
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
    if (diskio_write_rnd_setopt("largefile", f) < 0)
	return -1;
    if (diskio_write_rnd_test(argv[2], &str) < 0)
	return -1;
    fprintf(stdout, "%s\n", str);
    free(str);
    diskio_write_rnd_exit();
    return 0;
}
#endif
