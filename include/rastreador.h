#ifndef TAREA2_RASTREADOR_H
#define TAREA2_RASTREADOR_H

/**
 * Create a new process (with fork). Child process executes the program
 * to be traced and father process traces it.
 *
 * @param argv
 * @param pause
 * @return
 */
int system_call_tracer_execute(char **argv, bool pause);

#endif //TAREA2_RASTREADOR_H
