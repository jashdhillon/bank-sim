#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <signal.h>
#include <semaphore.h>

#include <pthread.h>
#include <float.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "data.h"
#include "bankingServer.h"
#include "strhelper.h"

int sock = -1;

int* socket_index = NULL;
int* socket_len = NULL;
int* sockets = NULL;

int* account_index = NULL;
int* account_len = NULL;
account** accounts = NULL;

pthread_mutex_t accounts_lock;
pthread_mutex_t sockets_lock;
sem_t diag_lock;

//Gets reads and parses the execution arguments
int parse_exec_args(int argc, char** argv, unsigned int* port) {

	int i;
	int has_port = 0;
	int expected_args = 1;

	for(i = 1; i < argc; i++) {
		if(!has_port) {
		    has_port = i;
		    expected_args += 1;
		}
	}

	if(argc < expected_args || argc > expected_args) {
		print_error("Invalid Argument Count");
		return 1;
	}

	if(!has_port) {
		print_error("Runtime arguments must include port number");
		return 1;
	}

	unsigned int port_val = 0;
	
	sscanf(argv[has_port], "%d", &port_val);

	if(port_val <= 0) {
		//Arbitrary, but usefull check to 
		print_error("Entered port number is not valid (must be greater than 0)");
		return 1;
	}

	*port = port_val;

	return 0;
}

int send_packet(int socket, char* data, int data_len) {

	//Prefix packet meta-data
	char* packet_meta_data = int_to_padded_string(data_len + 1, 16);

	//Send packet meta_data
	int send_res = send(socket, packet_meta_data, 16, 0);
	
	if(send_res < 0) {
		print_error("Error Occured While Sending Data");
		return send_res;
	}
		
	//Send packet
	send_res = send(socket, data, data_len + 1, 0);
	
	if(send_res < 0) {
		print_error("Error Occured While Sending Data");
	}

	//printf("<SERVER>: %s\n", data);

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
			printf("Ending Connection with Client\n");
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
			printf("Ending Connection with Client\n");
			return 0;
		}
	}

	//printf("<CLIENT>: %s\n", *data);

	return data_len;
}

int contains_account(char* name) {
	
	int i;

	for(i = 0; i < *account_index; i++) {
		char* cur = accounts[i] -> name;

		if(is_string_equal(cur, name)) {
			return i;
		}
	}

	return -1;
}

int create_account(char* name) {
	if(contains_account(name) >= 0) {
		return -1;
	}

	int index = (*account_index)++;

	int name_len = strlen(name) + 1;
	char* name_val = malloc(sizeof(char) * name_len);

	if(!name_val) {
		printf("Cannot Allocate Memory");
		exit(1);
	}

	strncpy(name_val, name, name_len);

	accounts[index] = malloc(sizeof(account));
	accounts[index] -> name = name_val;
	accounts[index] -> balance = 0;
	accounts[index] -> is_active = 0;

	if((*account_index) == ((*account_len) - 1)) {
		(*account_len) += DEFAULT_ARRAY_INCREMENT;
		accounts = realloc(accounts, sizeof(account*) * (*account_len));

		if(!accounts) {
			print_error("Cannot Allocate Memory");
			exit(1);
		}
	}

	return index;
}

int process_command(int socket, char* input, int* is_logged_on) {
	if(!input) {
		print_error("Command Entered is NULL");
		return -1;
	}

	int cmd_argc = 0;

	char** cmd_argv = tokenize(input, ' ', &cmd_argc, 0);
	char* cmd = cmd_argv[0];

	//No data syntex validation needed as client performs this validation
	//Validate that the command can be accomplished
	
	int data_index = strlen(input);
	int data_len = data_index + 1;
	char* data = malloc(sizeof(char) * (data_len));

	if(!data) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}

	data = strncpy(data, input, data_len);

	data[data_index] = '\0';
	
	if(is_string_equal(cmd, "create")) {
		pthread_mutex_lock(&accounts_lock);
		char* args = input + 7;

		int create_res = create_account(args);
		pthread_mutex_unlock(&accounts_lock);

		if(*is_logged_on || create_res < 0) {
			printf("Failed to Create Account\n");
			data = append(data, " fail", &data_index, &data_len);

			if(*is_logged_on) {
				data = append(data, " Account_Cannot_Be_Created_While_Session_Is_In_Progress", &data_index, &data_len);
			} if(create_res == -1) {
				data = append(data, " Account_Already_Exists", &data_index, &data_len);
			}
		} else {
			printf("Creating Account\n");
			data = append(data, " success", &data_index, &data_len);
		}

		send_packet(socket, data, data_index);
	} else if(is_string_equal(cmd, "serve")) {
		pthread_mutex_lock(&accounts_lock);
		char* args = input + 6;

		int acc_index = contains_account(args);
		int exists = acc_index >= 0;
		int is_active = exists ? (accounts[acc_index] -> is_active) : 0;

		if(*is_logged_on || !exists || is_active) {
			printf("Cannot Serve Account\n");
			data = append(data, " fail", &data_index, &data_len);

			if(*is_logged_on) {
				data = append(data, " Cannot_Connect_to_Another_Account_While_One_Is_In_Use", &data_index, &data_len);
			} else if(exists) {
				data = append(data, " Account_Already_In_Use", &data_index, &data_len);
			} else {
				data = append(data, " Account_Doesn't_Exist", &data_index, &data_len);
			}
		} else {
			printf("Serving Account\n");
			data = append(data, " success", &data_index, &data_len);
			accounts[acc_index] -> is_active = 1;
			*is_logged_on = acc_index + 1;
		}

		pthread_mutex_unlock(&accounts_lock);

		send_packet(socket, data, data_index);
	} else if(is_string_equal(cmd, "deposit")) {	
		pthread_mutex_lock(&accounts_lock);

		double amt = -1;
		double cur_bal = accounts[(*is_logged_on) - 1] -> balance;

		amt = strtod(cmd_argv[1], NULL);
		
		if(amt <= 0) {
			data = append(data, " fail", &data_index, &data_len);
			data = append(data, " Deposit_Amount_Must_Be_Greater_Than_Zero", &data_index, &data_len);
		} else if(!(*is_logged_on)) {
			data = append(data, " fail", &data_index, &data_len);
			data = append(data, " Not_Currently_In_A_Session", &data_index, &data_len);
		} else {
			printf("Depositing to Account\n");
			data = append(data, " success", &data_index, &data_len);
			 
			cur_bal += amt;

			accounts[(*is_logged_on) - 1] -> balance = cur_bal;

			char* bal = double_to_string(accounts[(*is_logged_on) - 1] -> balance);

			data = append(data, " ", &data_index, &data_len);

			data = append(data, bal, &data_index, &data_len);

			free(bal);
		}

		pthread_mutex_unlock(&accounts_lock);

		send_packet(socket, data, data_index);
	} else if(is_string_equal(cmd, "withdraw")) {
		pthread_mutex_lock(&accounts_lock);

		double amt = -1;
		double cur_bal = accounts[(*is_logged_on) - 1] -> balance;

		amt = strtod(cmd_argv[1], NULL);

		if(amt > cur_bal || amt <= 0) {
			data = append(data, " fail", &data_index, &data_len);
			data = append(data, " Withdraw_Amount_Must_Be_Less_Than_Or_Equal_To_Balance_But_Greater_Than_Zero", &data_index, &data_len);
		} else if(!(*is_logged_on)) {
			data = append(data, " fail", &data_index, &data_len);
			data = append(data, " Not_Currently_In_A_Session", &data_index, &data_len);
		} else {

			printf("Withdrawing from Account\n");
			data = append(data, " success", &data_index, &data_len);
			 
			cur_bal -= amt;

			accounts[(*is_logged_on) - 1] -> balance = cur_bal;

			char* bal = double_to_string(accounts[(*is_logged_on) - 1] -> balance);

			data = append(data, " ", &data_index, &data_len);
			data = append(data, bal, &data_index, &data_len);

			free(bal);
		}

		pthread_mutex_unlock(&accounts_lock);

		send_packet(socket, data, data_index);
	} else if(is_string_equal(cmd, "query")) {
		pthread_mutex_lock(&accounts_lock);
		if(!(*is_logged_on)) {
			data = append(data, " fail", &data_index, &data_len);
			data = append(data, " Not_Currently_In_A_Session", &data_index, &data_len);
		} else {
			printf("Querying Account\n");
			data = append(data, " success", &data_index, &data_len);

			char* bal = double_to_string(accounts[(*is_logged_on) - 1] -> balance);

			data = append(data, " ", &data_index, &data_len);
			data = append(data, bal, &data_index, &data_len);

			free(bal);
		}

		pthread_mutex_unlock(&accounts_lock);

		send_packet(socket, data, data_index);
	} else if(is_string_equal(cmd, "end")) {
		pthread_mutex_unlock(&accounts_lock);
		if(!(*is_logged_on)) {
			data = append(data, " fail", &data_index, &data_len);
			data = append(data, " Cannot_Connect_to_Another_Account_While_One_Is_In_Use", &data_index, &data_len);
		} else {
			data = append(data, " success", &data_index, &data_len);
			accounts[(*is_logged_on) - 1] -> is_active = 0;
			*is_logged_on = 0;
		}
		pthread_mutex_unlock(&accounts_lock);

		send_packet(socket, data, data_index);
	} else if(is_string_equal(cmd, "quit")) {
		printf("Disconnecting from Client\n");
		if((*is_logged_on) - 1 >= 0 && account_len > 0) {
			accounts[(*is_logged_on) - 1] -> is_active = 0;
		}
		*is_logged_on = 0;

		data = append(data, " success", &data_index, &data_len);
		send_packet(socket, data, data_index);
	} else {
		free(data);
		free(cmd_argv);
		return -1;
	}

	free(data);
	free(cmd_argv);
	return 0;
}

int is_exiting = 0;

void* client_handler(void* argv) {
	client_handler_args* args = (client_handler_args*) argv;
	
	int socket = args -> socket;
	int is_logged_on = 0;

	printf("Accepted Client Connection\n");

	int is_running = 1;

	while(is_running) {
		char* data;
		int recv_res = recv_packet(socket, &data);

		if(recv_res > 0) {
			sem_wait(&diag_lock);
			int cmd_res = process_command(socket, data, &is_logged_on);
			sem_post(&diag_lock);

			if(cmd_res < 0) {
				printf("Failed to Execute Command\n");
			}
		} else if(recv_res == 0) {
			is_running = 0;
		} else {
			is_running = 0;
			printf("An Error Occured While Receving Packet\n");
		}
	}

	close(socket);
}

void cleanup() {
    	printf("\nCleaning up and tie loose ends before exiting\n\n");
	int i;

	for(i = 0; i < *account_index; i++) {
		account* acc = accounts[i];
		free(acc);
	}

	for(i = 0; i < *socket_index; i++) {
		send_packet(sockets[i], "shutdown", strlen("shutdown"));
	}

	printf("END\n");
}

void handle_interupt() {
	is_exiting = 1;	
}

void handle_alarm(int sig) {
	int i;

	sem_wait(&diag_lock);
	printf("Accounts: \n[\n");
	for(i = 0; i < *account_index; i++) {
		account* acc = accounts[i];
		printf("%s\t%lf\t%s\n", acc -> name, acc -> balance, (acc -> is_active ? "IN SERVICE" : ""));
	}
	printf("]\n");

	sem_post(&diag_lock);
	alarm(DIAG_DELAY);
}

void* connection_handler(void* argv) {
	connection_handler_args* args = (connection_handler_args*) argv;
	
	int port = args -> port;

	struct sockaddr_in server;	

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0) {
		print_error("Failed to create socket");
		exit(1);
	}

	//Ensure the socket closes and is reusable as soon as the server is shut down
	int t = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if(bind(sock, (struct sockaddr*) &server, sizeof(server))) {
		print_error("Failed to bind to port");
		exit(1);
	}

	printf("Bound to port: %u\n", port);

	listen(sock, 5);

	printf("Listening for client connection\n");

	int tid_index = 0;
	int tid_len = DEFAULT_ARRAY_SIZE;

	pthread_t** tids = malloc(sizeof(pthread_t*) * tid_len);

	if(!tids) {
		print_error("Cannot Allocate Memory");
		exit(1);
	}

	free(args);

	while(1) {
		int client_socket = accept(sock, (struct sockaddr*) 0, 0);
		
		if(is_exiting) {
			break;
		}

		sem_wait(&diag_lock);
		if(client_socket == -1) {
			print_error("Failed to accept client connection");
			break;
		}

		client_handler_args* chargs = malloc(sizeof(client_handler_args));

		chargs->socket = client_socket;

		if(!chargs) {
			print_error("Cannot Allocate Memeory");
			exit(1);
		}

		pthread_t* tid = malloc(sizeof(pthread_t));

		if(!tid) {
			print_error("Cannot Allocate Memory");
			exit(1);
		}

		if(pthread_create(tid, NULL, client_handler, chargs)) {
			print_error("Error Spawning Thread");
			exit(1);
		}

		sem_post(&diag_lock);

		pthread_mutex_lock(&sockets_lock);

		sockets[(*socket_index)++] = client_socket;

		if((*socket_index) == ((*socket_len) - 1)) {
			(*socket_len) += DEFAULT_ARRAY_INCREMENT;

			sockets = realloc(sockets, sizeof(int) * (*socket_len));

			if(!sockets) {
				print_error("Cannot Allocate Memory");
				exit(1);
			}
		}
		pthread_mutex_unlock(&sockets_lock);

		tids[tid_index++] = tid;

		if(tid_index == (tid_len - 1)) {
			tid_len += DEFAULT_ARRAY_INCREMENT;
			tids = realloc(tids, sizeof(pthread_t*) * tid_len);

			if(!tids) {
				print_error("Cannot Allocate Memory");
				exit(1);
			}
		}
	}
}

int main(int argc, char* argv[]) {
    	printf("START\n");

    	//Register exit events
	signal(SIGINT, handle_interupt);
	atexit(cleanup);

	//Register diagnostic events
	signal(SIGALRM, handle_alarm);

	alarm(DIAG_DELAY);

	pthread_mutex_init(&accounts_lock, NULL);
	sem_init(&diag_lock, 0, 1);

	//Setup socket list
	socket_index = malloc(sizeof(int));
	socket_len = malloc(sizeof(int));

	if(!(socket_index && socket_len)) {
		print_error("Cannot Allocate Memory");
		return 1;
	}

	*socket_index = 0;
	*socket_len = DEFAULT_ARRAY_SIZE;
	sockets = malloc(sizeof(int) * (*socket_len));

	if(!sockets) {
		print_error("Cannot Allocate Memory");
		return 1;
	}

	//Setup accounts list
	account_index = malloc(sizeof(int));
	account_len = malloc(sizeof(int));

	if(!(account_index && account_len)) {
		print_error("Cannot Allocate Memory");
		return 1;
	}

	*account_index = 0;
	*account_len = DEFAULT_ARRAY_SIZE;
	accounts = malloc(sizeof(account*) * (*account_len));

	if(!accounts) {
		print_error("Cannot Allocate Memory");
		return 1;
	}

	//Socket setup
	unsigned int port;

	if(parse_exec_args(argc, argv, &port)) {
		print_error("An error occured, while parsign execution arguments");
		return 1;
	}

	connection_handler_args* chargs = malloc(sizeof(connection_handler_args));

	chargs->port = port;

	if(!chargs) {
		print_error("Cannot Allocate Memeory");
		exit(1);
	}
	
	pthread_t* tid = malloc(sizeof(pthread_t));

	if(!tid) {
		print_error("Cannot Allocate Memory");
		return 1;
	}

	if(pthread_create(tid, NULL, connection_handler, chargs)) {
		print_error("Error Spawning Thread");
		return 1;
	}

	int i;

	while(!is_exiting) {
		sleep(1);
	}

	printf("Exiting\n");

	//Close socket if it exists
	if(sock >= 0) {
		close(sock);
	}

	free(tid);

	return 0;
}
