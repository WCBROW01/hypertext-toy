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

#include "http.h"
#include "connection.h"
#include "mime-types.h"

static char working_path[4096];
static size_t working_path_len;

// Returns: the index of the string, or -1 upon error.
static inline ssize_t search_string_enum_table(
    const char *str, const char **table, size_t size
)
{
    for (ssize_t i = 0; i < (ssize_t) size; ++i) {
        if (!strcmp(str, table[i])) return i;
    }
    return -1;
}

/*
* All the HTTP stuff may move into a file named http.c
* (if I can figure out how to share the working path + length without globals,
* that may just be what has to happen.)
*/

/*
 * Returns:
 * 1 if recovery is complete
 * 0 if recovery is incomplete
 * -1 if the end of the header is never found after 8KB
 */
static int recv_http_header(int fd, struct http_header *header) {
    while (header->len < sizeof(header->buf) && recv(fd, header->buf + header->len, 1, 0) > 0) {
        if (++header->len >= 4 &&
            // look for the end of the header
            header->buf[header->len - 4] == '\r' &&
            header->buf[header->len - 3] == '\n' &&
            header->buf[header->len - 2] == '\r' &&
            header->buf[header->len - 1] == '\n')
        {
            return 1;
        }
    }

    return header->len == sizeof(header->buf) ? -1 : 0;
}

static struct http_request parse_http_request(char *http_header) {
    struct http_request res = { .error = 0 };

    char request_type[8];
    int http_result = sscanf(http_header, "%7s %4095s HTTP/%d.%d\r\n", request_type, res.path, &res.major_version, &res.minor_version);
    if (http_result == 1) {
        fprintf(stderr, "URI too long\n");
        res.error = 414;
    } else if (http_result != 4) {
        fprintf(stderr, "Not a valid HTTP header\n");
        res.error = 400;
    }

    printf("Recieved %s request for path %s using HTTP %d.%d\n", request_type, res.path, res.major_version, res.minor_version);

    const char *REQUEST_TYPE_TABLE[] = {
        "GET", "HEAD", "POST", "PUT", "DELETE",
        "CONNECT", "OPTIONS", "TRACE", "PATCH"
    };

    res.request_type = search_string_enum_table(request_type, REQUEST_TYPE_TABLE, sizeof(REQUEST_TYPE_TABLE) / sizeof(REQUEST_TYPE_TABLE[0]));

	return res;
}

// decode hexadecimal characters from string
static inline int hextoc(const char *s) {
    int r = 0;
    if (s[0] >= '0' && s[0] <= '9') {
        r = (s[0] - '0') * 16;
    } else if ((s[0] | 32) >= 'a' && (s[0] | 32) <= 'f') {
        r = ((s[0] | 32) - 'a' + 10) * 16;
    }

    if (s[1] >= '0' && s[1] <= '9') {
        r += s[1] - '0';
    } else if ((s[1] | 32) >= 'a' && (s[1] | 32) <= 'f') {
        r += (s[1] | 32) - 'a' + 10;
    }

    return r;
}

/*
 * If a buffer is given, the decoded URI is copied into it.
 * This assumes that you know your buffer is large enough for it.
 */
static char *decode_percent_encoding(const char *uri, char *buf) {
    if (!uri) return NULL;

    int allocated = 0;
    char *r;
    if (!buf) {
        allocated = 1;
        size_t len = 0;
        for (const char *s = uri; *s; ++len, ++s) {
            if (*s == '%') s += 2;
        }

        r = malloc(len + 1);
        if (!r) return NULL;
    } else r = buf;
    
    {
        char *s;
        for (s = r; *uri; ++s, ++uri) {
            if (*uri == '%') {
                if (uri[1] == '\0' || uri[2] == '\0') { // uh oh
                    if (allocated) free(r);
                    return NULL;
                }
                int c = hextoc(++uri);
                ++uri;
                if (c) *s = c; // watch out for null chars!
                else --s;
            } else *s = *uri;
        }
        *s = '\0';
    }

    return r;
}

// This mutates the string we give to it, but that's fine.
static struct URI parse_uri(char *path) {
    struct URI ret = {.error = 0};
    if (*path == '/') ++path;
    strtok(path, "?"); // tokenize query
    if (*path == '\0') path = "index.html"; // path is root

    // replace passed in path with decoded path
    path = decode_percent_encoding(path, NULL);
    if (!path) {
        ret.error = 500;
        return ret;
    }

    char pathbuf[PATH_MAX];
    char *abs_path = realpath(path, pathbuf);
    // bad path, or someone is trying to be sneaky...
    if (!abs_path) ret.error = 404;
    else if (strncmp(abs_path, working_path, working_path_len)) ret.error = 403;

    // Path validity check
    if (stat(abs_path, &ret.filestat) == -1) {
        ret.error = 404;
    // figure out if the path is a directory
    } else if (S_ISDIR(ret.filestat.st_mode)) {
        size_t len = strlen(abs_path);
        if (len < PATH_MAX - 10) {
            strcpy(abs_path + len, "/index.html");
            if (stat(abs_path, &ret.filestat) == -1)
                ret.error = 404;
        } else ret.error = 404;
    }

    // copy correct path
    strncpy(ret.path, pathbuf + working_path_len, PATH_MAX - working_path_len);

    // decode query
    decode_percent_encoding(strtok(NULL, "?"), ret.query);

    free(path);
    return ret;
}

static const char *http_status_str(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 414: return "URI Too Long";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default: return "Internal Server Error";
    }
}

static void create_header(struct http_response *res) {
    const char *CONN_TYPE_TABLE[] = {"close", "keep-alive"};

    res->header.len = snprintf(
        res->header.buf, sizeof(res->header.buf),
        "HTTP/%d.%d %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: %s\r\n\r\n",
        res->major_version, res->minor_version, res->status, http_status_str(res->status),
        res->mime_type ? res->mime_type : "", // if there is none, just don't send a mime type
        res->content_length,
        CONN_TYPE_TABLE[res->connection]
    );
}

static void create_error_page(struct http_response *res) {
    res->content = fmemopen(NULL, 8192, "w+");
    res->content_length = fprintf(res->content, "%d %s", res->status, http_status_str(res->status));
}

static struct http_response create_response(struct http_request *req) {
    struct http_response res = {
        .connection = CONN_CLOSE,
        .mime_type = "text/html", // mime type of error pages
        .header_sent = 0,
        .content_sent = 0
    };

    if (req->error) {
        res.major_version = 1;
        res.minor_version = 1;
        res.status = req->error;
    } else {
        res.major_version = req->major_version;
        res.minor_version = req->minor_version;
        res.uri = parse_uri(req->path);
    }

    switch (req->request_type) {
        case HTTP_GET: {
            res.connection = CONN_CLOSE;
            if (!res.uri.error) {
                res.status = 200;
                res.content = fopen(res.uri.path + 1, "r");
                if (res.content) {
                    res.mime_type = lookup_mime_type(get_file_ext(res.uri.path));
                	res.content_length = res.uri.filestat.st_size;
                } else {
                	res.status = 500;
                	create_error_page(&res);
                }
            } else {
                res.status = res.uri.error;
                create_error_page(&res);
            }
        } break;
        default: {
            res.status = 501;
            create_error_page(&res);
        }
    }

    create_header(&res);
    return res;
}

// 0 if incomplete, 1 if complete
static int send_response(int fd, struct http_response *res) {
    if (res->header_sent < res->header.len) {
        res->header_sent += send(fd, res->header.buf + res->header_sent, res->header.len - res->header_sent, 0);
        return 0;
    } else if (res->content_sent < res->content_length) {
        fseek(res->content, res->content_sent, SEEK_SET);
        char sendbuf[4096];
        size_t buflen = fread(sendbuf, 1, sizeof(sendbuf), res->content);
        res->content_sent += send(fd, sendbuf, buflen, 0);
    }

    return res->content_sent == res->content_length;
}

int main(void) {
    // cwd is working path (for now)
    if (!getcwd(working_path, sizeof(working_path))) {
    	fprintf(stderr, "Failed to get current working directory: %s\n", strerror(errno));
    	return 1;
    }

    working_path_len = strlen(working_path);

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
		.sin_port = htons(8000),
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
