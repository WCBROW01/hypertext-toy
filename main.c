#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <netinet/in.h>

#include "config.h"
#include "connection.h"
#include "mime-types.h"

// this will accept arguments some day...
int main(int, char *argv[]) {
	load_default_config();
	
	for (char **arg = argv + 1; *arg; ++arg) parse_config_option(*arg);
	
	load_mime_type_list();

	int server_fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (server_fd == -1) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		return 1;
	}

    // Connections will have a 2 minute timeout for both send and receive
    {
		struct timeval timeout = {
			.tv_sec = 120,
			.tv_usec = 0
		};
		
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
	
	htt_server_init(server_fd);
	while (!htt_server_poll());

	// if for some reason we fail to poll, PANIC AND DIE
	fprintf(stderr, "Unrecoverable error, server closing.\n");
	return 1;
}
