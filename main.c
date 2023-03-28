#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include "config.h"
#include "http.h"
#include "connection.h"
#include "mime-types.h"

int main(void) {
	load_default_config();

	int server_fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (server_fd == -1) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		return 1;
	}

    // Connections will have a 2 minute timeout
	struct timeval timeout = {
		.tv_sec = 120,
		.tv_usec = 0
	};

	// Set options
	setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));

	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_port = global_config.server_port,
		.sin_addr.s_addr = htonl(INADDR_ANY)
	};

	if (bind(server_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
		fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
		return 1;
	}

	if (listen(server_fd, 128) == -1) {
		fprintf(stderr, "Failed to listen to socket: %s\n", strerror(errno));
		return 1;
	}

	ConnectionList *connections = ConnectionList_Create(server_fd);

	for (;;) {
		// if for some reason we fail to poll, PANIC AND DIE
		if (ConnectionList_Poll(connections) == -1) break;
	}

	close(server_fd);
	return 0;
}
