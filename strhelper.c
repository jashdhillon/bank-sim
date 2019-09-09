#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "strhelper.h"
#include "iohelper.h"

//Check if string s1 and s2 are equal
int is_string_equal(char* s1, char* s2) {
	return !strcmp(s1, s2);
}

//Returns number of occurances of character in the string provided
int contains(char* str, char c) {
	int i = 0, count = 0;

	while(str[i] != '\0') {
		count = (str[i] == c ? count + 1 : count);
		i++;
	}

	return count;
}

//Check if string s2 is in string s1
int contains_string(char* s1, char* s2) {
	return strstr(s1, s2) != NULL;
}

//Check if string s1 endswith string s2
int ends_with(char* s1, char* s2) {
	
	if(!s1) {
		return 0;
	} else if(!s2) {
		return 1;
	}

	size_t s1_len = strlen(s1);
	size_t s2_len = strlen(s2);

	if(s2_len > s1_len)
	    return 0;

	return is_string_equal(s1 + s1_len - s2_len, s2);
}

//Returns start and end indicies for string, trimming off the leading white spaces inside and outside quotation marks
void trim(char* str, int* start_index, int* end_index) {
	(*start_index) = 0;
	(*end_index) = strlen(str) - 1;

	if(*str == '\0') {
		(*start_index) = 0;
		(*end_index) = 0;
		return;
	}

	while((*end_index) > (*start_index) && (isspace(str[*start_index]) || str[*start_index] == '"')) {
		(*start_index)++;
	}

	if((*str) == '\0') {
		(*start_index) = 0;
		(*end_index) = 0;
		return;
	}

	while((*end_index) > (*start_index) && (isspace(str[*end_index]) || str[*end_index] == '"')) {
		(*end_index)--;
	}
}

//Tokenizes the input string on the delimeter provided as an array of strings
//Must free result
char** tokenize(char* data, char delim, int* result_size, int include_delim) {
	int data_index = 0, buffer_index = 0, result_index = 0, buffer_size = DEFAULT_BUFFER_SIZE;
	*result_size = DEFAULT_ARRAY_SIZE;
	char** result = (char**) realloc(NULL, sizeof(char*) * (*result_size));

	if(result == NULL) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}


	if(!result) {
		return result;
	}
	
	char* buffer = realloc(NULL, sizeof(char) * buffer_size);

	if(buffer == NULL) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}

	while(data[data_index] != '\0') {

	    	char c = data[data_index];
		data_index++;

		//Resize buffer if more space is needed
		if(buffer_index == (buffer_size - 1)) {
			buffer_size += DEFAULT_BUFFER_INCREMENT;
			buffer = realloc(buffer, sizeof(char) * buffer_size);
			if(buffer == NULL) {
				print_error("Cannot Allocate Memory");
				exit(1);
			}

		}

		//Resize result if more space is needed
		if(result_index == ((*result_size) - 1)) {
			*result_size += DEFAULT_ARRAY_INCREMENT;
			result = realloc(result, sizeof(char*) * (*result_size));
			if(result == NULL) {
				print_error("Cannot Allocate Memory");
				exit(1);
			}

		}

		buffer[buffer_index] = '\0';

		int occur = contains(buffer, '"');

		if(c == delim && (!occur || occur > 1)) {
			
			//Include delim on end of old section
			if(include_delim == 1) {
				buffer[buffer_index] = c;
				buffer_index++;
			}

			result[result_index] = buffer;
			result_index++;

			buffer_size = DEFAULT_BUFFER_SIZE;
			buffer_index = 0;
			buffer = realloc(NULL, sizeof(char) * buffer_size);
			if(buffer == NULL) {
				print_error("Cannot Allocate Memory");
				exit(1);
			}

			//Include delim on start of new section
			if(include_delim == 2) {
				buffer[buffer_index] = c;
				buffer_index++;
			}

		} else {
			buffer[buffer_index] = c;
			buffer_index++;
		}
	}

	buffer[buffer_index] = '\0';
	result[result_index] = buffer;
	result_index++;

	*result_size = result_index;

	return result;
}

//Tokenizes the input string on the delimeter provided as an array of strings and maps the input to index_map passed
//Must free result
char** tokenize_mapped(char* data, char delim, int result_size, int* index_map) {
	int data_index = 0, buffer_index = 0, result_index = 0, buffer_size = DEFAULT_BUFFER_SIZE;
	char** result = (char**) realloc(NULL, sizeof(char*) * (result_size));	

	if(result == NULL) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}
	
	char* buffer = realloc(NULL, sizeof(char) * buffer_size);

	if(buffer == NULL) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}

	int i;

	for(i = 0; i < result_size; i++) {
		result[i] = malloc(sizeof(char));
		
		if(!result[i]) {
			print_error("Cannot Allocate Memory");
			exit(1);
		}

		result[i][0] = '\0';
	}

	while(data[data_index] != '\0') {

	    	char c = data[data_index];
		data_index++;

		//Resize buffer if more space is needed
		if(buffer_index == (buffer_size - 1)) {
			buffer_size += DEFAULT_BUFFER_INCREMENT;
			buffer = realloc(buffer, sizeof(char) * buffer_size);
			if(buffer == NULL) {
				print_error("Cannot Allocate Memory");
				exit(1);
			}

		}

		buffer[buffer_index] = '\0';

		int occur = contains(buffer, '"');

		if(c == delim && (!occur || occur > 1)) {
			
			//Remove "" when 2 are present
			if(0 && occur == 2) {
				int j = 1;
				
				buffer[buffer_index - 1] = '\0';

				for(j = 1; j <= buffer_index; j++) {
					buffer[j-1] = buffer[j];
				}
			}

			int mapped_index = index_map[result_index];

			if(mapped_index < 0) {
				printf("There be some funny business heres\n");
				exit(1);
			}
			
			//Free default allocation
			free(result[mapped_index]);

			result[mapped_index] = buffer;
			result_index++;

			buffer_size = DEFAULT_BUFFER_SIZE;
			buffer_index = 0;
			buffer = realloc(NULL, sizeof(char) * buffer_size);
			
			if(buffer == NULL) {
				print_error("Cannot Allocate Memory");
				exit(1);
			}

		} else {
			buffer[buffer_index] = c;
			buffer_index++;
		}
	}
	
	//Handle what is remaining in buffer
	buffer[buffer_index] = '\0';
	int mapped_index = index_map[result_index];

	if(mapped_index < 0) {
		printf("There be some funny business heres\n");
		exit(1);
	}

	result[mapped_index] = buffer;
	result_index++;

	return result;
}

//Finds the index of the target string in the provided list
int find(char** list, int list_len, char* target) {
	int i, j;

	for(i = 0; i < list_len; i++) {
		int liststr_start = 0, liststr_end = 0;
		int targetstr_start = 0, targetstr_end = 0;
		
		trim(list[i], &liststr_start, &liststr_end);
		trim(target, &targetstr_start, &targetstr_end);

		char* liststr = list[i] + liststr_start;
		char* targetstr = target + targetstr_start;

		int liststr_len = (liststr_end - liststr_start + 1);
		int targetstr_len = (targetstr_end - targetstr_start + 1);

		int equal_len = liststr_len == targetstr_len;
		int min_len = liststr_len < targetstr_len ? liststr_len : targetstr_len;

		if(equal_len && strncmp(liststr, targetstr, min_len) == 0) {
			return i;
		}
	}

	return -1;
}

//Appends string s2 to string s1
char* append(char* s1, char* s2, int* s1_index, int* s1_len) {
	int append_size = strlen(s2);

	if(*s1_index + append_size + 1 > *s1_len) {
		(*s1_len) += DEFAULT_BUFFER_INCREMENT;
		s1 = realloc(s1, sizeof(char) * (*s1_len));
		return append(s1, s2, s1_index, s1_len);
	}

	int i;

	for(i = 0; i < append_size; i++) {
		s1[*s1_index] = s2[i];
		(*s1_index)++;
	}

	s1[*s1_index] = '\0';

	return s1;
}

//Checks to see if the string matches the given regex expression
int match(regex_t *exp, char* str) {
	regmatch_t matches[1];

	if(regexec(exp, str, 1, matches, 0) == 0) {
		return 1;
	} else {
		return 0;
	}
}

//Checks to see if the input string is an integer
int is_integer(char* data) {
	
	int rv;
	regex_t exp;

	rv = regcomp(&exp, "^-?[0-9]+$", REG_EXTENDED);

	if(rv != 0) {
		print_error("Regex expression could not be compiled");
		exit(1);
	}

	return match(&exp, data);
}

//Checks to see if the input string is a double
int is_double(char* data) {
	
	int rv;
	regex_t exp;

	rv = regcomp(&exp, "^-?[0-9]+(\\.[0-9]+)?$", REG_EXTENDED);

	if(rv != 0) {
		print_error("Regex expression could not be compiled");
		exit(1);
	}

	return match(&exp, data);
}

//Checks the type of the string provided
Type get_type(char* data) {
	if(is_integer(data)) {
		return Integer;
	} else if(is_double(data)) {
		return Double;
	}
	return String;
}

char* int_to_padded_string(int num, int len) {
	int digit_len = log10(num) + 1;

	if(digit_len > len) {
		print_error("Passed integer exceeds capacity of digit lenth provided");
		return NULL;
	}

	char* res = malloc(sizeof(char) * (len + 1));

	if(!res) {
		print_error("Cannot Allocate Memory");
		return NULL;
	}

	int i;

	char* res_num_start = res;

	for(i = 0; i < len - digit_len; i++) {
		res[i] = '0';
	}

	res_num_start += (len - digit_len);

	snprintf(res_num_start, digit_len + 1, "%d", num);
	
	return res;
}

int digits(long num) {
	if(num == 0) {
		return 1;
	}

	return 1 + digits(num / 10.0);
}

char* int_to_string(long num) {
	int digit_len = digits(num);

	int max_digits = 3 + DBL_MANT_DIG - DBL_MIN_EXP;

	if(digit_len > max_digits) {
		return NULL;
	}

	char* res = malloc(sizeof(char) * (digit_len + 4));

	if(!res) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}

	snprintf(res, digit_len, "%lu", num);

	res[digit_len + 1] = '\0';
	
	return res;
}

char* double_to_string(double num) {
	int digit_len = 3 + DBL_MANT_DIG - DBL_MIN_EXP;

	int res_index = 0;
	int res_len = digit_len;

	char* res = malloc(sizeof(char) * (res_len + 1));

	if(!res) {
		print_error("Cannot Allocate Memory");
		exit(1);
	
	}

	snprintf(res, digit_len + 1, "%lf", num);

	return res;
}

