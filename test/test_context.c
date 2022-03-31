#include <poll.h>
#include <string.h>

#include "context.h"
#include "player.h"
#include "task.h"

static void ctx_add_player_e(Context *ctx, const Player *p) {
	int err = ctx_add_player(ctx, p);
	ASSERT(err >= 0);
}

static void test_ctx_add_player(void) {
	Context *ctx = ctx_create();

	ctx_add_player_e(ctx, &(Player){ .fd = 1, .name = "Player1" });
	ctx_add_player_e(ctx, &(Player){ .fd = 2, .name = "Player2" });
	ctx_add_player_e(ctx, &(Player){ .fd = 3, .name = "Player3" });
	ctx_add_player_e(ctx, &(Player){ .fd = 4, .name = "Player4" });

	ASSERT(ctx->players_len == 4);
	for (size_t i = 0; i < ctx->players_len; i++) {
		ASSERT(ctx->players[i].fd == (int)i + 1);
	}

	ASSERT(ctx->pfds_len == 4);
	for (size_t i = 0; i < ctx->pfds_len; i++) {
		ASSERT(ctx->pfds[i].fd == (int)i + 1);
	}

	ctx_destory(ctx);
}

static void test_ctx_add_player_fail_when_adding_same_player(void) {
	Context *ctx = ctx_create();

	int actual;

	actual = ctx_add_player(ctx, &(Player){ .fd = 1, .name = "Player1" });
	ASSERT(actual >= 0);

	actual = ctx_add_player(ctx, &(Player){ .fd = 1, .name = "Player1" });
	ASSERT(actual < 0);

	ctx_destory(ctx);
}

static void test_ctx_remove_player(void) {
	Context *ctx = ctx_create();

	ctx_add_player_e(ctx, &(Player){ .fd = 1, .name = "Player1" });
	ctx_add_player_e(ctx, &(Player){ .fd = 2, .name = "Player2" });
	ctx_add_player_e(ctx, &(Player){ .fd = 3, .name = "Player3" });

	ctx_remove_player(ctx, 2);
	ASSERT(ctx->players_len == 2);
	ASSERT(ctx->pfds_len == 2);
	ASSERT(ctx_get_player(ctx, 2) == NULL);

	ctx_remove_player(ctx, 3);
	ctx_remove_player(ctx, 1);

	ASSERT(ctx->players_len == 0);
	ASSERT(ctx->pfds_len == 0);
	ASSERT(ctx_get_player(ctx, 1) == NULL);
	ASSERT(ctx_get_player(ctx, 3) == NULL);

	// Should do nothing when the player does not exist.
	ctx_remove_player(ctx, 1);
	ctx_remove_player(ctx, 2);

	ASSERT(ctx->players_len == 0);
	ASSERT(ctx->pfds_len == 0);

	ctx_destory(ctx);
}

static void test_ctx_get_player(void) {
	Context *ctx = ctx_create();

	ctx_add_player_e(ctx, &(Player){ .fd = 1, .name = "Player1" });
	ctx_add_player_e(ctx, &(Player){ .fd = 2, .name = "Player2" });
	ctx_add_player_e(ctx, &(Player){ .fd = 3, .name = "Player3" });

	ASSERT(strcmp(ctx_get_player(ctx, 2)->name, "Player2") == 0);
	ASSERT(strcmp(ctx_get_player(ctx, 3)->name, "Player3") == 0);
	ASSERT(strcmp(ctx_get_player(ctx, 1)->name, "Player1") == 0);

	ctx_destory(ctx);
}

int main(void) {
	test_ctx_add_player();
	test_ctx_add_player_fail_when_adding_same_player();
	test_ctx_remove_player();
	test_ctx_get_player();
}
