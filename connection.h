/**
 * @file connection.h
 * @author Will Brown
 * @brief Functions and data structures related to the http server and connections
 * @version 0.1
 * @date 2023-08-31
 *
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef CONNECTION_H
#define CONNECTION_H

typedef struct htt_connection htt_connection_t;

/**
 * @brief function pointer type for callbacks
 */
typedef int (*htt_callback_t)(htt_connection_t *connection);

/**
 * @brief function pointer type for freeing data
 */
typedef void (*htt_free_t)(void *data);

/**
 * @brief The data for a connection
 */
struct htt_connection {
    htt_callback_t callback;
    htt_free_t free_func; // used if an error occurred
    void *data;
    int fd;
};

/**
 * @brief assign default callbacks and data for new connections
 *
 * @param conn connection to initialize
 * @return 0 on success, -1 on failure
 */
int htt_connection_init(htt_connection_t *conn);

/**
 * @brief close a connection
 *
 * @param conn connection to close
 */
void htt_connection_close(htt_connection_t *conn);

/**
 * @brief initialize web server
 */
void htt_server_init(int server_fd);

/**
 * @brief poll connections
 *
 * @return 0, unless the server encountered an error.
 */
int htt_server_poll(void);

#endif // CONNECTION_H
