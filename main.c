#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include "config.h"
#include "connection.h"

ConnectionList *connections;
int server_fd;

// Might have more things if the server gets more complex, but probably not.
// I expect myself to keep cleanup simple.
void cleanup(void) {
	ConnectionList_Delete(connections);
	close(server_fd);
}

void term_handler(int signum) {
	fprintf(stderr, "Termination requested, server closing.\n");
	cleanup();
	struct sigaction act = { .sa_handler = SIG_DFL };
	sigaction(signum, &act, NULL);
	raise(signum);
}

// this will accept arguments some day...
int main(void) {
	// register signal handlers
	{
		struct sigaction act = { .sa_handler = &term_handler };
		if (sigaction(SIGTERM, &act, NULL) == -1 || sigaction(SIGINT, &act, NULL) == -1) {
			fprintf(stderr, "Unable to register signal handlers:\n");
			return 1;
		}
	}

	load_default_config();

	server_fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (server_fd == -1) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		return 1;
	}

    // Connections will have a 2 minute timeout
    {
		struct timeval timeout = {
			.tv_sec = 120,
			.tv_usec = 0
		};

		// Set options
		setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
		setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));
    }

	// Bind IP address
	{
		struct sockaddr_in sa = {
			.sin_family = AF_INET,
			.sin_port = global_config.server_port,
			.sin_addr.s_addr = htonl(INADDR_ANY)
		};

		if (bind(server_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
			fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
			return 1;
		}
	}

	if (listen(server_fd, 128) == -1) {
		fprintf(stderr, "Failed to listen to socket: %s\n", strerror(errno));
		return 1;
	}

	connections = ConnectionList_Create(server_fd);

	while (ConnectionList_Poll(connections) != -1);

	// if for some reason we fail to poll, PANIC AND DIE
	fprintf(stderr, "Unrecoverable error, server closing.\n");
	cleanup();
	return 1;
}
