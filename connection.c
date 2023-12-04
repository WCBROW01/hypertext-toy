#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/epoll.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "connection.h"
#include "http.h"
#include "callback.h"

#define MAX_EVENTS 128

static int epollfd;
static int server_fd;

// default callbacks and data for new connections
int htt_connection_init(htt_connection_t *conn) {
	conn->callback = &http_request_callback;
	conn->free_func = &free;
	conn->data = calloc(1, sizeof(struct sized_buffer) + 8192);
	if (conn->data) ((struct sized_buffer *) conn->data)->cap = 8192;
	return !conn->data - 1;
}

void htt_connection_close(htt_connection_t *conn) {
	shutdown(conn->fd, SHUT_RDWR);
	close(conn->fd);
	epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, NULL);
	free(conn);
}

void htt_server_init(int fd) {
	server_fd = fd;
	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		fprintf(stderr, "epoll_create1: %s\n", strerror(errno));
		exit(1);
	}
	
	// allocate data for server fd
	htt_connection_t *server_conn = malloc(sizeof(*server_conn));
	if (!server_conn) {
		fprintf(stderr, "Unable to allocate memory for server connection.\n");
		exit(1);
	}
	
	server_conn->fd = server_fd;
	
	// add listening socket to epoll
	struct epoll_event listen_ev = {
		.events = EPOLLIN,
		.data.ptr = server_conn
	};
	
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &listen_ev) == -1) {
		fprintf(stderr, "epoll_ctl: server_fd: %s\n", strerror(errno));
		exit(1);
	}
}

// Return -1 on error, 0 on success
int htt_server_poll(void) {
	struct epoll_event events[MAX_EVENTS];
	
	int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
	if (nfds == -1) {
		fprintf(stderr, "epoll_wait: %s\n", strerror(errno));
		return -1;
	}
	
	for (int i = 0; i < nfds; ++i) {
		htt_connection_t *cdata = events[i].data.ptr;
		
		if (cdata->fd == server_fd) {
			int client_fd = accept(server_fd, NULL, NULL);
			if (client_fd != -1) {
				// set nonblocking
				int flags = fcntl(client_fd, F_GETFL, 0);
				fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
				
				// Connections will have a 2 minute timeout for both send and receive
				// TCP_CORK is also enabled since we are sending a header
			    {
					struct timeval timeout = {
						.tv_sec = 120,
						.tv_usec = 0
					};
					
					setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
					setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

					int tcp_cork_option = 1;
					setsockopt(client_fd, IPPROTO_TCP, TCP_CORK, &tcp_cork_option, sizeof(tcp_cork_option));
			    }
				
				htt_connection_t *conn = malloc(sizeof(*conn));
				conn->fd = client_fd;
				struct epoll_event ev = {
					.events = EPOLLIN | EPOLLOUT,
					.data.ptr = conn
				};
				
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
					fprintf(stderr, "epoll_ctl: client_fd: %s\n", strerror(errno));
					return -1;
				}
				
				htt_connection_init(conn);
			}
		} else if (events[i].events & EPOLLERR) {
			cdata->free_func(cdata->data);
			htt_connection_close(cdata);
		} else {
			cdata->callback(cdata);
		}
	}

	return 0;
}
