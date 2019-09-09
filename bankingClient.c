#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <pthread.h>

#include "data.h"
#include "bankingClient.h"
#include "iohelper.h"
#include "strhelper.h"

#define DATA "Hello World"

int sock;

int is_running = 0;
int is_logged_on = 0;

pthread_mutex_t is_running_lock;
pthread_mutex_t is_logged_on_lock;

//Gets reads and parses the execution arguments
int parse_exec_args(int argc, char** argv, char** host_name, unsigned int* port) {

	int i;
	int has_host_name = 0;
	int has_port = 0;
	int expected_args = 1;

	for(i = 1; i < argc; i++) {
		if(!has_host_name) {
			has_host_name = i;
			expected_args += 1;
		} else if(!has_port) {
			has_port = i;
			expected_args += 1;
		}
	}

	if(argc < expected_args || argc > expected_args) {
		print_error("Invalid Argument Count");
		return 1;
	}

	if(!has_host_name) {
		print_error("Runtime arguments must include host name");
		return 1;
	}

	if(!has_port) {
		print_error("Runtime arguments must include port number");
		return 1;
	}

	*host_name = argv[has_host_name];

	//TODO: add for checks to see if the arg passed is an int
	unsigned int port_val = 0;
	
	sscanf(argv[has_port], "%d", &port_val);

	if(port_val <= 0) {
		//Arbitrary, but usefull check 
		print_error("Entered port number is not valid (must be greater than 0)");
		return 1;
	}

	*port = port_val;

	return 0;
}

int send_packet(int socket, char* data, int data_len) {

	//Prefix packet meta-data
	char* packet_meta_data = int_to_padded_string(data_len, 16);

	//Send packet meta_data
	int send_res = send(socket, packet_meta_data, 16, 0);
	
	if(send_res < 0) {
		print_error("Error Occured While Sending Data");
		return send_res;
	}
		
	//Send packet
	send_res = send(socket, data, data_len, 0);
	
	if(send_res < 0) {
		print_error("Error Occured While Sending Data");
	}

	//printf("<CLIENT>: %s\n", data);

	return send_res;
}

int recv_packet(int socket, char** data) {
	int bytes_read = 0;

	int meta_data_index = 0;
	int meta_data_len = DEFAULT_BUFFER_SIZE;
	char* meta_data = malloc(sizeof(char) * meta_data_len);

	if(!meta_data) {
		print_error("Cannot Allocate Memory");
		return -1;
	}

	while(bytes_read < 16) {
		char buff[2];

		int rval = recv(socket, buff, 1, 0);

		buff[1] = '\0';

		if(rval > 0) {
			bytes_read += rval;

			meta_data = append(meta_data, buff, &meta_data_index, &meta_data_len);
		} else if(rval == 0) {
			printf("Ending Connection with Server\n");
			return 0;
		}
	}	

	bytes_read = 0;

	int data_index = 0, data_len = -1;

	sscanf(meta_data, "%d", &data_len);

	free(meta_data);

	*data = malloc(sizeof(char) *  data_len);

	if(!data) {
		print_error("Cannot Allocate Memory");
		return -1;
	}

	if(data_len < 0) {
		print_error("Error Occured while Parsing Packet Data Length");
		return -1;
	}

	while(bytes_read < data_len) {
		char buff[2];

		int rval = recv(socket, buff, 1, 0);

		buff[1] = '\0';

		if(rval > 0) {
			bytes_read += rval;
			
			*data = append(*data, buff, &data_index, &data_len);
		} else if(rval == 0) {
			printf("Ending Connection with Server\n");
			return 0;
		}

		//printf("%d %d\n", bytes_read, data_len);
	}

	//printf("<SERVER>: %s\n", *data);

	return data_len;
}

char* replace_c(char* src, char tar, char rep) {
	int i, len = strlen(src);

	for(i = 0; i < len; i++) {
		if(src[i] == tar) {
			src[i] = rep;
		}
	}

	return src;
}

char* spaceize(char* src) {
	return replace_c(src, '_', ' ');
}

char* despace(char* src) {
	return replace_c(src, ' ', '_');
}

int process_response(int socket, char* input) {
	if(!input) {
		print_error("Response is NULL");
		return -1;
	}

	int res_argc = 0;

	char** res_argv = tokenize(input, ' ', &res_argc, 0);
	char* res = res_argv[0];

	if(is_string_equal(res, "create")) {
		if(is_string_equal(res_argv[res_argc - 1], "success")) {
			printf("Created Account\n");
		} else {
			printf("Failed to Create Account: %s\n", spaceize(res_argv[res_argc - 1]));
		}
	} else if(is_string_equal(res, "serve")) {
		if(is_string_equal(res_argv[res_argc - 1], "success")) {
			printf("Served Account\n");
			pthread_mutex_lock(&is_logged_on_lock);
			is_logged_on = 1;
			pthread_mutex_unlock(&is_logged_on_lock);
		} else {
			printf("Failed to Create Account: %s\n", spaceize(res_argv[res_argc - 1]));
		}
	} else if(is_string_equal(res, "deposit")) {
		if(is_string_equal(res_argv[res_argc - 2], "success")) {
			printf("Deposited to Account\n");
			printf("Balance: %s\n", res_argv[res_argc - 1]);
		} else {
			printf("Failed to Deposit to Account: %s\n", spaceize(res_argv[res_argc - 1]));
		}
	} else if(is_string_equal(res, "withdraw")) {
		if(is_string_equal(res_argv[res_argc - 2], "success")) {
			printf("Withdrawn from Account\n");
			printf("Balance: %s\n", res_argv[res_argc - 1]);
		} else {
			printf("Failed to Withdraw from Account: %s\n", spaceize(res_argv[res_argc - 1]));
		}

	} else if(is_string_equal(res, "query")) {
		if(is_string_equal(res_argv[res_argc - 2], "success")) {
			printf("Queryed Account\n");
			printf("Balance: %s\n", res_argv[2]);
		} else {
			printf("Failed to Query Account: %s\n", spaceize(res_argv[res_argc - 1]));
		}
	} else if(is_string_equal(res, "end")) {
		if(is_string_equal(res_argv[res_argc - 1], "success")) {
			printf("Ended Account Session\n");
			pthread_mutex_lock(&is_logged_on_lock);
			is_logged_on = 0;
			pthread_mutex_unlock(&is_logged_on_lock);
		} else {
			printf("Failed to End Session: %s\n", spaceize(res_argv[res_argc - 1]));
		}
	} else if(is_string_equal(res, "quit")) {
		if(is_string_equal(res_argv[res_argc - 1], "success")) {
			printf("Disconnected from Server\n");

			pthread_mutex_lock(&is_logged_on_lock);
			is_logged_on = 0;
			pthread_mutex_unlock(&is_logged_on_lock);

			pthread_mutex_lock(&is_running_lock);
			is_running = 0;
			pthread_mutex_unlock(&is_running_lock);
		} else {
			printf("Failed to Quit: %s\n", spaceize(res_argv[res_argc - 1]));
		}
	} else if(is_string_equal(res, "shutdown")) {
		printf("Server is Shutting Down, Successfully Disconnected from Server\n");
		pthread_mutex_lock(&is_logged_on_lock);
		is_logged_on = 0;
		pthread_mutex_unlock(&is_logged_on_lock);

		pthread_mutex_lock(&is_running_lock);
		is_running = 0;
		pthread_mutex_unlock(&is_running_lock);
	} else {
		free(res_argv);
		return -1;
	}

	printf("\n");

	free(res_argv);
	free(input);
	return 0;
}

void* response_handler(void* argv) {
	response_handler_args* args = (response_handler_args*) argv;
	
	int socket = args -> socket;

	while(is_running) {
		char* data;
		int recv_res = recv_packet(socket, &data);
		
		//TODO: Add safety nets for is_running
		if(recv_res > 0) {
			//printf("<SERVER>: %s\n", data);

			int res_res = process_response(socket, data);

			if(res_res < 0) {
				print_error("An Error Occured while Processing a Reponse");
			}
		} else if(recv_res == 0) {
			pthread_mutex_lock(&is_running_lock);
			is_running = 0;
			pthread_mutex_unlock(&is_running_lock);
		} else {
			pthread_mutex_lock(&is_running_lock);
			is_running = 0;
			pthread_mutex_unlock(&is_running_lock);
			print_error("An Error Occured While Receving Packet");
		}
	}

	//printf("EXITING RESPONSE HANDLER\n");
	exit(0);
}

int process_command(char* input) {
	if(!input) {
		print_error("Command Entered is NULL");
		return -1;
	}

	int cmd_argc = 0;

	char** cmd_argv = tokenize(input, ' ', &cmd_argc, 0);
	char* cmd = cmd_argv[0];

	//printf("Val: %s, Null: %d\n", cmd_argv[1], *cmd_argv[1] == 0);

	if(is_string_equal(cmd, "create")) {
		pthread_mutex_lock(&is_logged_on_lock);
		char* args = input + 7;
		if(cmd_argc < 2) {
			printf("Invalid Argument Count for Command: create\n");
		} else if(is_logged_on) {
			printf("Currenly Serving an Account, Cannot Create Another\n");
		} else if(!(*args) || strlen(args) > 255) {
			printf("Account Name Must be Greater than 0, but Less than 256 Characters\n");
		} else {
			printf("Creating Account\n");
			send_packet(sock, input, strlen(input) + 1);
		}
		pthread_mutex_unlock(&is_logged_on_lock);
	} else if(is_string_equal(cmd, "serve")) {
		pthread_mutex_lock(&is_logged_on_lock);
		char* args = input + 6;
		if(cmd_argc < 2) {
			printf("Invalid Argument Count for Command: serve\n");
		} else if(!(*args) || strlen(args) > 255) {
			printf("Account Name Must be Greater than 0, but Less than 256 Characters\n");
		} else if(is_logged_on) {
			printf("Already Serving an Account, Cannot Serve Another\n");
		} else {
			printf("Serving Account\n");
			send_packet(sock, input, strlen(input) + 1);
		}
		pthread_mutex_unlock(&is_logged_on_lock);
	} else if(is_string_equal(cmd, "deposit")) {
		pthread_mutex_lock(&is_logged_on_lock);
		if(cmd_argc != 2) {
			printf("Invalid Argument Count for Command: deposit\n");
		} else if(!(*cmd_argv[1]) || !is_double(cmd_argv[1])) {
			printf("Deposit Amount is Required as Decimal Number\n");
		} else if(!is_logged_on) {
			printf("No Session Exists, Cannot Make Desposit\n");
		} else {
			printf("Depositing to Account\n");
			send_packet(sock, input, strlen(input) + 1);
		}
		pthread_mutex_unlock(&is_logged_on_lock);
	} else if(is_string_equal(cmd, "withdraw")) {
		pthread_mutex_lock(&is_logged_on_lock);
		if(cmd_argc != 2) {
			printf("Invalid Argument Count for Command: withdraw\n");
		} else if(!(*cmd_argv[1]) || !is_double(cmd_argv[1])) {
			printf("Withdraw Amount is Required as Decimal Number\n");
		} else if(!is_logged_on) {
			printf("No Session Exists, Cannot Make Withdraw\n");
		} else {
			printf("Withdrawing from Account\n");
			send_packet(sock, input, strlen(input) + 1);
		}
		pthread_mutex_unlock(&is_logged_on_lock);
	} else if(is_string_equal(cmd, "query")) {
		pthread_mutex_lock(&is_logged_on_lock);
		if(cmd_argc != 1) {
			printf("Invalid Argument Count for Command: query\n");
		} else if(!is_logged_on) {
			printf("No Session Exists, Cannot Query Account\n");
		} else {
			printf("Querying Account\n");
			send_packet(sock, input, strlen(input) + 1);
		}
		pthread_mutex_unlock(&is_logged_on_lock);
	} else if(is_string_equal(cmd, "end")) {
		pthread_mutex_lock(&is_logged_on_lock);
		if(cmd_argc != 1) {
			printf("Invalid Argument Count for Command: end\n");
		} else if(!is_logged_on) {
			printf("No Session Exists, Cannot End Session\n");
		} else {
			printf("Ending Account Session\n");
			send_packet(sock, input, strlen(input) + 1);
		}
		pthread_mutex_unlock(&is_logged_on_lock);
	} else if(is_string_equal(cmd, "quit")) {
		if(cmd_argc != 1) {
			printf("Invalid Argument Count for Command: end\n");
		} else {
			printf("Disconnecting from Server\n");
			send_packet(sock, input, strlen(input) + 1);
		}
	} else {
		free(cmd_argv);
		return -1;
	}

	free(cmd_argv);
	return 0;
}

void* input_handler(void* argv) {	

	while(is_running) {
		char* input = get_line(stdin);

		pthread_mutex_lock(&is_running_lock);
		if(is_running) {
			int cmd_res = process_command(input);

			if(cmd_res < 0) {
				printf("Invalid Command Passed! Valid Commands Include <create, serve, deposit, widthdraw, query, end, quit>\n");
			}
		}
		pthread_mutex_unlock(&is_running_lock);

		free(input);

		sleep(2);
	}
}

int main(int argc, char* argv[]) {
	
	pthread_mutex_init(&is_running_lock, NULL);
	pthread_mutex_init(&is_logged_on_lock, NULL);

	char* host_name;
	unsigned int port;

	if(parse_exec_args(argc, argv, &host_name, &port)) {
		print_error("An error occured, while parsign execution arguments");
		return 1;
	}

	struct sockaddr_in server;
	struct hostent *hp;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0) {
		print_error("Failed to create socket");
		return 0;
	}

	server.sin_family = AF_INET;
	
	hp = gethostbyname(host_name);

	if(hp == 0) {
		print_error("GetNameByHost Failed");
		return 1;
	} 

	memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_port = htons(port);
	
	int con_res = -1;

	//Repeat attempts to make a connection in intervals of 3 seconds
	while(con_res < 0) {
		con_res = connect(sock, (struct sockaddr*) &server, sizeof(server));
		if(con_res < 0) {
			print_error("Failed to connect to server");
			sleep(3);
		}
	}

	printf("Successfully Connected to: %s at ip: %u on port: %d\n", host_name, server.sin_addr, port);	
	
	//Does not need lock as it occures before child thread is spawned
	is_running = 1;

	response_handler_args* rhargs = malloc(sizeof(response_handler_args));

	rhargs->socket = sock;

	if(!rhargs) {
		print_error("Cannot Allocate Memeory");
		return 1;
	}

	pthread_t* tid = malloc(sizeof(pthread_t));

	if(!tid) {
		print_error("Cannot Allocate Memory");
		return 1;
	}

	if(pthread_create(tid, NULL, response_handler, rhargs)) {
		print_error("Error Spawning Thread");
		return 1;
	}

	input_handler(NULL);

	close(sock);

	return 0;
}
