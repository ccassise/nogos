#include <string.h>

#include "queue.h"
#include "task.h"

static void test_queue_create(void) {
	Queue *queue = queue_create(sizeof(char));

	ASSERT(queue != NULL);

	queue_free(queue);
}

static void test_queue_get_set(void) {
	Queue *queue = queue_create(sizeof(int));

	int num1 = 42;
	int num2 = 9;

	queue_put(queue, &num1);

	for (size_t i = 0; i < 50; i++) {
		queue_put(queue, &num2);
	}

	int got = *(int*)queue_get(queue);

	ASSERT(got == num1);

	for (size_t i = 0; i < 49; i++) {
		got = *(int*)queue_get(queue);
	}

	ASSERT(!queue_isempty(queue));

	got = *(int*)queue_get(queue);

	ASSERT(got == num2);

	queue_free(queue);
}

static void test_queue_empty(void) {
	Queue *queue = queue_create(sizeof(int));

	const int num_loops = 512;
	for (int i = 0; i < num_loops; i++) {
		queue_put(queue, &i);
	}

	int expect = 0;
	while (!queue_isempty(queue)) {
		int actual = *(int*)queue_get(queue);

		ASSERT(expect == actual);

		expect++;
	}

	ASSERT(expect == num_loops);

	queue_free(queue);
}

static void test_queue_one_at_a_time(void) {
	Queue *queue = queue_create(sizeof(size_t));

	const size_t num_loops = 512;
	for (size_t i = 0; i < num_loops; i++) {
		queue_put(queue, &i);
		size_t actual = *(size_t*)queue_get(queue);

		ASSERT(i == actual);
	}

	queue_free(queue);
}

static void test_queue_put_two_get_one(void) {
	Queue *queue = queue_create(sizeof(int));

	int set_num = 0;
	int get_num = 0;
	for (int i = 1; i < 5000; i++) {
		if (i % 3 == 0) {
			int actual = *(int*)queue_get(queue);

			ASSERT(get_num == actual);

			get_num++;
		} else {
			queue_put(queue, &set_num);
			set_num++;
		}
	}

	queue_free(queue);
}

static void test_queue_any_sized_type(void) {
	const char data[][512] = {
		"hello",
		"world",
		"how are you?\n",
		"I'm doing well thanks for asking :)\n"
	};

	Queue *queue = queue_create(sizeof(data[0]));
	
	for (size_t i = 0; i < sizeof data / sizeof data[0]; i++) {
		queue_put(queue, data[i]);
	}

	const char *actual;

	for (size_t i = 0; i < sizeof data / sizeof data[0]; i++) {
		actual = (const char*)queue_get(queue);
		ASSERT(strcmp(data[i], actual) == 0);
	}

	queue_free(queue);
}

int main(void) {
	test_queue_create();
	test_queue_get_set();
	test_queue_empty();
	test_queue_one_at_a_time();
	test_queue_put_two_get_one();
	test_queue_any_sized_type();
}
