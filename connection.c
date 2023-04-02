/**
 * @file connection.c
 * @author Will Brown
 * @brief Code for managing HTTP connections
 * @version 0.1
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023 Will Brown
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>

#include "connection.h"
#include "http.h"

/**
 * @brief List of connections
 */
struct ConnectionList {
	struct pollfd *clients; ///< list of clients for pollfd
	struct connection **connections; ///< contain pointers to connections (to save memory, the struct is pretty huge)
	int connections_len; ///< length of all arrays
	int connections_end; ///< end of the connections array
	int num_connections; ///< current number of connections
	int *complete_connections; ///< indices of complete connections (to reuse)
	int num_complete_connections; ///< number of complete connections
	int complete_connections_start; ///< start of the complete connection queue
};


ConnectionList *ConnectionList_Create(int server_fd) {
    ConnectionList *list = malloc(sizeof(ConnectionList));
    list->connections_len = 128; // reasonable default
    list->connections_end = 1;
    list->clients = malloc(list->connections_len * sizeof(*list->clients));
    list->connections = malloc(list->connections_len * sizeof(*list->connections));
    list->complete_connections = malloc(list->connections_len * sizeof(*list->complete_connections));
    list->num_complete_connections = 0;
    list->complete_connections_start = 0;

    list->clients[0] = (struct pollfd) {
	    .fd = server_fd,
	    .events = POLLIN,
	    .revents = 0
	};

	list->num_connections = 1;

    return list;
}

static void ConnectionList_Remove(ConnectionList *list, int index) {
	shutdown(list->clients[index].fd, SHUT_RDWR);
	close(list->clients[index].fd);
	list->clients[index].fd = -1;
	printf("Freeing connection %d\n", index);
	free(list->connections[index]);
	list->connections[index] = NULL; // not specifically required, but will be useful on desctruction
	list->complete_connections[(list->complete_connections_start + list->num_complete_connections) & (list->connections_len - 1)] = index;
	++list->num_complete_connections;
	--list->num_connections;
}

void ConnectionList_Delete(ConnectionList *list) {
	--list->num_connections;
	// close all connections (the operating system will not completely clean up for us since there are sockets involved)
	for (int i = 1; list->num_connections; ++i)
		if (list->connections[i]) ConnectionList_Remove(list, i);

	free(list->clients);
	free(list->connections);
	free(list->complete_connections);
	free(list);
}

// If the realloc partially succeeds, then that isn't really harmful.
static int ConnectionList_Resize(ConnectionList *list) {
	int new_len = list->connections_len << 1; // double size of arrays
	void *realloc_res;
	realloc_res = realloc(list->clients, new_len * sizeof(*list->clients));
	if (!realloc_res) return 0;
	else list->clients = realloc_res;
	realloc_res = realloc(list->connections, new_len * sizeof(*list->connections));
	if (!realloc_res) return 0;
	else list->connections = realloc_res;
	realloc_res = realloc(list->complete_connections, new_len * sizeof(*list->complete_connections));
	if (!realloc_res) return 0;
	else list->connections = realloc_res;
	list->connections_len = new_len;
	return 1;
}

static int ConnectionList_Add(ConnectionList *list, int client_fd) {
	// Try allocating memory before computing anything to avoid possible leaks
	struct connection *new_connection = malloc(sizeof(*new_connection));
	if (!new_connection) return 0;
	memset(new_connection, 0, sizeof(*new_connection));

	int index;
	if (list->num_complete_connections > 0) {
		index = list->complete_connections[list->complete_connections_start++];
		list->complete_connections_start &= list->connections_len - 1;
		--list->num_complete_connections;
	} else {
		index = list->connections_end++;
	}

	list->connections[index] = new_connection;
	++list->num_connections;

    list->clients[index] = (struct pollfd) {
		.fd = client_fd,
		.events = POLLIN,
		.revents = 0
	};

	return 1;
}

// Return -1 on error, 0 on success
int ConnectionList_Poll(ConnectionList *list) {
	if (list->num_connections == list->connections_len) ConnectionList_Resize(list);

	int nevents = poll(list->clients, list->connections_end, 120000);
	if (nevents > 0) {
		// listening descriptor is readable
		if (list->clients[0].revents & POLLIN && list->num_connections < list->connections_len) {
			int client_fd = accept(list->clients[0].fd, NULL, NULL);
			if (client_fd != -1) ConnectionList_Add(list, client_fd);
			--nevents;
		}

		for (int i = 1; nevents; ++i) {
			if (list->clients[i].revents) {
			    struct connection *curr_conn = list->connections[i];
				if (list->clients[i].revents & POLLIN && curr_conn->state == STATE_REQ) {
					int result = recv_http_header(list->clients[i].fd, &curr_conn->header);
					if (result) {
						curr_conn->state = STATE_RES;
						list->clients[i].events = POLLOUT;
						struct http_request req = result == 1 ?
							parse_http_request(curr_conn->header.buf) :
							(struct http_request) { .error = 400 };
						curr_conn->res = create_response(&req);
					}
				} else if (list->clients[i].revents & POLLOUT && curr_conn->state == STATE_RES) {
					if (send_response(list->clients[i].fd, &curr_conn->res)) {
						fclose(curr_conn->res.content);
						ConnectionList_Remove(list, i);
					}
				} else { // some error happened
					if (curr_conn->state == STATE_RES) fclose(curr_conn->res.content);
					ConnectionList_Remove(list, i);
				}
				--nevents;
			}
		}
	} else if (nevents == -1) {
		fprintf(stderr, "Error polling events: %s\n", strerror(errno));
		return -1; // oh no
	}

	return 0;
}
