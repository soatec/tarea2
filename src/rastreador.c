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
 * Hold system call information
 */
typedef struct system_call_data {
    int system_call_number;
    int system_call_value_returned;
    const char *system_call_name;
} system_call_data;

// Private functions

// Functions for child process
/*
 * Execute Prog and indicates that wants to be traced and stops
 */
int execute_child_process(char **argv){
    int err_status;
    printf("Corriendo el programa %s:\n", argv[0]);
    err_status = ptrace(PTRACE_TRACEME);
    if (err_status == -1) {
        fprintf(stderr, "Error ejecutando ptrace con PTRACE_TRACEME. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }

    err_status = kill(getpid(), SIGSTOP);
    if (err_status == -1) {
        fprintf(stderr, "Error ejecutando kill con SIGSTOP. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }
    return execvp(argv[0], argv);
}

// Functions for parent process
/*
 * Child process runs until a system call is initialized or finished
 */
int continue_child_process_exection(pid_t child_process_id, bool pause) {
    int status;
    int error_status;
    while (true) {
        error_status = ptrace(PTRACE_SYSCALL, child_process_id, 0, 0);
        if (error_status == -1) {
            fprintf(stderr, "Error ejecutando ptrace con PTRACE_SYSCALL. (Errno %d: %s)\n",
                    errno, strerror(errno));
            return EXIT_FAILURE;
        }
        waitpid(child_process_id, &status, 0);

        // Se realiza un and con 0x80 para asegurar que el programa
        // se detuvo por un system call, debido a PTRACE_O_TRACESYSGOOD
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80){
            if (pause) {
                printf("System Call detectado. Presione cualquier tecla para continuar...");
                getchar();
            }
            return EXIT_SUCCESS;
        }
        if (WIFEXITED(status)) {
            return EXIT_FAILURE;
        }
    }
}

/*
 * Get system value returned from last call number execute in
 * process child_process_id
 */
int get_syscall_value_returned(system_call_data *syscall_reg, enum __ptrace_request request, pid_t child){
	int rax_content;

    rax_content = ptrace(request, child, sizeof(long) * RAX);
    if (rax_content == -1) {
        fprintf(stderr, "Error ejecutando ptrace al leer RAX. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }

    syscall_reg->system_call_value_returned = rax_content;
	return EXIT_SUCCESS;
}

/*
 * Get system last call number execute in process child_process_id
 */
int get_syscall_number(system_call_data *syscall_reg,
        enum __ptrace_request request, pid_t child_process_id){
    int orig_rax_content;
    char *syscall_name = NULL;

    orig_rax_content = ptrace(request, child_process_id, sizeof(long) * ORIG_RAX);
    if (orig_rax_content == -1) {
        fprintf(stderr, "Error ejecutando ptrace al leer ORIG_RAX. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }

    syscall_name = "Nombre de system call";
    syscall_reg->system_call_number = orig_rax_content;
    syscall_reg->system_call_name = syscall_name;
    return EXIT_SUCCESS;
}

/*
 * Print information for the system call in syscall_reg argument
 */
void print_system_call(system_call_data *syscall_reg) {
    fprintf(stderr, "Syscall (%d): %s\n    Returned value: %d\n",
            syscall_reg->system_call_number,
            syscall_reg->system_call_name,
            syscall_reg->system_call_value_returned);
}

/*
 * Make child_process_id continue to only stop    a SysCall
 * then waits for the child_process_id
 */
int execute_parent_process(pid_t child_process_id, bool pause){
	int status;
	system_call_data syscall_reg;
	waitpid(child_process_id, &status, 0);
	ptrace(PTRACE_SETOPTIONS, child_process_id, 0, PTRACE_O_TRACESYSGOOD);
	while(1) {
		if (continue_child_process_exection(child_process_id, pause) != 0){
            break;
		}
        status = get_syscall_number(&syscall_reg, PTRACE_PEEKUSER, child_process_id);
        if (status != EXIT_SUCCESS){
            return  status;
        }
		if (continue_child_process_exection(child_process_id, false) != 0) {
		    break;
		}
        status = get_syscall_value_returned(&syscall_reg, PTRACE_PEEKUSER, child_process_id);
        if (status != EXIT_SUCCESS){
            return  status;
        }
        print_system_call(&syscall_reg);
	}
	return EXIT_SUCCESS;
}

// Public functions

/*
 * Create a new process (with fork). Child process executes the program
 * to be traced and father process traces it.
 */
int system_call_tracer_execute(char **argv, bool pause){
	pid_t fork_return = fork();
    if (fork_return == 0) {
        return execute_child_process(argv);
    } else if (fork_return == -1) {
        fprintf(stderr, "Error ejecutando fork. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    } else {
        return execute_parent_process(fork_return, pause);
    }
}
