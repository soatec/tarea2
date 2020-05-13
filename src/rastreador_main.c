#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "rastreador.h"




int main(int argc, char *argv[]) {
	int c;
	char *arg_prog =  NULL;

	 if (argc < 2) {
        fprintf(stderr, "Usage: %s prog args\n", argv[0]);
		return EXIT_FAILURE;
    }

	while (1) {
	   int option_index = 0;
	   static struct option long_options[] = {
		   {"Prog",  required_argument, 0, 0},
		   {"",  required_argument, 0, 'v'},
		   {"",  required_argument, 0, 'V'},
		   {0,         0,                 0,  0 }
	   };

	   c = getopt_long(argc, argv, ":v:V:",
				long_options, &option_index);
	   if (c == -1)
		   break;

	   switch (c) {	   
	   case 'v':
		   printf("option v with value '%s'\n", optarg);
		   arg_prog = optarg;
		   break;

	   case 'V':
		   printf("option V with value '%s'\n", optarg);
		   arg_prog = optarg;
		   break;

	   case '?':
		   break;

	   default:
		   printf("use ./rastreador [-v|-V] Prog [opciones prog] \n");
	   }
	}
	if (optind < argc) {
	   printf("opciones prog: ");
	   while (optind < argc)
		   printf("%s ", argv[optind++]);
	   printf("\n");
	}	
	
	p_trace_request(1, &arg_prog);

	exit(EXIT_SUCCESS);
}
