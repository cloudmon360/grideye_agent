#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/wait.h>

#include "grideye_plugin.h"

/*! Fork and exec a process and read output from stdout
 * @param[out]  buf    Buffer to store output from process
 * @param[in]   buflen Length of buffer
 * @param[in]   ...    Variable argument list
 * @retval -1          Error. Error string in buf
 * @retval  n          Number of bytes read
 * @note Assume that the output (if any) is terminated by a LF/CR, or some other
 * non-printable character which can be replaced with \0 and yield a string
 */
int fork_exec_read(char *buf, int buflen, ...)
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
    } else { /* child */
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

	if (argv)
		free(argv);
	
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
