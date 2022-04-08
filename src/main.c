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

#include "libnogo/nogo.h"

#include "context.h"
#include "lobby.h"
#include "log.h"
#include "message.h"
#include "player.h"
#include "queue.h"

#define BACKLOG 10

#define RESPONSE_SIZE 512

/**
 * @brief Sends a message to all players in the current lobby.
 * 
 * @param l The lobby that the message will be sent to.
 * @param str The string that will be sent. If str has a terminating null
 * character it will be discarded.
 * @param slen The length of the string that will be sent.
 * @return -1 if the message failed to write. 0 otherwise.
 */
static int broadcast_all(Lobby *l, const char *str, size_t slen) {
	int result = 0;

	for (int i = 0; i < l->players_len; i++) {
		long wrote = l->players[i].write(&l->players[i], str, slen);
		if (wrote <= 0) {
			result = -1;
		}
	}

	return result;
}

/**
 * @brief Sends a message to all players in the current lobby except for the
 * given player.
 * 
 * @param l The lobby that the message will be sent to.
 * @param str The string that will be sent. If str has a terminating null
 * character it will be discarded.
 * @param slen The length of the string that will be sent.
 * @param playerfd The file descriptor of the player.
 * @return -1 if the message failed to write. 0 otherwise.
 */
static int broadcast_from(Lobby *l, const char *str, size_t slen, int playerfd) {
	int result = 0;

	for (int i = 0; i < l->players_len; i++) {
		if (l->players[i].fd != playerfd) {
			long wrote = l->players[i].write(&l->players[i], str, slen);
			if (wrote <= 0) {
				result = -1;
			}
		}
	}

	return result;
}

static long write_ok(const Player *player) {
	return player->write(player, "OK\r\n", 4);
}

/**
 * @brief Writes error and given message to player. The message should be terminated with CRLF.
 * 
 * @param player The player to write to.
 * @param msg The CRLF terminated message to include with the error. Should be NULL if no message is being sent.
 * @return long How many bytes were written. -1 on errors.
 */
static long write_error(const Player *player, const void *msg) {
	if (msg == NULL) {
		return player->write(player, "ERROR\r\n", 7);
	}

	#define ERROR_MSG_SIZE 128
	#define ERROR_LEN 5

	char error_msg[ERROR_MSG_SIZE] = "ERROR";
	strncat(error_msg + ERROR_LEN, msg, ERROR_MSG_SIZE - ERROR_LEN);

	#undef ERROR_MSG_SIZE
	#undef ERROR_LEN
	return player->write(player, error_msg, strlen(error_msg));
}

static void *get_in_addr(struct sockaddr_storage* ss) {
	if (ss->ss_family == AF_INET) {
		return &(((struct sockaddr_in*)ss)->sin_addr);
	}

	return &(((struct sockaddr_in6*)ss)->sin6_addr);
}

static int serve_pro_join(Context *ctx, NogoProtocol *pro, Player *player) {
	(void)pro;

	int result;
	if ((result = lobby_join(ctx->l, player)) < 0 ) {
		return result;
	}

	char buf[RESPONSE_SIZE];
	int buf_size = snprintf(buf, RESPONSE_SIZE, "GOTJOIN %s\r\n", player->name);
	if (buf_size <= 0) {
		LOG_ERROR("failed to create gotjoin message\n");
		return -1;
	}

	if (broadcast_from(ctx->l, buf, (size_t)buf_size, player->fd) < 0) {
		LOG_ERROR("failed to broadcast gotjoin from player\n");
		return -1;
	}

	return result;
}

static int serve_pro_move(Context *ctx, NogoProtocol *pro, Player *player) {
	if (lobby_play_move(ctx->l, player, pro->arg1, pro->arg2) < 0) {
		return -1;
	}

	LOG_DEBUG("[%s<%d>] played move %s %s\n", player->name, player->fd, pro->arg1, pro->arg2);

	write_ok(player);

	char buf[RESPONSE_SIZE];
	int buf_size = snprintf(buf, RESPONSE_SIZE, "GOTMOVE %s %s\r\n", pro->arg1, pro->arg2);
	if (buf_size <= 0) {
		LOG_ERROR("failed to create gotmove message\n");
		return -1;
	}

	if (broadcast_from(ctx->l, buf, (size_t)buf_size, player->fd) < 0) {
		LOG_ERROR("failed to broadcast gotmove from player\n");
		return -1;
	}

	int team;
	if ((team = lobby_winner(ctx->l)) != -1) {
		buf_size = snprintf(buf, RESPONSE_SIZE, "GOTWINNER %c\r\n", team);
		if (buf_size <= 0) {
			LOG_ERROR("failed to create gotwinner message\n");
			return -1;
		}

		if (broadcast_all(ctx->l, buf, (size_t)buf_size) < 0) {
			LOG_ERROR("failed to broadcast gotwinner to all\n");
			return -1;
		}
	}

	return 0;
}

static void serve(Context *ctx, NogoProtocol *pro, Player *player) {
	if (!player->is_login && pro->type != NOGO_PRO_LOGIN && pro->type != NOGO_PRO_LOGOUT) {
		pro->type = NOGO_PRO_ERROR;
	}

	int status;
	switch (pro->type) {
	case NOGO_PRO_JOIN:
		LOG_DEBUG("[%s<%d>] joined a lobby\n", player->name, player->fd);

		status = serve_pro_join(ctx, pro, player);
		break;
	case NOGO_PRO_LEAVE:
		LOG_DEBUG("[%s<%d>] left a lobby\n", player->name, player->fd);

		lobby_leave(ctx->l, player);

		const char left[] = "GOTLEAVE\r\n";
		broadcast_from(ctx->l, left, (sizeof left / sizeof left[0]) - 1, player->fd);

		status = 0;
		break;
	case NOGO_PRO_LOGIN:
		memset(player->name, '\0', PLAYER_NAME_SIZE);
		memcpy(player->name, pro->arg1, PLAYER_NAME_SIZE - 1);
		player->is_login = true;

		LOG_DEBUG("[%s<%d>] login\n", player->name, player->fd);

		status = 0;
		break;
	case NOGO_PRO_LOGOUT:
		LOG_DEBUG("[%s<%d>] logout\n", player->name, player->fd);

		player->is_login = false;
		queue_put(ctx->closeq, &player->fd);
		lobby_leave(ctx->l, player);
		ctx_remove_player(ctx, player->fd);
		status = 0;
		break;
	case NOGO_PRO_MOVE:
		status = serve_pro_move(ctx, pro, player);
		break;
	case NOGO_PRO_ERROR:
	default:
		status = -1;
	}

	if (status == -1) {
		write_error(player, NULL);
	} else if (pro->type != NOGO_PRO_MOVE) {
		write_ok(player);
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
		LOG_ERROR("getaddrinfo error: %s\n", gai_strerror(status));
		exit(71);
	}

	int listener = -1;
	for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		int yes = 1;
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) {
			perror("setsockopt");
			exit(71);
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
		exit(71);
	}

	printf("Listening on %s\n", argv[1]);

	Queue *msgq = queue_create(sizeof(Message));
	Queue *closeq = queue_create(sizeof(int));
	Context *ctx = ctx_create();
	if (ctx) {
		ctx->msgq = msgq;
		ctx->closeq = closeq;
		ctx->l = lobby_create(9, 9);
	}

	if (!msgq || !closeq || !ctx || !ctx->l) {
		LOG_ERROR("failed to instantiate structs\n");
		exit(71);
	}

	if (ctx_add_player(ctx, &(Player){ .fd = listener, .name = "HOST" }) < 0) {
		LOG_ERROR("failed to add listener\n");
		exit(70);
	}

	for (;;) {
		int poll_checked = 0;  // Number of current poll events handled.
		int poll_len = poll(ctx->pfds, ctx->pfds_len, -1);
		if (poll_len == -1) {
			perror("poll");
			break;
		}

		for (size_t i = 0; i < ctx->pfds_len && poll_checked < poll_len; i++) {
			if (ctx->pfds[i].revents & POLLIN) {
				poll_checked++;

				if (ctx->pfds[i].fd == listener) {
					struct sockaddr_storage remoteaddr;
					socklen_t addrlen = sizeof remoteaddr;

					int newfd = accept(listener, (struct sockaddr*)&remoteaddr, &addrlen);
					
					if (newfd == -1) {
						perror("accept");
					} else {
						Player new_player;
						memset(&new_player, 0, sizeof new_player);

	  					new_player.msgq = ctx->msgq;
						new_player.fd = newfd;
						new_player.write = player_write;
						new_player.read = player_read;

						ctx_add_player(ctx, &new_player);

						write_ok(&new_player);

						char remote_ip[INET6_ADDRSTRLEN];
						LOG_DEBUG("new connection: %s %d on socket: %d\n",
							inet_ntop(remoteaddr.ss_family, get_in_addr(&remoteaddr),
							remote_ip,
							INET6_ADDRSTRLEN),
							newfd,
							newfd);
					}
				} else {
					int sender_fd = ctx->pfds[i].fd;
					Player *player = ctx_get_player(ctx, sender_fd);
					if (!player) {
						LOG_ERROR("unable to get player: %d\n", sender_fd);
						continue;
					}

					char buf[MSG_MAX_SIZE + 1];
					long buf_len = player->read(player, buf, MSG_MAX_SIZE);
					
					if (buf_len <= 0) {
						if (buf_len == 0) {
							printf("socket closed: %d\n", sender_fd);
						} else {
							perror("recv");
						}

						lobby_leave(ctx->l, ctx_get_player(ctx, sender_fd));
						ctx_remove_player(ctx, sender_fd);
						close(sender_fd);
					} else {
						buf[buf_len] = '\0';
						NogoProtocol pro = nogo_parse(buf, (size_t)buf_len);
						LOG_DEBUG("[%d] parse: %d '%s' '%s'\n", sender_fd, pro.type, pro.arg1, pro.arg2);
						serve(ctx, &pro, player);
					}
				}
			}
		}

		// Send all messages in queue.
		while (!queue_isempty(ctx->msgq)) {
			Message msg = *(Message*)queue_get(ctx->msgq);

			for (int i = 0; i < msg.to_len; i++) {
				if (send(msg.to[i], &msg.data, (size_t)msg.data_len, 0) == -1) {
					perror("send");
				}
			}
		}

		// Close all file descriptors in queue.
		while (!queue_isempty(ctx->closeq)) {
			int fd = *(int*)queue_get(ctx->closeq);

			LOG_DEBUG("closing [%d]\n", fd);
			close(fd);
		}
	}

	printf("Connection closed\n");

	queue_free(msgq);
	queue_free(closeq);
	lobby_free(ctx->l);
	ctx_destory(ctx);

	return 0;
}
