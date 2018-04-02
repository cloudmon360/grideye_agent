/* Code thanks to Torbjörn Granlund tg@gmplib.org
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Constants
 */
#define LOOPCOUNT 1000000

long 
mem_read_test1(int logsize) 
{
    size_t         size;
    unsigned char *p;
    long           s = 0;
    long           i = 0;
    long           c;

    size = (size_t) 1 << logsize;
    if ((p = malloc (size + 65536)) == NULL)
	return -1;
    s = 0;
    i = 0;
    for (c = 0; c < LOOPCOUNT; c++){
	i = (i * 46099 + 20160308893) & (size - 1);
	s += p[i];
	s += p[i + 8192];
	s += p[i + 32768];
	s += p[i + 65536];
    }
    free(p);
    return s;
}
