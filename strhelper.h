#include <regex.h>
#include "data.h"

//Check if string s1 and s2 are equal
int is_string_equal(char* s1, char* s2);

//Returns number of occurances of character in the string provided
int contains(char* str, char c);

//Check if string s2 is in string s1
int contains_string(char* s1, char* s2);

//Check if string s1 endswith string s2
int ends_with(char* s1, char* s2);

//Returns start and end indicies for string, trimming off the leading white spaces inside and outside quotation marks
void trim(char* str, int* start_index, int* end_index);

//Tokenizes the input string on the delimeter provided as an array of strings
//Must free result
char** tokenize(char* data, char delim, int* result_size, int include_delim);

//Tokenizes the input string on the delimeter provided as an array of strings and maps the input to index_map passed
//Must free result
char** tokenize_mapped(char* data, char delim, int result_size, int* index_map);

//Finds the index of the target string in the provided list
int find(char** list, int list_len, char* target);

//Checks to see if the string matches the given regex expression
int match(regex_t *exp, char* str);

//Checks to see if the input string is an integer
int is_integer(char* data);

//Checks to see if the input string is a double
int is_double(char* data);

//Checks the type of the string provided
Type get_type(char* data);

//Appends string s2 to string s1
char* append(char* s1, char* s2, int* s1_index, int* s1_len);

//Converts integer to a string padded with zeros if number of digits is less than len
char* int_to_padded_string(int num, int len);

//Gets the number of digits in the long passed in
int digits(long num);

//Converts integer to a string
char* int_to_string(long num);

//Converts double to a string
char* double_to_string(double num);
