/**
 * @file connection.h
 * @author Will Brown
 * @brief Data structure for managing HTTP connections
 * @version 0.1
 * @date 2023-04-02
 *
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "http.h"

/**
 * @brief enum denoting whether the connection is in the request or response stage
 */
enum connection_state { STATE_REQ, STATE_RES, STATE_END };

/**
 * @brief A node in the list of connections.
 */
struct connection {
	enum connection_state state;
	union {
		struct http_header header;
		struct http_response res;
	};
};

/**
 * @brief List of connections
 * @details Connections are stored as an array of pointers to a connection,
 * because the structure is big, and connections get added/removed very frequently.
 * This was previously a linked list, but I wanted a direct mapping between clients and connections.
 * The internal structure of the list is abstracted away so that it cannot be relied upon,
 * and is subject to change at any moment.
 */
typedef struct ConnectionList ConnectionList;

/**
 * @brief Create an instance of a ConnectionList
 *
 * @param server_fd the file descriptor for the server
 * @return a pointer to the new ConnectionList
 */
ConnectionList *ConnectionList_Create(int server_fd);

/**
 * @brief Delete an instance of a ConnectionList
 *
 * @param list the list to delete
 */
void ConnectionList_Delete(ConnectionList *list);

/**
 * @brief Poll and handle requests on a connection list
 *
 * @param list the list to poll
 * @param -1 on error, 0 on success
 */
int ConnectionList_Poll(ConnectionList *list);

#endif // CONNECTION_H
