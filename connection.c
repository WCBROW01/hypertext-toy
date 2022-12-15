#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"

// Structure is iterable
struct ConnectionList {
    struct connection *head;
    struct connection *tail;
    struct connection *last;
    struct connection *it;
};

ConnectionList *ConnectionList_Create(void) {
    ConnectionList *ret = malloc(sizeof(ConnectionList));
    ret->head = ret->tail = ret->it = NULL;
    return ret;
}

void ConnectionList_Delete(ConnectionList *list) {
    while (list->head) {
        struct connection *removed = list->head;
        list->head = list->head->next;
        free(removed);
    }

    free(list);
}

void ConnectionList_Add(ConnectionList *list) {
    struct connection *new_conn = malloc(sizeof(struct connection));
    memset(new_conn, 0, sizeof(struct connection));
    if (!list->head) {
        list->head = list->tail = list->it = new_conn;
    } else {
        list->tail->next = new_conn;
        list->tail = new_conn;
    }
}

void ConnectionList_ResetIt(ConnectionList *list) {
    list->it = list->head;
}

struct connection *ConnectionList_NextIt(ConnectionList *list) {
    list->last = list->it;
    if (list->it) list->it = list->it->next;
    return list->it;
}

struct connection *ConnectionList_GetIt(ConnectionList *list) {
    return list->it;
}

void ConnectionList_RemoveIt(ConnectionList *list) {
    if (!list->it) return;

    struct connection *removed = list->it;

    if (list->head == list->tail) {
        list->head = list->tail = list->it = NULL;
    } else if (list->it == list->head) {
        list->head = list->head->next;
        list->it = list->head;
    } else if (list->it == list->tail) {
        list->tail = list->last;
        list->tail->next = NULL;
        list->it = NULL;
    } else {
        list->last->next = list->it->next;
        list->it = list->it->next;
    }

    free(removed);
}
