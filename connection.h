#ifndef CONNECTION_H
#define CONNECTION_H

#include "http.h"

enum connection_state { STATE_REQ, STATE_RES };

/*
 * Connections are stored in a linked list because the structure is big,
 * and connections get added/removed very frequently.
 */
struct connection {
    struct connection *next;
	enum connection_state state;
	union {
		struct http_header header;
		struct http_response res;
	};
};

// Structure is iterable
typedef struct ConnectionList ConnectionList;

ConnectionList *ConnectionList_Create(void);
void ConnectionList_Delete(ConnectionList *list);

// Add to tail
void ConnectionList_Add(ConnectionList *list);

// Iterator functions (may make iterators into a struct if I have more of this type of structure)

// Sets the iterator to the head of the list. Run this before using the iterator!
void ConnectionList_ResetIt(ConnectionList *list);

// Moves to the next element in the list
struct connection *ConnectionList_NextIt(ConnectionList *list);

// Retrieve the current element being iterated on
struct connection *ConnectionList_GetIt(ConnectionList *list);

/*
 * Remove the current element being iterated on.
 * The iterator will be moved to the next element, or become NULL if none are left.
 */
void ConnectionList_RemoveIt(ConnectionList *list);

#endif // CONNECTION_H
