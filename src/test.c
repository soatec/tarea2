#include <unistd.h>
#include <string.h>
#include "test.h"

int write_stdout(char *content, int iterations) {
    int status;
    content = strcat(content, "\n");
    for (int i = 0; i < iterations; i++){
        status = write(1, content, strlen(content));
        if (status == -1) {
            return status;
        }
    }
    return  status;
}