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
#include <iperf_api.h> 

#include <cligen/cligen.h>
#include <clixon/clixon.h>

#include "grideye_plugin.h"

int
iperf_getopt(const char *optname, char **value);
int iperf_client_test(int argc, char *argv[], char **outstr);
int iperf_results_parse(char *result, float *sent_bytes, float *sent_bps,
			float *recv_bytes,
			float *recv_bps);

static const struct grideye_plugin_api api = {
    2,
    GRIDEYE_PLUGIN_MAGIC,
    "http",
    "str",         /* input format */
    "xml",         /* output format */
    iperf_getopt,
    NULL,
    iperf_client_test,      /* actual test */
    NULL
};

int
iperf_getopt(const char *optname, 
	       char      **value)
{
    if (strcmp(optname, "yangmetric"))
	return 0;
    if ((*value = strdup("{\"metrics\":"
			 "{\"name\":\"iperf_sent_bytes\",\"description\":\"iperf bytes sent\", \"type\":\"decimal64-6\",\"units\":\"bytes\"},"
			 "{\"name\":\"iperf_sent_bps\",\"description\":\"iperf bits per second sent\", \"type\":\"decimal64-6\",\"units\":\"bytes\"},"
			 "{\"name\":\"iperf_recv_bytes\",\"description\":\"iperf bytes recv\", \"type\":\"decimal64-6\",\"units\":\"bytes\"},"
			 "{\"name\":\"iperf_recv_bps\",\"description\":\"iperf bits per second recv\", \"type\":\"decimal64-6\",\"units\":\"bytes\"}"
			 "}")) == NULL)
	return -1;
    return 0;
}

int iperf_results_parse(char *result, float *sent_bytes, float *sent_bps,
			float *recv_bytes,
			float *recv_bps)
{
    cxobj *x = NULL;
    cxobj *xmltree = NULL;
    
    if (json_parse_str(result, &xmltree) < 0) {
	fprintf(stderr, "Failed to parse json output\n");
	return -1;
    }

    if ((x = xpath_first(xmltree, "//end/sum_sent/bits_per_second")) == NULL) {
	fprintf(stderr, "Could not parse XML output\n");
	return -1;
    }

    *sent_bps = atof(xml_body(x));
    
    if ((x = xpath_first(xmltree, "//end/sum_sent/bytes")) == NULL) {
	fprintf(stderr, "Could not parse XML output\n");
	return -1;
    }

    *sent_bytes = atof(xml_body(x));

    if ((x = xpath_first(xmltree, "//end/sum_received/bits_per_second")) == NULL) {
	fprintf(stderr, "Could not parse XML output\n");
	return -1;
    }

    *recv_bps = atof(xml_body(x));
    
    if ((x = xpath_first(xmltree, "//end/sum_received/bytes")) == NULL) {
	fprintf(stderr, "Could not parse XML output\n");
	return -1;
    }

    *recv_bytes = atof(xml_body(x));

    return 0;
}

int iperf_client_test(int argc, char *argv[], char **outstr)
{
    int i = 0;
    int j = 0;
    int slen = 0;
    int delimiters = 0;
    int port = 0;
    int duration = 0;
    // int tos = 0;
    int retval = 0;
	
    uint64_t rate = 0;    

    char *key = NULL;
    char *value = NULL;
    char *result = NULL;
    char *host = NULL;
    char *str = NULL;

    float sent_bytes = 0;
    float sent_bps = 0;
    float recv_bytes = 0;
    float recv_bps = 0;

    struct iperf_test *test;
    
    for (i = 1; i <= argc; i++) {
	delimiters = 0;
	
	if (argv[i] == NULL)
	    continue;
	for (j = 0; j < strlen(argv[i]); j++) {
	    if (argv[i][j] == '=')
		delimiters++;
	}

	if (delimiters != 1) {
	    fprintf(stderr, "Malformed configuration string\n");
	    goto fail;
	}
	
	key = strtok(argv[i], "=");
	value = strtok(NULL, "=");

	if (strcmp(key, "host") == 0)
	    host = strdup(value);
	else if (strcmp(key, "port") == 0)
	    port = atoi(value);
	else if (strcmp(key, "rate") == 0)
	    rate = atoi(value);
	else if (strcmp(key, "duration") == 0)
	    duration = atoi(value);
	/* else if (strcmp(key, "tos")) */
	/*     tos = atoi(value); */
	else
	    continue;
    }

    if (host == NULL)
	goto fail;
    if (port == 0)
	port = 5201;
    
    if ((test = iperf_new_test()) == NULL) {
	fprintf(stderr, "Could not create iperf client\n");
	goto fail;
    }

    iperf_defaults(test);
    iperf_set_test_role(test, 'c');
    iperf_set_test_json_output(test, 1);
    iperf_set_test_server_hostname(test, host);
    iperf_set_test_server_port(test, port);

    if (rate != 0)
	iperf_set_test_rate(test, rate);
    if (duration != 0)
	iperf_set_test_duration(test, duration);
    /* if (tos != 0) { */
    /* 	if (tos < 0 || tos > 255) */
    /* 	    goto fail; */
    /* 	iperf_set_test_tos(test, tos); */
    /* } */
    
    if (iperf_run_client(test) < 0) {
        fprintf(stderr, "error - %s\n", iperf_strerror(i_errno));
        exit(EXIT_FAILURE);
    }

    result = iperf_get_test_json_output_string(test);
    iperf_free_test(test);

    iperf_results_parse(result, &sent_bytes, &sent_bps, &recv_bytes,
			&recv_bps);
    
    if ((slen = snprintf(NULL, 0,
			 "<iperf_sent_bytes>%f</iperf_sent_bytes>"	\
			 "<iperf_sent_bps>%f</iperf_sent_bps>" \
			 "<iperf_recv_bytes>%f</iperf_recv_bytes>" \
			 "<iperf_recv_bps>%f</iperf_recv_bps>",
			 sent_bytes, sent_bps, recv_bytes, recv_bps)) <= 0)
	goto fail;

    if ((str = malloc(slen + 1)) == NULL)
	goto fail;
    
    if ((slen = snprintf(str, slen + 1,
			 "<iperf_sent_bytes>%f</iperf_sent_bytes>"	\
			 "<iperf_sent_bps>%f</iperf_sent_bps>" \
			 "<iperf_recv_bytes>%f</iperf_recv_bytes>" \
			 "<iperf_recv_bps>%f</iperf_recv_bps>",
			 sent_bytes, sent_bps, recv_bytes, recv_bps)) <= 0)
	goto done;
    
    *outstr = str;
    
 done:
    return retval;
    
 fail:
    retval = -1;
    goto done;
}

void *grideye_plugin_init(int version)
{
    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;

    return (void*)&api;
}

#ifndef _NOMAIN
int main(int   argc, 
	 char *argv[]) 
{
    char   *str = NULL;
    
    if (argc < 2){
	fprintf(stderr, "usage %s <args>\n", argv[0]);
	return -1;
    }
    if (grideye_plugin_init(2) == NULL)
	return -1;
    if (iperf_client_test(argc, argv, &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    return 0;
}
#endif
