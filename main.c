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

    int clients_len = 128; // reasonable default size
	struct pollfd *clients = malloc(clients_len * sizeof(struct pollfd));
	clients[0] = (struct pollfd) {
	    .fd = server_fd,
	    .events = POLLIN,
	    .revents = 0
	};

	ConnectionList *connections = ConnectionList_Create();
	int num_connections = 1;

	for (;;) {
		int connections_closed = 0;

#define CLOSE_CONNECTION() do { \
	++connections_closed; \
	iterate_next = 0; \
	close(clients[i].fd); \
	clients[i].fd = -1; \
	ConnectionList_RemoveIt(connections); \
} while (0)

		int nevents = poll(clients, num_connections, 120000);
		if (nevents > 0) {
			// listening descriptor is readable
			if (clients[0].revents & POLLIN && num_connections < clients_len) {
				int client_fd = accept(server_fd, NULL, NULL);
				if (client_fd != -1) {
					clients[num_connections] = (struct pollfd) {
						.fd = client_fd,
						.events = POLLIN,
						.revents = 0
					};

					ConnectionList_Add(connections);
					++num_connections;
				}
				--nevents;
			}

            ConnectionList_ResetIt(connections);
			for (int i = 1, j = 0; i < num_connections && j < nevents; ++i) {
			    int iterate_next = 1;
				if (clients[i].revents) {
				    struct connection *curr_conn = ConnectionList_GetIt(connections);
					if (clients[i].revents & POLLIN && curr_conn->state == STATE_REQ) {
						int result = recv_http_header(clients[i].fd, &curr_conn->header);
						if (result) {
							curr_conn->state = STATE_RES;
							clients[i].events = POLLOUT;
							struct http_request req = result == 1 ?
								parse_http_request(curr_conn->header.buf) :
								(struct http_request) { .error = 400 };
							curr_conn->res = create_response(&req);
						}
					} else if (clients[i].revents & POLLOUT && curr_conn->state == STATE_RES) {
						if (send_response(clients[i].fd, &curr_conn->res)) {
							fclose(curr_conn->res.content);
							CLOSE_CONNECTION();
						}
					} else { // some error happened
						if (curr_conn->state == STATE_RES) fclose(curr_conn->res.content);
						CLOSE_CONNECTION();
					}
					++j;
				}
				if (iterate_next) ConnectionList_NextIt(connections);
			}
		} else if (nevents == -1) {
			fprintf(stderr, "Error polling events: %s\n", strerror(errno));
			close(server_fd);
			return 1;
		}

		/* Doing this in the loop would've been possible, but very messy.
		 * It would've also delayed response times, which would not be ideal. */
		for (int i = num_connections - 1; i >= 1 && connections_closed > 0; --i) {
			if (clients[i].fd == -1) {
				--num_connections;
				--connections_closed;
				// This could potentially become very slow, I have a solution in mind.
				memmove(clients+i, clients+i+1, (num_connections-i) * sizeof(struct pollfd));
			}
		}

		// Resize clients if necessary
		if (num_connections == clients_len) {
		    struct pollfd *new_array = realloc(clients, clients_len + 128);
		    if (new_array) {
		        clients = new_array;
		        clients_len += 128;
		    }
		} else if (num_connections > 0 && num_connections < clients_len - 128) {
		    struct pollfd *new_array = realloc(clients, clients_len - 128);
		    if (new_array) {
		        clients = new_array;
		        clients_len -= 128;
		    }
		}
	}

	close(server_fd);
	return 0;
}
