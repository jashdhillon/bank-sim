#include "data.h"

#ifndef PROTOTYPES
#define PROTOTYPES 1
	#define DIAG_DELAY 15

	typedef struct _connection_handler_args {
		int port;
	} connection_handler_args;

	typedef struct _client_handler_args {
		int socket;
	} client_handler_args;

	typedef struct _account {
		char* name;
		double balance;
		double is_active;
	} account;
#endif
