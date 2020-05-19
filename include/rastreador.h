#ifndef TAREA2_RASTREADOR_H
#define TAREA2_RASTREADOR_H



/**
 * Create a new process (with fork). Child process executes the program
 * to be traced and father process traces it.
 *
 * @param argv: Arguments for the child process to run
 * @param pause: true if the user wants to pause the code
 *               and continue by pressing any button, false
 *               otherwise
 * @return status_value: 0 for success, 1 for failure
 */
int system_call_tracer_execute(char **argv, bool pause);

#endif //TAREA2_RASTREADOR_H
