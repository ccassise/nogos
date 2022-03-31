#include <poll.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "log.h"
#include "player.h"

#define PLAYERS_START 2
#define PFDS_START 2

static int resize(Context *ctx) {
	if (ctx->players_len == ctx->players_size) {
		ctx->players = realloc(ctx->players, sizeof *ctx->players * ctx->players_size * 2);
		if (!ctx->players) {
			return -1;
		}
	}

	if (ctx->pfds_len == ctx->pfds_size) {
		ctx->pfds = realloc(ctx->pfds, sizeof *ctx->pfds * ctx->pfds_size * 2);
		if (!ctx->pfds) {
			return -1;
		}
	}

	return 0;
}

Context *ctx_create(void) {
	Context *result = calloc(1, sizeof *result);
	if (result) {
		result->players = calloc(PLAYERS_START, sizeof *result->players);
		result->players_size = PLAYERS_START;

		result->pfds = calloc(PFDS_START, sizeof *result->pfds);
		result->pfds_size = PFDS_START;
	}

	return result;
}

void ctx_destory(Context *ctx) {
	free(ctx->players);
	free(ctx->pfds);
	free(ctx);
}

int ctx_add_player(Context *ctx, const Player *player) {
	if (resize(ctx) < 0) {
		return -1;
	}

	if (ctx_get_player(ctx, player->fd)) {
		LOG_ERROR("player already exists\n");
		return -1;
	}

	ctx->players[ctx->players_len] = *player;
	ctx->players_len++;

	ctx->pfds[ctx->pfds_len].fd = player->fd;
	ctx->pfds[ctx->pfds_len].events = POLLIN;
	ctx->pfds[ctx->pfds_len].revents = 0;
	ctx->pfds_len++;

	return 0;
}

Player *ctx_get_player(Context *ctx, int fd) {
	Player *found = NULL;
	for (size_t i = 0; i < ctx->players_len; i++) {
		if (ctx->players[i].fd == fd) {
			found = &ctx->players[i];
			break;
		}
	}

	return found;
}

void ctx_remove_player(Context *ctx, int fd) {
	// Remove from players.
	for (size_t i = 0; i < ctx->players_len; i++) {
		if (ctx->players[i].fd == fd) {
			ctx->players[i] = ctx->players[ctx->players_len - 1];
			ctx->players_len--;
			break;
		}
	}

	// Remove from pfds.
	for (size_t i = 0; i < ctx->pfds_len; i++) {
		if (ctx->pfds[i].fd == fd) {
			ctx->pfds[i] = ctx->pfds[ctx->pfds_len - 1];
			ctx->pfds_len--;
			break;
		}
	}
}
