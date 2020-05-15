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
		   printf("option v with prog '%s'\n", optarg);
		   arg_prog = optarg;
		   break;

	   case 'V':
		   printf("option V with prog '%s'\n", optarg);
		   arg_prog = optarg;
		   break;

	   case '?':
		   printf("option ?'\n");
		   break;

	   default:
		   printf("use ./rastreador [-v|-V] Prog [opciones prog] \n");
	   }
	}
	
	printf("  argc %d  optind %d \n",argc,optind);
	
	if (optind < argc) {
	  // printf("opciones prog: ");
	    while (optind < argc)
			printf("%s ", argv[optind++]);
	   printf("\n");
	}	
	printf("running %s argc %d  optind %d \n",arg_prog,argc,optind);
	p_trace_request(1, &arg_prog);

	exit(EXIT_SUCCESS);
}
