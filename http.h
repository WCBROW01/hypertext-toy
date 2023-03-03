// Data structure definitions for HTTP server, could be used for a C file in future

#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <limits.h>

#include <sys/stat.h>

#include "constants.h"

struct http_header {
    char buf[8192];
    size_t len;
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
 * @brief structure for an HTTP request
 */
struct http_request {
    char path[HTTP_PATH_MAX];
    enum http_request_type request_type;
    int major_version, minor_version;
    int error;
};

/**
 * @brief Parse an HTTP header into a http_request struct
 * 
 * @param http_header string containing the HTTP headerr
 * @return struct containing the HTTP request
 */
struct http_request parse_http_request(char *http_header);

/**
 * @brief A parsed URI, with query separated from path and all percent encoded characters decoded.
 */
struct URI {
    char path[HTTP_PATH_MAX];
    char query[4096];
    struct stat filestat;
    int error;
};

/**
 * @brief Type of HTTP connection.
 */
enum connection_type { CONN_CLOSE, CONN_KEEPALIVE };

/**
 * @brief Structure for an HTTP response
 */
struct http_response {
    int major_version, minor_version;
    int status, connection;
    struct URI uri;
    const char *mime_type;
    size_t header_sent;
    struct http_header header;
    size_t content_length, content_sent;
    FILE *content;
};

/**
 * @brief Create a response object from a request object
 * 
 * @param req the request to use
 * @return the response for your request
 */
struct http_response create_response(struct http_request *req);

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
