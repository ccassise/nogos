#include <string.h>

#include "task.h"

#include "message_queue.h"

static void test_msgq_create(void) {
	MessageQueue *msgq = msgq_create();

	ASSERT(msgq != NULL);

	msgq_destroy(msgq);
}

static void test_msgq_get_set(void) {
	MessageQueue *msgq = msgq_create();

	Message msg1 = { .to = { 0, 1, 2 }, .data = "hello, ", .to_count = 3, .data_count = 8 };
	Message msg2 = { .to = { 3, 4 }, .data = "world", .to_count = 2, .data_count = 6 };


	msgq_put(msgq, &msg1);

	for (size_t i = 0; i < 50; i++) {
		msgq_put(msgq, &msg2);
	}

	Message got = msgq_get(msgq);

	ASSERT(got.to[0] == 0);
	ASSERT(got.to[1] == 1);
	ASSERT(got.to[2] == 2);
	ASSERT(got.to_count == 3);
	ASSERT(strcmp("hello, ", got.data) == 0);
	ASSERT(got.data_count == 8);

	for (size_t i = 0; i < 49; i++) {
		msgq_get(msgq);
	}

	ASSERT(!msgq_empty(msgq));

	got = msgq_get(msgq);

	ASSERT(got.to[0] == 3);
	ASSERT(got.to[1] == 4);
	ASSERT(got.to_count == 2);
	ASSERT(strcmp("world", got.data) == 0);
	ASSERT(got.data_count == 6);

	msgq_destroy(msgq);
}

static void test_msgq_empty(void) {
	MessageQueue *msgq = msgq_create();

	const int num_loops = 512;
	for (int i = 0; i < num_loops; i++) {
		msgq_put(msgq, &(Message){ .to = { i }, .data = "hello", .to_count = 1, .data_count = 6 });
	}

	int expect = 0;
	while (!msgq_empty(msgq)) {
		Message msg = msgq_get(msgq);

		ASSERT(expect == msg.to[0]);
		ASSERT(msg.to_count == 1);
		ASSERT(strcmp("hello", msg.data) == 0);
		ASSERT(msg.data_count == 6);

		expect++;
	}

	ASSERT(expect == num_loops);

	msgq_destroy(msgq);
}

static void test_msgq_one_at_a_time(void) {
	MessageQueue *msgq = msgq_create();

	const int num_loops = 512;
	for (int i = 0; i < num_loops; i++) {
		msgq_put(msgq, &(Message){ .to = { i }, .data = "hello", .to_count = 1, .data_count = 6 });
		Message msg = msgq_get(msgq);

		ASSERT(msg.to[0] == i);
		ASSERT(msg.to_count == 1);
		ASSERT(strcmp("hello", msg.data) == 0);
		ASSERT(msg.data_count == 6);
		ASSERT(msgq_empty(msgq));
	}

	msgq_destroy(msgq);
}

static void test_msgq_put_two_get_one(void) {
	MessageQueue *msgq = msgq_create();

	int set_num = 0;
	int get_num = 0;
	for (int i = 1; i < 2048; i++) {
		if (i % 3 == 0) {
			Message msg = msgq_get(msgq);

			ASSERT(msg.to[0] == get_num);
			get_num++;
		} else {
			msgq_put(msgq, &(Message){ .to = { set_num }, .data = "hello", .to_count = 1, .data_count = 6 });
			set_num++;
		}
	}

	msgq_destroy(msgq);
}

int main(void) {
	test_msgq_create();
	test_msgq_get_set();
	test_msgq_empty();
	test_msgq_one_at_a_time();
	test_msgq_put_two_get_one();
}
