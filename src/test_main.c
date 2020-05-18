#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include "test.h"

int main(int argc, char *argv[]) {
    int  opt;
    int  status;
    int  iterations = -1;
    char *content = NULL;

    while ((opt = getopt(argc, argv, "i:c:")) != -1) {
        switch (opt) {
            case 'i':
                iterations = atoi(optarg);
                break;
            case 'c':
                content = optarg;
                break;
            default:
                fprintf(stderr, "Uso: %s -i iteraciones -c contenido\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (content == NULL) {
        fprintf(stderr, "-c content is a mandatory argument\n");
        return EXIT_FAILURE;
    }

    if (iterations == -1) {
        fprintf(stderr, "-i iterations is a mandatory argument\n");
        return EXIT_FAILURE;
    } else if (iterations < 0) {
        fprintf(stderr, "-i iterations must be positive\n");
        return EXIT_FAILURE;
    }

    status = write_stdout(content, iterations);
    if (status == -1) {
        fprintf(stderr, "Error en el programa test. (Errno %d: %s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}