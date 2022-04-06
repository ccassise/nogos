#include <stdlib.h>
#include <string.h>

#include "libnogo/nogo.h"

#include "lobby.h"
#include "task.h"

typedef struct T {
	Lobby *l;
} T;

static void test_setup(T *t) {
	t->l = lobby_create(7, 5);
}

static void test_teardown(T *t) {
	lobby_free(t->l);
}

static void test_lobby_join(void) {
	T t;
	test_setup(&t);

	const Player player = { .fd = 3 };

	ASSERT(t.l->players_len == 0);

	int actual = lobby_join(t.l, &player);
	ASSERT(actual == 0);

	ASSERT(t.l->players_len == 1);
	ASSERT(t.l->players[0].fd == player.fd);

	test_teardown(&t);
}

static void test_lobby_join_full(void) {
	T t;
	test_setup(&t);

	int actual;

	actual = lobby_join(t.l, &(Player){ .fd = 1 });
	ASSERT(actual == 0);

	actual = lobby_join(t.l, &(Player){ .fd = 2 });
	ASSERT(actual == 0);

	actual = lobby_join(t.l, &(Player){ .fd = 3 });
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_join_same_player(void) {
	T t;
	test_setup(&t);

	int actual;

	actual = lobby_join(t.l, &(Player){ .fd = 1 });
	ASSERT(actual == 0);

	actual = lobby_join(t.l, &(Player){ .fd = 1 });
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_leave(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };
	const Player player3 = (Player){ .fd = 3 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);
	lobby_leave(t.l, &player1);

	ASSERT(t.l->players_len == 1);
	ASSERT(t.l->players[0].fd == player2.fd);
	ASSERT(t.l->players[0].team == 'O');

	lobby_join(t.l, &player3);

	ASSERT(t.l->players_len == 2);
	ASSERT(t.l->players[0].fd == player2.fd);
	ASSERT(t.l->players[0].team == 'O');
	ASSERT(t.l->players[1].fd == player3.fd);
	ASSERT(t.l->players[1].team == 'X');

	lobby_leave(t.l, &player3);

	ASSERT(t.l->players_len == 1);
	ASSERT(t.l->players[0].fd == player2.fd);
	ASSERT(t.l->players[0].team == 'O');

	lobby_leave(t.l, &player2);

	ASSERT(t.l->players_len == 0);

	lobby_leave(t.l, &player2);

	test_teardown(&t);
}

static void test_lobby_play_move(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	lobby_play_move(t.l, &player1, "1", "3");
	lobby_play_move(t.l, &player2, "6", "0");
	lobby_play_move(t.l, &player1, "2", "2");

	const char *expect = (
		". . . . .\n"
		". . . O .\n"
		". . O . .\n"
		". . . . .\n"
		". . . . .\n"
		". . . . .\n"
		"X . . . ."
	);
	char *actual = nogo_board_str(t.l->board);

	ASSERT(strcmp(expect, actual) == 0);

	free(actual);
	test_teardown(&t);
}

static void test_lobby_play_move_overflow(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	int actual;

	actual = lobby_play_move(t.l, &player1, "1", "999999999999999999999999");

	ASSERT(actual == -1);

	actual = lobby_play_move(t.l, &player1, "999999999999999999999999", "1");

	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_player_not_in_lobby(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	int actual = lobby_play_move(t.l, &player2, "1", "2");

	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_lobby_not_full(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);

	int actual = lobby_play_move(t.l, &player2, "1", "2");

	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_not_their_turn(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	int actual;

	actual = lobby_play_move(t.l, &player2, "1", "2");
	ASSERT(actual == -1);

	actual = lobby_play_move(t.l, &player1, "1", "3");
	ASSERT(actual == 0);

	actual = lobby_play_move(t.l, &player1, "2", "2");
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_space_not_free(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	int actual;

	actual = lobby_play_move(t.l, &player1, "1", "2");
	ASSERT(actual == 0);

	actual = lobby_play_move(t.l, &player2, "1", "2");
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void lobby_play_move_e(Lobby *l, const Player *player, const char *row_str, const char *col_str) {
	int actual = lobby_play_move(l, player, row_str, col_str);
	ASSERT(actual != -1);
}

static void test_lobby_play_move_fails_after_winner(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	// 	". . . . .\n"
	// 	". . X . .\n"
	// 	". X O X .\n"
	// 	". . X . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	"O O O . ."

	lobby_play_move_e(t.l, &player1, "2", "2");
	lobby_play_move_e(t.l, &player2, "1", "2");
	lobby_play_move_e(t.l, &player1, "6", "0");
	lobby_play_move_e(t.l, &player2, "2", "1");
	lobby_play_move_e(t.l, &player1, "6", "1");
	lobby_play_move_e(t.l, &player2, "2", "3");
	lobby_play_move_e(t.l, &player1, "6", "2");
	lobby_play_move_e(t.l, &player2, "3", "2");

	int actual = lobby_play_move(t.l, &player1, "0", "0");
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_declares_winner_x(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	// 	". . . . .\n"
	// 	". . X . .\n"
	// 	". X O X .\n"
	// 	". . X . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	"O O O . ."

	lobby_play_move_e(t.l, &player1, "2", "2");
	lobby_play_move_e(t.l, &player2, "1", "2");
	lobby_play_move_e(t.l, &player1, "6", "0");
	lobby_play_move_e(t.l, &player2, "2", "1");
	lobby_play_move_e(t.l, &player1, "6", "1");
	lobby_play_move_e(t.l, &player2, "2", "3");
	lobby_play_move_e(t.l, &player1, "6", "2");
	lobby_play_move_e(t.l, &player2, "3", "2");

	ASSERT(lobby_winner(t.l) == 'X');

	test_teardown(&t);
}

static void test_lobby_declares_winner_o(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	// 	"X O . . .\n"
	// 	"O . . . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	". . . . ."

	lobby_play_move_e(t.l, &player1, "0", "1");
	lobby_play_move_e(t.l, &player2, "0", "0");
	lobby_play_move_e(t.l, &player1, "1", "0");

	ASSERT(lobby_winner(t.l) == 'O');

	test_teardown(&t);
}

static void test_lobby_declares_winner_o_big_group(void) {
	T t;
	test_setup(&t);

	const Player player1 = (Player){ .fd = 1 };
	const Player player2 = (Player){ .fd = 2 };

	lobby_join(t.l, &player1);
	lobby_join(t.l, &player2);

	// 	". O O X O\n"
	// 	"O X X X O\n"
	// 	"O X X O .\n"
	// 	". O O . .\n"
	// 	". . . . .\n"
	// 	". X . X .\n"
	// 	". . . . ."

	lobby_play_move_e(t.l, &player1, "0", "4");
	lobby_play_move_e(t.l, &player2, "0", "3");
	lobby_play_move_e(t.l, &player1, "1", "4");
	lobby_play_move_e(t.l, &player2, "1", "3");
	lobby_play_move_e(t.l, &player1, "2", "3");
	lobby_play_move_e(t.l, &player2, "1", "2");
	lobby_play_move_e(t.l, &player1, "3", "2");
	lobby_play_move_e(t.l, &player2, "2", "2");
	lobby_play_move_e(t.l, &player1, "3", "1");
	lobby_play_move_e(t.l, &player2, "2", "1");
	lobby_play_move_e(t.l, &player1, "2", "0");
	lobby_play_move_e(t.l, &player2, "1", "1");
	lobby_play_move_e(t.l, &player1, "1", "0");
	lobby_play_move_e(t.l, &player2, "5", "1");
	lobby_play_move_e(t.l, &player1, "0", "1");
	lobby_play_move_e(t.l, &player2, "5", "3");
	lobby_play_move_e(t.l, &player1, "0", "2");

	ASSERT(lobby_winner(t.l) == 'O');

	test_teardown(&t);
}

int main(void) {
	test_lobby_join();
	test_lobby_join_full();
	test_lobby_join_same_player();
	test_lobby_leave();
	test_lobby_play_move();
	test_lobby_play_move_overflow();
	test_lobby_play_move_player_not_in_lobby();
	test_lobby_play_move_lobby_not_full();
	test_lobby_play_move_not_their_turn();
	test_lobby_play_move_space_not_free();
	test_lobby_play_move_fails_after_winner();
	test_lobby_declares_winner_x();
	test_lobby_declares_winner_o();
	test_lobby_declares_winner_o_big_group();
}
