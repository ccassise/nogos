#include <stdlib.h>
#include <string.h>

#include "lobby.h"
#include "task.h"

typedef struct {
	Lobby *l;
	MessageQueue *msgq;
} T;

static void test_setup(T *t) {
	t->msgq = msgq_create();
	t->l = lobby_create(t->msgq, 7, 5);
}

static void test_teardown(T *t) {
	msgq_destroy(t->msgq);
	lobby_destroy(t->l);
}

static void test_lobby_join(void) {
	T t;
	test_setup(&t);

	const int playerfd = 3;

	ASSERT(t.l->players_count == 0);

	int actual = lobby_join(t.l, playerfd);
	ASSERT(actual == 0);

	ASSERT(t.l->players_count == 1);
	ASSERT(t.l->players[0].fd == playerfd);

	test_teardown(&t);
}

static void test_lobby_join_full(void) {
	T t;
	test_setup(&t);

	int actual;

	actual = lobby_join(t.l, 1);
	ASSERT(actual == 0);

	actual = lobby_join(t.l, 2);
	ASSERT(actual == 0);

	actual = lobby_join(t.l, 3);
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_join_same_player(void) {
	T t;
	test_setup(&t);

	int actual;

	actual = lobby_join(t.l, 1);
	ASSERT(actual == 0);

	actual = lobby_join(t.l, 1);
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_leave(void) {
	T t;
	test_setup(&t);

	const int player1fd = 3;
	const int player2fd = 4;
	const int player3fd = 5;

	lobby_join(t.l, player1fd);
	lobby_join(t.l, player2fd);
	lobby_leave(t.l, player1fd);

	ASSERT(t.l->players_count == 1);
	ASSERT(t.l->players[0].fd == player2fd);
	ASSERT(t.l->players[0].team == 'O');

	lobby_join(t.l, player3fd);

	ASSERT(t.l->players_count == 2);
	ASSERT(t.l->players[0].fd == player2fd);
	ASSERT(t.l->players[0].team == 'O');
	ASSERT(t.l->players[1].fd == player3fd);
	ASSERT(t.l->players[1].team == 'X');

	lobby_leave(t.l, player3fd);

	ASSERT(t.l->players_count == 1);
	ASSERT(t.l->players[0].fd == player2fd);
	ASSERT(t.l->players[0].team == 'O');

	lobby_leave(t.l, player2fd);

	ASSERT(t.l->players_count == 0);

	lobby_leave(t.l, player2fd);

	test_teardown(&t);
}

static void test_lobby_leave_message(void) {
	T t;
	test_setup(&t);

	const int player1fd = 3;
	const int player2fd = 4;

	lobby_join(t.l, player1fd);
	lobby_join(t.l, player2fd);
	lobby_leave(t.l, player1fd);

	Message last_msg;
	while (!msgq_empty(t.msgq)) {
		last_msg = msgq_get(t.msgq);
	}

	ASSERT(strncmp(last_msg.data, "OPPONENT_LEFT\r\n", (size_t)last_msg.data_count) == 0);
	ASSERT(last_msg.data_count == 15);
	ASSERT(last_msg.to_count == 1);
	ASSERT(last_msg.to[0] == 4);

	test_teardown(&t);
}

static void test_lobby_play_move(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	lobby_play_move(t.l, 1, "1", "3");
	lobby_play_move(t.l, 2, "6", "0");
	lobby_play_move(t.l, 1, "2", "2");

	const char *expect = (
		". . . . .\n"
		". . . O .\n"
		". . O . .\n"
		". . . . .\n"
		". . . . .\n"
		". . . . .\n"
		"X . . . ."
	);
	char *actual = board_str(t.l->board);

	ASSERT(strcmp(expect, actual) == 0);

	Message msg;

	msg = msgq_get(t.msgq);
	ASSERT(strncmp(msg.data, "MOVE 1 3\r\n", (size_t)msg.data_count) == 0);
	ASSERT(msg.to_count == 1);
	ASSERT(msg.to[0] == 2);

	msg = msgq_get(t.msgq);
	ASSERT(strncmp(msg.data, "MOVE 6 0\r\n", (size_t)msg.data_count) == 0);
	ASSERT(msg.to_count == 1);
	ASSERT(msg.to[0] == 1);

	msg = msgq_get(t.msgq);
	ASSERT(strncmp(msg.data, "MOVE 2 2\r\n", (size_t)msg.data_count) == 0);
	ASSERT(msg.to_count == 1);
	ASSERT(msg.to[0] == 2);

	free(actual);
	test_teardown(&t);
}

static void test_lobby_play_move_overflow(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	int actual;

	actual = lobby_play_move(t.l, 1, "1", "999999999999999999999999");

	ASSERT(actual == -1);

	actual = lobby_play_move(t.l, 1, "999999999999999999999999", "1");

	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_player_not_in_lobby(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	int actual = lobby_play_move(t.l, 2, "1", "2");

	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_lobby_not_full(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);

	int actual = lobby_play_move(t.l, 2, "1", "2");

	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_not_their_turn(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	int actual;

	actual = lobby_play_move(t.l, 2, "1", "2");
	ASSERT(actual == -1);

	actual = lobby_play_move(t.l, 1, "1", "3");
	ASSERT(actual == 0);

	actual = lobby_play_move(t.l, 1, "2", "2");
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_play_move_space_not_free(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	int actual;

	actual = lobby_play_move(t.l, 1, "1", "2");
	ASSERT(actual == 0);

	actual = lobby_play_move(t.l, 2, "1", "2");
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void lobby_play_move_e(Lobby *l, char team, const char *row_str, const char *col_str) {
	int actual = lobby_play_move(l, team, row_str, col_str);
	ASSERT(actual != -1);
}

static void test_lobby_play_move_fails_after_winner(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	// 	". . . . .\n"
	// 	". . X . .\n"
	// 	". X O X .\n"
	// 	". . X . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	"O O O . ."

	lobby_play_move_e(t.l, 1, "2", "2");
	lobby_play_move_e(t.l, 2, "1", "2");
	lobby_play_move_e(t.l, 1, "6", "0");
	lobby_play_move_e(t.l, 2, "2", "1");
	lobby_play_move_e(t.l, 1, "6", "1");
	lobby_play_move_e(t.l, 2, "2", "3");
	lobby_play_move_e(t.l, 1, "6", "2");
	lobby_play_move_e(t.l, 2, "3", "2");

	int actual = lobby_play_move(t.l, 1, "0", "0");
	ASSERT(actual == -1);

	test_teardown(&t);
}

static void test_lobby_declares_winner_x(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	// 	". . . . .\n"
	// 	". . X . .\n"
	// 	". X O X .\n"
	// 	". . X . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	"O O O . ."

	lobby_play_move_e(t.l, 1, "2", "2");
	lobby_play_move_e(t.l, 2, "1", "2");
	lobby_play_move_e(t.l, 1, "6", "0");
	lobby_play_move_e(t.l, 2, "2", "1");
	lobby_play_move_e(t.l, 1, "6", "1");
	lobby_play_move_e(t.l, 2, "2", "3");
	lobby_play_move_e(t.l, 1, "6", "2");
	lobby_play_move_e(t.l, 2, "3", "2");

	Message last_msg;
	while (!msgq_empty(t.msgq)) {
		last_msg = msgq_get(t.msgq);
	}

	ASSERT(strncmp(last_msg.data, "WINS X\r\n", (size_t)last_msg.data_count) == 0);
	ASSERT(last_msg.data_count == 8);
	ASSERT(last_msg.to_count == 2);
	ASSERT(last_msg.to[0] == 1);
	ASSERT(last_msg.to[1] == 2);

	test_teardown(&t);
}

static void test_lobby_declares_winner_o(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	// 	"X O . . .\n"
	// 	"O . . . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	". . . . .\n"
	// 	". . . . ."

	lobby_play_move_e(t.l, 1, "0", "1");
	lobby_play_move_e(t.l, 2, "0", "0");
	lobby_play_move_e(t.l, 1, "1", "0");

	Message last_msg;
	while (!msgq_empty(t.msgq)) {
		last_msg = msgq_get(t.msgq);
	}

	ASSERT(strncmp(last_msg.data, "WINS O\r\n", (size_t)last_msg.data_count) == 0);
	ASSERT(last_msg.data_count == 8);
	ASSERT(last_msg.to_count == 2);
	ASSERT(last_msg.to[0] == 1);
	ASSERT(last_msg.to[1] == 2);

	test_teardown(&t);
}

static void test_lobby_declares_winner_o_big_group(void) {
	T t;
	test_setup(&t);

	lobby_join(t.l, 1);
	lobby_join(t.l, 2);

	// 	". O O X O\n"
	// 	"O X X X O\n"
	// 	"O X X O .\n"
	// 	". O O . .\n"
	// 	". . . . .\n"
	// 	". X . X .\n"
	// 	". . . . ."

	lobby_play_move_e(t.l, 1, "0", "4");
	lobby_play_move_e(t.l, 2, "0", "3");
	lobby_play_move_e(t.l, 1, "1", "4");
	lobby_play_move_e(t.l, 2, "1", "3");
	lobby_play_move_e(t.l, 1, "2", "3");
	lobby_play_move_e(t.l, 2, "1", "2");
	lobby_play_move_e(t.l, 1, "3", "2");
	lobby_play_move_e(t.l, 2, "2", "2");
	lobby_play_move_e(t.l, 1, "3", "1");
	lobby_play_move_e(t.l, 2, "2", "1");
	lobby_play_move_e(t.l, 1, "2", "0");
	lobby_play_move_e(t.l, 2, "1", "1");
	lobby_play_move_e(t.l, 1, "1", "0");
	lobby_play_move_e(t.l, 2, "5", "1");
	lobby_play_move_e(t.l, 1, "0", "1");
	lobby_play_move_e(t.l, 2, "5", "3");
	lobby_play_move_e(t.l, 1, "0", "2");

	Message last_msg;
	while (!msgq_empty(t.msgq)) {
		last_msg = msgq_get(t.msgq);
	}

	ASSERT(strncmp(last_msg.data, "WINS O\r\n", (size_t)last_msg.data_count) == 0);
	ASSERT(last_msg.data_count == 8);
	ASSERT(last_msg.to_count == 2);
	ASSERT(last_msg.to[0] == 1);
	ASSERT(last_msg.to[1] == 2);

	test_teardown(&t);
}

int main(void) {
	test_lobby_join();
	test_lobby_join_full();
	test_lobby_join_same_player();
	test_lobby_leave();
	test_lobby_leave_message();
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
