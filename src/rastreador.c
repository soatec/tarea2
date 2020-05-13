#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/ptrace.h> 
#include <sys/user.h> 
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "rastreador.h"
#include "utils.h"

/*
Declare that wants to be traced and then stops
*/
int do_child(int argc, char **argv){

	char *args [argc+1];
    memcpy(args, argv, argc * sizeof(char*));
    args[argc] = NULL;
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
}

/*
Make child continue to only stop when a SysCall
* then waits for the child
*/ 
int do_trace(pid_t child){
	int status, syscall, retval;
	struct user_regs_struct regs;
	waitpid(child, &status, 0);
	ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
	while(1) {
		if (wait_for_syscall(child) != 0) break;

		syscall = ptrace(PTRACE_PEEKUSER, child, sizeof(long)*sizeof(regs));
		fprintf(stderr, "syscall(%d) = ", syscall);

		if (wait_for_syscall(child) != 0) break;

		retval = ptrace(PTRACE_PEEKUSER, child, sizeof(long)*sizeof(regs));
		fprintf(stderr, "%d\n", retval);
	}
	return 0;
}

/*
 * 
 * to create two processes 
 * child executes the program to be traced. 
 * father traces it.
 * 
 */
int p_trace_request(int argc, char **argv){
	
	pid_t child = fork();
    if (child == 0) {
        return do_child(argc, argv);
    } else {
        return do_trace(child);
    }
}
