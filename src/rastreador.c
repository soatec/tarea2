#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <stdbool.h>
#include "rastreador.h"

// Structures

/*
 *
 */
typedef struct system_call_data {
    int system_call_number;
    const char *system_call_name;
} system_call_data;

// Private functions

/*
 *
 */
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
 * Declare that wants to be traced and then stops
 */
int do_child(char **argv){
    printf("Corriendo el programa %s:\n", argv[0]);
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(argv[0], argv);
}

/*
 *
 */
int get_syscall_reg(system_call_data *syscall_reg, enum __ptrace_request request, pid_t child){
	int system_call_numer = -1;
	char *syscall_name = NULL;

    system_call_numer = ptrace(request, child, sizeof(long) * ORIG_RAX);
    if (system_call_numer == -1) {
        fprintf(stderr, "Error ejecutando fork. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }

    syscall_name = "Nombre de system call";
    syscall_reg->system_call_number = system_call_numer;
    syscall_reg->system_call_name = syscall_name;
	return EXIT_SUCCESS;
}

/*
 *
 */
void print_syscall_(system_call_data syscall_reg) {
    fprintf(stderr, "Syscall (%d): %s\n",
            syscall_reg.system_call_number, syscall_reg.system_call_name);
}

/*
 * Make child continue to only stop    a SysCall
 * then waits for the child
 */
int do_trace(pid_t child){
	int status;
	int ptrace_requested_data;
	system_call_data syscall_reg;
	waitpid(child, &status, 0);
	ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
	while(1) {
		if (wait_for_syscall(child) != 0){
            break;
		}
        status = get_syscall_reg(&syscall_reg, PTRACE_PEEKUSER, child);
        if (status != EXIT_SUCCESS){
            return  status;
        }
		print_syscall_(syscall_reg);
		if (wait_for_syscall(child) != 0) {
		    break;
		}
        ptrace_requested_data = ptrace(PTRACE_PEEKUSER, child, sizeof(long) * RAX);
        if (ptrace_requested_data == -1) {
            fprintf(stderr, "Error ejecutando ptrace. (Errno %d: %s)\n",
                    errno, strerror(errno));
            return EXIT_FAILURE;
        }
	}
	return EXIT_SUCCESS;
}

// Public functions

int system_call_tracer_execute(char **argv, bool pause){
	pid_t fork_return = fork();
    if (fork_return == 0) {
        return do_child(argv);
    } else if (fork_return == -1) {
        fprintf(stderr, "Error ejecutando fork. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    } else {
        return do_trace(fork_return);
    }
}
