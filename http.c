#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "constants.h"
#include "config.h"
#include "mime-types.h"
#include "http.h"

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
 * Returns:
 * 1 if recovery is complete
 * 0 if recovery is incomplete
 * -1 if the end of the header is never found after 8KB
 */
int recv_http_header(int fd, struct http_header *header) {
	ssize_t recv_res;
    while (header->len < sizeof(header->buf) && (recv_res = recv(fd, header->buf + header->len, sizeof(header->buf - header->len), 0)) > 0) {
    	header->len += recv_res;
        if (header->len >= 4 &&
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

struct http_request parse_http_request(char *http_header) {
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
// if an invalid character is found, -1 is returned.
static inline int hextoc(const char *s) {
    int r;
    if (s[0] >= '0' && s[0] <= '9') {
        r = (s[0] - '0') * 16;
    } else if ((s[0] | 32) >= 'a' && (s[0] | 32) <= 'f') {
        r = ((s[0] | 32) - 'a' + 10) * 16;
    } else {
        return -1;
    }

    if (s[1] >= '0' && s[1] <= '9') {
        r += s[1] - '0';
    } else if ((s[1] | 32) >= 'a' && (s[1] | 32) <= 'f') {
        r += (s[1] | 32) - 'a' + 10;
    } else {
        return -1;
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
                int c = hextoc(++uri);
                if (c == -1) { // uh oh
                    if (allocated) free(r);
                    return NULL;
                }
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
	// allocate and zero
    struct URI ret = {0};
    
    if (*path == '/') ++path;
    strtok(path, "?"); // tokenize query
    if (*path == '\0') path = "index.html"; // path is root

    // replace passed in path with decoded path
    path = decode_percent_encoding(path, NULL);
    if (!path) {
        ret.status = 500;
        return ret;
    }

    char pathbuf[4096];
    char *abs_path = realpath(path, pathbuf);
    size_t abs_path_len = abs_path ? strlen(abs_path) : 0;
    free(path); // done with that
    
    // bad path, or someone is trying to be sneaky...
    if (!abs_path) {
    	ret.status = 404;
    	return ret;
    } else if (strncmp(abs_path, global_config.root_path, global_config.root_path_len)) {
    	ret.status = 403;
    	return ret;
    }

    // Path validity check
    if (stat(abs_path, &ret.filestat) == -1) {
        ret.status = 404;
        return ret;
    // figure out if the path is a directory
    } else if (S_ISDIR(ret.filestat.st_mode)) {
        if (abs_path_len < 4085) {
            strcpy(abs_path + abs_path_len, "/index.html");
            
            if (stat(abs_path, &ret.filestat) == -1) {
                // remove /index.html so a directory listing can be done instead
                abs_path[abs_path_len] = '\0';
                ret.status = URI_FOUND_DIR;
            } else {
            	abs_path_len += strlen("/index.html");
            	ret.status = URI_FOUND_FILE;
            }
        } else {
        	ret.status = 404;
        	return ret;
        }
    } else {
    	ret.status = URI_FOUND_FILE;
    }

    // copy correct path to new buffer
    size_t path_len = abs_path_len - global_config.root_path_len;
    ret.path = malloc(path_len + 1);
    if (!ret.path) {
        ret.status = 500;
        return ret;
    }
    
    strncpy(ret.path, pathbuf + global_config.root_path_len, path_len);
    ret.path[path_len] = '\0';

    // decode query
    char *query = strtok(NULL, "?");
    if (query) {
        ret.query = decode_percent_encoding(query, NULL);
        if (!ret.query) {
            ret.status = 500;
            return ret;
        }
    }

    return ret;
}

static void destroy_uri(struct URI *uri) {
	if (uri->path) free(uri->path);
	if (uri->query) free(uri->query);
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

static void create_error_page(struct http_response *res, const char *path) {
    res->content = open_memstream(&res->content_buf, &res->content_length);
    fprintf(
    	res->content,
		"<!DOCTYPE html>"
		"<html>"
			"<head>"
				"<title>%1$d %2$s</title>" // res->status, http_status_str(res->status
			"</head>"
			"<body>"
				"<h1>%1$d %2$s</h1>" // res->status, http_status_str(res->status
				"<p>%3$s</p>" // path
			"</body>"
		"</html>",
		res->status, http_status_str(res->status), path
	);
    fflush(res->content);
}

static void create_dir_listing(struct http_response *res) {
	fprintf(
		res->content,
		"<!DOCTYPE html>"
		"<html>"
			"<head>"
				"<title>Index of %1$s</title>"
			"</head>"
			"<body>"
				"<h1>Index of %1$s</h1>",
		res->uri.path
	);
	
	DIR *dir = opendir(res->uri.path + 1);

	struct dirent *dp;
	while ((dp = readdir(dir))) {
		fprintf(res->content, "<a href=\"%1$s\">%1$s</a><br>", dp->d_name);
	}

	closedir(dir);

	fprintf(res->content, "</body></html>");
	fflush(res->content);
}

struct http_response *create_response(struct http_request *req) {
    struct http_response *res = calloc(1, sizeof(*res));
    if (!res) return NULL;
    *res = (struct http_response) {
        .connection = CONN_CLOSE,
        .mime_type = "text/html", // mime type of error pages
        .header_sent = 0,
        .content_length = 0,
        .content_sent = 0,
        .content_buf = NULL
    };

    if (req->error) {
        res->major_version = 1;
        res->minor_version = 1;
        res->status = req->error;
    } else {
        res->major_version = req->major_version;
        res->minor_version = req->minor_version;
        res->uri = parse_uri(req->path);
    }

    switch (req->request_type) {
        case HTTP_GET: {
            res->connection = CONN_CLOSE;
            switch (res->uri.status) {
            	case URI_FOUND_FILE: {
            		res->status = 200;
            		res->content = fopen(res->uri.path + 1, "r");
            		if (res->content) {
		                res->mime_type = lookup_mime_type(get_file_ext(res->uri.path));
		            	res->content_length = res->uri.filestat.st_size;
		            } else {
		            	res->status = 500;
		            	create_error_page(res, req->path);
		            }
            	} break;
            	case URI_FOUND_DIR: {
            		res->status = 200;
            		res->content = open_memstream(&res->content_buf, &res->content_length);
            		if (res->content) {
            			create_dir_listing(res);
            		} else {
            			res->status = 500;
		            	create_error_page(res, req->path);
            		}
            	} break;
            	default: {
            		res->status = res->uri.status;
	                create_error_page(res, req->path);
            	}
            }
        } break;
        default: {
            res->status = 501;
            create_error_page(res, req->path);
        }
    }

    create_header(res);
    return res;
}

void destroy_response(struct http_response *res) {
    if (!res) return;
	destroy_uri(&res->uri);
	fclose(res->content);
	if (res->content_buf) free(res->content_buf);
	free(res);
}
