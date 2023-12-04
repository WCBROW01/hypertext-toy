#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>

#include "callback.h"
#include "connection.h"
#include "http.h"

int http_request_callback(htt_connection_t *conn) {
	struct sized_buffer *header = conn->data;
	int recv_res = recv_http_header(conn->fd, header);
	if (recv_res) {
		struct http_request req = recv_res == 1 ?
			parse_http_request(header->buf) :
			(struct http_request) { .error = 400 };
		free(header);
		conn->data = create_response(&req);
		if (!conn->data) {
			htt_connection_close(conn);
			return -1;
		} else {
			conn->callback = &http_response_header_callback;
			conn->free_func = &destroy_response;
		}
	}
	
	return 0;
}

int http_response_header_callback(htt_connection_t *conn) {
	struct http_response *res = conn->data;

	ssize_t send_result;
    while ((send_result = write(conn->fd, res->header_buf + res->header_sent, res->header_length - res->header_sent)) > 0) {
        res->header_sent += send_result;
    }

	if (res->header_sent == res->header_length) {
		if (res->content_length) conn->callback = &http_response_content_callback;
		// no content, end connection
		else {
			destroy_response(conn->data);
			htt_connection_close(conn);
		}
	}
	
	return 0;
}

int http_response_content_callback(htt_connection_t *conn) {
	struct http_response *res = conn->data;

	ssize_t send_result;
	do {
		if (res->content_buf) {
			send_result = write(conn->fd, res->content_buf, res->content_length - res->content_sent);
			if (send_result > 0) res->content_sent += send_result;
		} else {
			send_result = sendfile(conn->fd, res->content, &res->content_sent, res->content_length - res->content_sent);
		}
	} while (send_result > 0);

	if (res->content_sent == res->content_length) {
		destroy_response(conn->data);
		htt_connection_close(conn);
	}
	
	return 0;
}
