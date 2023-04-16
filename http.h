/**
 * @file http.h
 * @author Will Brown
 * @brief Data structures for HTTP server
 * @version 0.1
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <limits.h>

#include <sys/stat.h>

#include "constants.h"

/**
 * @brief Structure for a HTTP header
 */
struct http_header {
    char buf[8192]; ///< Buffer for the header (max 8192 bytes)
    size_t len; ///< Actual length of the header
};

/**
 * @brief Recover HTTP header from a socket.
 * @details This will recover chunks of the header until there is no more to recover.
 * Keep calling this until you get a nonzero return value.
 *
 * @param fd file descriptor of the socket
 * @param header pointer to the header in memory
 * @return 1 if recovery is complete,
 * 0 if recovery is incomplete,
 * -1 if the end of the header is never found after 8KB
 */
int recv_http_header(int fd, struct http_header *header);

/**
 * @brief enumeration of the types of HTTP requests
 */
enum http_request_type {
    HTTP_GET, HTTP_HEAD, HTTP_POST, HTTP_PUT, HTTP_DELETE,
    HTTP_CONNECT, HTTP_OPTIONS, HTTP_TRACE, HTTP_PATCH
};

/**
 * @brief Structure for a HTTP request
 */
struct http_request {
    char path[HTTP_PATH_MAX]; ///< Path given in the request
    enum http_request_type request_type; ///< Type of request
    int major_version; ///< Major HTTP version
    int minor_version; ///< Minor HTTP version
    int error; ///< Error code for the HTTP request (if applicable)
};

/**
 * @brief Parse a HTTP header into a http_request struct
 *
 * @param http_header string containing the HTTP header
 * @return struct containing the HTTP request
 */
struct http_request parse_http_request(char *http_header);

/**
 * @brief Type of HTTP connection.
 */
enum connection_type { CONN_CLOSE, CONN_KEEPALIVE };

/**
 * @brief A parsed URI
 */
typedef struct URI URI;

/**
 * @brief Structure for a HTTP response
 */
struct http_response {
    int major_version; ///< Major HTTP version
    int minor_version; ///< Minor HTTP version
    int status; ///< HTTP status
    enum connection_type connection; ///< Type of connection
    URI *uri; ///< URI of the file to serve
    const char *mime_type; ///< MIME type of the file
    size_t header_sent; ///< Number of bytes of the header sent
    struct http_header header; ///< Header data
    size_t content_length; ///< Length of the content section of the response
    size_t content_sent; ///< Number of bytes of content sent
    FILE *content; ///< stdio FILE pointer to the content
};

/**
 * @brief Create a http_response struct from a http_request struct
 *
 * @param req the request to use
 * @return the response for your request
 */
struct http_response *create_response(struct http_request *req);

/**
 * @brief Free all memory associated with a http_response struct
 *
 * @param res the response to free
 */
void destroy_response(struct http_response *res);

/**
 * @brief Send a HTTP response over a socket
 * @details This will send chunks of the response until there is no more to send.
 * Keep calling this until you get a nonzero return value.
 *
 * @param fd file descriptor of the socket
 * @param res the response to send
 * @return 0 if incomplete, 1 if complete
 */
int send_response(int fd, struct http_response *res);

#endif // HTTP_H
