#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "callback.h"
#include "connection.h"
#include "http.h"

int http_request_callback(htt_connection_t *conn) {
	struct http_header *header = conn->data;
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
    while ((send_result = send(conn->fd, res->header.buf + res->header_sent, res->header.len - res->header_sent, 0)) > 0) {
        res->header_sent += send_result;
    }

	if (res->header_sent == res->header.len) {
		conn->callback = &http_response_content_callback;
	}
	
	return 0;
}

int http_response_content_callback(htt_connection_t *conn) {
	struct http_response *res = conn->data;

	ssize_t send_result;
	do {
        fseek(res->content, res->content_sent, SEEK_SET);
        char sendbuf[4096];
        size_t buflen = fread(sendbuf, 1, sizeof(sendbuf), res->content);
        send_result = send(conn->fd, sendbuf, buflen, 0);
        if (send_result > 0) res->content_sent += send_result;
    } while (send_result > 0);

	if (res->content_sent == res->content_length) {
		destroy_response(conn->data);
		htt_connection_close(conn);
	}
	
	return 0;
}
