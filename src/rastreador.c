#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/ptrace.h>
#include <bits/types.h> 
#include <sys/user.h> 
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "rastreador.h"
#include "utils.h"

int wait_for_syscall(pid_t child) {
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        if (WIFEXITED(status))
            return 1;
       fprintf(stderr, "[stopped %d (%x)]\n", status, WSTOPSIG(status));
    }
}

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

long get_request(enum __ptrace_request request, pid_t child, int offset) {
    long syscall = ptrace(request, child, offset);
    assert(errno == 0);
    return syscall;
}


char *read_string(pid_t child, long addr) {
    char *val;
     int i;
    long tmp;

    while (i < 8) {
        
        if ((tmp = ptrace(PTRACE_PEEKDATA, child, addr)) == -1)break;
        i++;
        memcpy(val, &tmp, sizeof tmp);
        val += sizeof (tmp);
    }
    return val;
}


struct syscall_info get_syscall_reg(enum __ptrace_request request,pid_t child){
	int nargs = -1;
	char *syscall_name = NULL;
	struct syscall_info sys;
	  
	nargs = get_request(request,child,sizeof(long)*ORIG_RAX);
    assert(errno == 0);
    syscall_name = read_string(child, nargs);

	sys.nargs = nargs;
	sys.name = syscall_name;
	//if( syscall <= MAX_SYSCALL_NUM) {
      //  pause_on( num, syscall);
    //}
	
	return sys;
}

void print_syscall_(struct syscall_info syscall_reg) {

    fprintf(stderr, "syscall(%d) syscall_information(%s) ", syscall_reg.nargs,syscall_reg.name);
    
}

/*
Make child continue to only stop    a SysCall
* then waits for the child
*/ 
int do_trace(pid_t child){
	int status,retval;
	struct syscall_info syscall_reg;
	waitpid(child, &status, 0);
	ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
	while(1) {
		if (wait_for_syscall(child) != 0) break;

		syscall_reg = get_syscall_reg(PTRACE_PEEKUSER,child);
		print_syscall_(syscall_reg);
		
		if (wait_for_syscall(child) != 0) break;

		retval = get_request(PTRACE_PEEKUSER,child,sizeof(long)*RAX);
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
