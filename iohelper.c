#include <stdio.h>
#include <stdlib.h>
#include "data.h"
#include "iohelper.h"

//Prints error message
void print_error(char* msg) {
	fprintf(stderr, "\nOops! Somethings seems to have gone wrong here: %s\n\n", msg);
}

//Gets the next line from standard input
//Must free results
char* get_line(FILE* file) {
	char *input;
	int c = NULL, size = DEFAULT_BUFFER_SIZE, len = 0;
	
	input = malloc(sizeof(char) * size);

	if(!input) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}

	if(!input) {
		return input;
    	}

	c = fgetc(file);

	while(c != EOF && c != '\n') {
		if(c != '\r') {
	       		input[len] = c;

			len++;

			if(len == (size - 1)) {
				size += DEFAULT_BUFFER_INCREMENT;
        			input = realloc(input, sizeof(char) * size);
				
				if(!input) {
					print_error("Cannot Allocate Memory");
					exit(1);
				}

         			if(!input) {
					return input;
				}
        		}
		}

		c = fgetc(file);
    	}

	input[len] = '\0';

	if(len < (size - 1)) {
		input = realloc(input, sizeof(char) * (len + 1));
		
		if(!input) {
			print_error("Cannot Allocate Memory");
			exit(1);
		}
	}

	return input;
}
