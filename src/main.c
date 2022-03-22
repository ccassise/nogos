#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lobby.h"
#include "log.h"
#include "message_queue.h"
#include "protocol.h"

#define BACKLOG 10

void *get_in_addr(struct sockaddr* sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void add_to_pfds(struct pollfd *pfds[], int newfd, size_t *fd_count, size_t *fd_size) {
	if (*fd_count == *fd_size) {
		*fd_size *= 2;

		pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN;
	(*pfds)[*fd_count].revents = 0;

	(*fd_count)++;
}

void del_from_pfds(struct pollfd pfds[], size_t i, size_t *fd_count) {
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}

typedef struct {
	Lobby *l;
	MessageQueue *msgq;
} Context;

void serve(Context *ctx, Protocol *pro, int fd) {
	int status;
	switch (pro->type) {
	case PRO_JOIN:
		status = lobby_join(ctx->l, fd);
		break;
	case PRO_LEAVE:
		status = lobby_leave(ctx->l, fd);
		break;
	case PRO_MOVE:
		status = lobby_play_move(ctx->l, fd, pro->arg1, pro->arg2);
		break;
	case PRO_ERROR:
	default:
		status = -1;
	}

	if (status == -1) {
		msgq_put(ctx->msgq, &(Message){ .to = { fd }, .to_count = 1, .data = "ERROR\r\n", .data_count = 7 });
	} else {
		msgq_put(ctx->msgq, &(Message){ .to = { fd }, .to_count = 1, .data = "OK\r\n", .data_count = 4 });
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("usage: nogos [port]\n");
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status;
	struct addrinfo *servinfo;
	if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	int listener;
	for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		int yes = 1;
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
			close(listener);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (listen(listener, BACKLOG) == -1) {
		perror("server: listen");
		exit(1);
	}

	printf("Listening on %s\n", argv[1]);


	size_t fd_count = 0;
	size_t fd_size = 5;
	struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

	add_to_pfds(&pfds, listener, &fd_count, &fd_size);

	MessageQueue *msgq = msgq_create();
	Context ctx = { .l = lobby_create(msgq), .msgq = msgq };
	if (!msgq) {
		fprintf(stderr, "failed to allocate memory for MessageQueue\n");
		exit(1);
	}

	for (;;) {
		int poll_checked = 0;  // Number of current poll events handled.
		int poll_count = poll(pfds, fd_count, -1);
		if (poll_count == -1) {
			perror("poll");
			break;
		}

		for (size_t i = 0; i < fd_count && poll_checked < poll_count; i++) {
			if (pfds[i].revents & POLLIN) {
				poll_checked++;

				if (pfds[i].fd == listener) {
					struct sockaddr_storage remoteaddr;
					socklen_t addrlen = sizeof remoteaddr;

					int newfd = accept(listener, (struct sockaddr*)&remoteaddr, &addrlen);
					
					if (newfd == -1) {
						perror("accept");
					} else {
						add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

						char remote_ip[INET6_ADDRSTRLEN];
						printf("new connection: %s %d on socket: %d\n",
							inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),
							remote_ip,
							INET6_ADDRSTRLEN),
							newfd,
							newfd);

	  					msgq_put(msgq, &(Message){ .to = { newfd }, .to_count = 1, .data = "OK\r\n", .data_count = 4 });
					}
				} else {
					int sender_fd = pfds[i].fd;

					char buf[MSG_MAX_SIZE];
					int buf_count = (int)recv(sender_fd, buf, MSG_MAX_SIZE - 1, 0);
					
					if (buf_count <= 0) {
						if (buf_count == 0) {
							printf("socket closed: %d\n", pfds[i].fd);
						} else {
							perror("recv");
						}

						lobby_leave(ctx.l, pfds[i].fd);

						close(pfds[i].fd);
						del_from_pfds(pfds, i, &fd_count);
					} else {
						Protocol pro = pro_parse(fmemopen(buf, (size_t)buf_count, "r"));
						DEBUG("[%d] parse: %d '%s' '%s'\n", sender_fd, pro.type, pro.arg1, pro.arg2);
						serve(&ctx, &pro, sender_fd);
					}
				}
			}
		}

		// Send all messages in queue.
		while (!msgq_empty(msgq)) {
			Message tmp = msgq_get(msgq);

			for (int k = 0; k < tmp.to_count; k++) {
				if (send(tmp.to[k], &tmp.data, (size_t)tmp.data_count, 0) == -1) {
					perror("send");
				}
			}
		}
	}

	printf("Connection closed\n");

	free(pfds);
	msgq_destroy(msgq);

	return 0;
}
