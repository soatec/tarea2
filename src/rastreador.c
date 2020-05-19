#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>
#include <termios.h>
#include "rastreador.h"
#include "constants.h"
// Structures

/*
 * Hold system call information
 */
typedef struct system_call_data {
    long int system_call_number;
    long int system_call_return_value;
    long int system_call_args[ARGS_QUANTITY];
} system_call_data;

/*
 * Hold system call usage count
 */
typedef struct system_call_usage {
    long int system_call_number;
    long int system_call_usage_count;
    struct system_call_usage* next;
} system_call_usage;

// Private functions

// Helper functions for system_call_usage linked list
/*
 * Init system_call_usage linked list
 */
system_call_usage * init_system_call_usage() {
    system_call_usage * head = NULL;
    head = (system_call_usage *) malloc(sizeof(system_call_usage));
    head->system_call_number = -1;
    head->system_call_usage_count = 0;
    head->next = NULL;
    return head;
}

/*
 * Print system call usage count log
 */
void print_system_call_usage(system_call_usage * head) {
    system_call_usage * current = head;

    while (current != NULL) {

        if (current->system_call_number >= SYSCALLS_QUANTITY){
            fprintf(stderr, "Error recuperando la información del system call %ld.\n",
                    current->system_call_number);
            return;
        }
        fprintf(stdout, "Se usaron: %ld"
                        "\tSyscall (%ld): %s\n",
                current->system_call_usage_count,
                current->system_call_number,
                syscalls_info[current->system_call_number].system_call_name);

        current = current->next;
    }
}

/*
 * Add system call count to log
 */
void push_system_call_usage(system_call_usage * head, long int in_system_call_number) {
    if (head->system_call_number == -1) {
        head->system_call_number = in_system_call_number;
        head->system_call_usage_count = 1;
        head->next = NULL;
        return;
    }

    system_call_usage * current = head;

    while (current->next != NULL) {
        if (current->system_call_number == in_system_call_number) {
            current->system_call_usage_count++;
            return;
        }
        current = current->next;
    }
    if (current->system_call_number == in_system_call_number) {
        current->system_call_usage_count++;
        return;
    }

    current->next = (system_call_usage *) malloc(sizeof(system_call_usage));
    current->next->system_call_number = in_system_call_number;
    current->next->system_call_usage_count = 1;
    current->next->next = NULL;
}

/*
 * Clean system_call_usage linked list
 */
void clean_system_call_usage(system_call_usage * head) { 
    system_call_usage * current = head;
    system_call_usage * next;
  
    while (current != NULL) { 
       next = current->next;
       free(current);
       current = next;
    }

    head = NULL; 
} 

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
        return FAILURE;
    }

    err_status = kill(getpid(), SIGSTOP);
    if (err_status == -1) {
        fprintf(stderr, "Error ejecutando kill con SIGSTOP. (Errno %d: %s)\n",
                errno, strerror(errno));
        return FAILURE;
    }
    return execvp(argv[0], argv);
}

// Functions for parent process
/*
 * Child process runs until a system call is initialized or finished
 */
int continue_child_process_execution(pid_t child_process_id, bool pause) {
    int status;
    int error_status;
    static struct termios oldt;
    static struct termios newt;
    while (true) {
        error_status = ptrace(PTRACE_SYSCALL, child_process_id, 0, 0);
        if (error_status == -1) {
            fprintf(stderr, "Error ejecutando ptrace con PTRACE_SYSCALL. (Errno %d: %s)\n",
                    errno, strerror(errno));
            return FAILURE;
        }
        waitpid(child_process_id, &status, 0);

        if (WIFEXITED(status)) {
            return SUCCESS;
        }

        // Se realiza un and con 0x80 para asegurar que el programa
        // se detuvo por un system call, debido a PTRACE_O_TRACESYSGOOD
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80){
            if (pause) {
                printf("System Call detectado. Presione cualquier tecla para continuar...\n");
                tcgetattr( STDIN_FILENO, &oldt);
                newt = oldt;
                newt.c_lflag &= ~(ICANON | ECHO);
                tcsetattr(STDIN_FILENO, TCSANOW, &newt);
                getchar();
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            }
            return IN_PROGESS;
        }
    }
}

/*
 * Get system value args from last call number execute in
 * process child_process_id
 */
int get_system_call_return_args(system_call_data *syscall_reg,
                                enum __ptrace_request request, pid_t child_process_id){
    int current_arg_content;
    for (int i = 0; i < ARGS_QUANTITY; i++){
        errno = 0;
        current_arg_content = ptrace(request, child_process_id, sizeof(long) * arguments[i]);
        if (errno != 0) {
            fprintf(stderr, "Error ejecutando ptrace al leer el argumento %d de syscall. (Errno %d: %s)\n",
                    i, errno, strerror(errno));
            return FAILURE;
        }
        syscall_reg->system_call_args[i] = current_arg_content;
    }
    return SUCCESS;
}


/*
 * Get system value return from last call number execute in
 * process child_process_id
 */
int get_system_call_return_value(system_call_data *syscall_reg,
                                 enum __ptrace_request request, pid_t child_process_id){
    long int reg_syscall_return_content;

    errno = 0;
    reg_syscall_return_content = ptrace(request, child_process_id, sizeof(long) * REG_SYSCALL_RETURN);
    if (errno != 0) {
        fprintf(stderr, "Error ejecutando ptrace al leer REG_SYSCALL_RETURN. (Errno %d: %s)\n",
                errno, strerror(errno));
        return FAILURE;
    }

    syscall_reg->system_call_return_value = reg_syscall_return_content;
    return SUCCESS;
}

/*
 * Get system last call number execute in process child_process_id
 */
int get_system_call_number(system_call_data *syscall_reg,
                           enum __ptrace_request request, pid_t child_process_id){
    long int reg_syscall_number_content;
    errno = 0;

    reg_syscall_number_content = ptrace(request, child_process_id, sizeof(long) * REG_SYSCALL_NUMBER);
    if (errno != 0) {
        fprintf(stderr, "Error ejecutando ptrace al leer REG_SYSCALL_NUMBER. (Errno %d: %s)\n",
                errno, strerror(errno));
        return FAILURE;
    }


    syscall_reg->system_call_number = reg_syscall_number_content;
    return SUCCESS;
}

/*
 * Print information for the system call in syscall_reg argument
 */
void print_system_call(system_call_data *syscall_reg) {
    if (syscall_reg->system_call_number >= SYSCALLS_QUANTITY){
        fprintf(stderr, "Error recuperando la información del system call %ld.\n",
                syscall_reg->system_call_number);
        return;
    }
    fprintf(stdout, "Syscall (%ld): %s\n"
                    "    Argumentos:\n",
            syscall_reg->system_call_number,
            syscalls_info[syscall_reg->system_call_number].system_call_name);
    for (int i = 0; i < ARGS_QUANTITY; i++) {
        if (strcmp(syscalls_info[syscall_reg->system_call_number].system_call_args[i], "") == 0){
            break;
        }
        fprintf(stdout, "        Argumento %d: %ld (%s)\n",
                i,
                syscall_reg->system_call_args[i],
                syscalls_info[syscall_reg->system_call_number].system_call_args[i]);
    }
    fprintf(stdout, "    Return value: %ld\n",
            syscall_reg->system_call_return_value);
}

/*
 * Make child_process_id continue to only stop    a SysCall
 * then waits for the child_process_id
 */
int execute_parent_process(pid_t child_process_id, bool pause){
    int status = -1;
    int continue_child_status = -1;
    system_call_usage * system_call_usage_log = init_system_call_usage();
    system_call_data syscall_reg;
    waitpid(child_process_id, &status, 0);
    ptrace(PTRACE_SETOPTIONS, child_process_id, 0, PTRACE_O_TRACESYSGOOD);
    while(true) {
        status = continue_child_process_execution(child_process_id, pause);
        if (status == FAILURE){
            break;
        }
        status = get_system_call_number(&syscall_reg, PTRACE_PEEKUSER, child_process_id);
        if (status == FAILURE){
            printf("a\n");
            break;
        }
        status = get_system_call_return_args(&syscall_reg, PTRACE_PEEKUSER, child_process_id);
        if (status == FAILURE){
            printf("b\n");
            break;
        }

        continue_child_status = continue_child_process_execution(child_process_id, false);
        if (continue_child_status == FAILURE) {
            break;
        }

        if (continue_child_status == SUCCESS) {
            syscall_reg.system_call_return_value = 0;
            print_system_call(&syscall_reg);
            printf("Ejecución del programa terminada satisfactoriamente.\n");
            print_system_call_usage(system_call_usage_log);
            clean_system_call_usage(system_call_usage_log);
            return SUCCESS;
        }

        status = get_system_call_return_value(&syscall_reg, PTRACE_PEEKUSER, child_process_id);
        if (status == FAILURE){
            break;
        }
        print_system_call(&syscall_reg);
        push_system_call_usage(system_call_usage_log, syscall_reg.system_call_number);
    }
    return FAILURE;
}

// Public functions

/*
 * Create a new process (with fork). Child process executes the program
 * to be traced and father process traces it.
 */
int system_call_tracer_execute(char **argv, bool pause){
    if (VALID_ARCH == 0) {
        fprintf(stderr, "La arquitectura actual no es soportada. Únicamente se soporta x86_64 e i386\n");
        return FAILURE;
    }
    pid_t fork_return = fork();
    if (fork_return == 0) {
        return execute_child_process(argv);
    } else if (fork_return == -1) {
        fprintf(stderr, "Error ejecutando fork. (Errno %d: %s)\n",
                errno, strerror(errno));
        return FAILURE;
    } else {
        return execute_parent_process(fork_return, pause);
    }
}
