/**
 * @file connection.h
 * @author Will Brown
 * @brief Data structure for managing HTTP connections
 * @version 0.1
 * @date 2023-03-03
 * 
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "http.h"

/**
 * @brief enum denoting whether the connection is in the request or response stage
 */
enum connection_state { STATE_REQ, STATE_RES };

/*
 * Connections are stored in a linked list because the structure is big,
 * and connections get added/removed very frequently.
 */

/**
 * @brief A node in the linked list of connections.
 */
struct connection {
    struct connection *next;
	enum connection_state state;
	union {
		struct http_header header;
		struct http_response res;
	};
};

/**
 * @brief Iterable linked list of connections
 */
typedef struct ConnectionList ConnectionList;

/**
 * @brief Create an instance of a ConnectionList
 * 
 * @return a pointer to the new ConnectionList
 */
ConnectionList *ConnectionList_Create(void);

/**
 * @brief Delete an instance of a ConnectionList
 * 
 * @param list the list to delete
 */
void ConnectionList_Delete(ConnectionList *list);

// Add to tail
/**
 * @brief Add a new connection to the tail of the connection list
 * 
 * @param list the list to add to
 */
void ConnectionList_Add(ConnectionList *list);

// Iterator functions (may make iterators into a struct if I have more of this type of structure)

/**
 * @brief Sets the iterator to the head of the list. Run this before using the iterator!
 * 
 * @param list the list to reset
 */
void ConnectionList_ResetIt(ConnectionList *list);

/**
 * @brief Moves to the next element in the list
 * 
 * @param list the list being iterated on
 * @return the element at the new position of the iterator
 */
struct connection *ConnectionList_NextIt(ConnectionList *list);

/**
 * @brief Retrieve the current element being iterated on
 * 
 * @param list the list being iterated on
 * @return the element at the current position of the iterator
 */
struct connection *ConnectionList_GetIt(ConnectionList *list);

/**
 * @brief Remove the current element being iterated on.
 * The iterator will be moved to the next element, or become NULL if none are left.
 * 
 * @param list the list being iterated on
 */
void ConnectionList_RemoveIt(ConnectionList *list);

#endif // CONNECTION_H
