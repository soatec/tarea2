#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>
#include "rastreador.h"

int main(int argc, char *argv[]) {
    int opt;
    int status;
    int quantity_arguments_parsed = 0;
    bool pause = false;
    char *prog_args[argc - 1];
    memcpy(prog_args, &argv[2], (argc - 1) * sizeof(char*));

    while ((opt = getopt(argc, argv, ":v::V::")) != -1) {
        switch (opt) {
            case 'V':
                pause = true;
            case 'v':
                quantity_arguments_parsed++;
                break;
            case '?':
                break;
            default:
                fprintf(stderr, "Uso: %s [-v | -V] Prog [opciones de Prog]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (quantity_arguments_parsed != 1){
        fprintf(stderr, "Verifique las opciones -v y -V.\n");
        return EXIT_FAILURE;
    }

    printf("\n"
           "  ____           _                      _                  _        _ _ ____            _                    ____      _ _     _ _ \n"
           " |  _ \\ __ _ ___| |_ _ __ ___  __ _  __| | ___  _ __    __| | ___  ( | ) ___| _   _ ___| |_ ___ _ __ ___    / ___|__ _| | |___( | )\n"
           " | |_) / _` / __| __| '__/ _ \\/ _` |/ _` |/ _ \\| '__|  / _` |/ _ \\  V V\\___ \\| | | / __| __/ _ \\ '_ ` _ \\  | |   / _` | | / __|V V \n"
           " |  _ < (_| \\__ \\ |_| | |  __/ (_| | (_| | (_) | |    | (_| |  __/      ___) | |_| \\__ \\ ||  __/ | | | | | | |__| (_| | | \\__ \\    \n"
           " |_| \\_\\__,_|___/\\__|_|  \\___|\\__,_|\\__,_|\\___/|_|     \\__,_|\\___|     |____/ \\__, |___/\\__\\___|_| |_| |_|  \\____\\__,_|_|_|___/    \n"
           "                                                                              |___/                                                \n");

    status = system_call_tracer_execute((char **) &prog_args, pause);
    if (status == -1) {
        fprintf(stderr, "Error en el programa rastreador. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}