#include <string.h>

#include "protocol.h"
#include "task.h"

static FILE *stream_setup(const char *input) {
	// This code weirdly causes issues with -fsanitize=address. Passing strlen +
	// 1 fixes it but then there's a terminating null in the input, which I
	// really do not want. Valgrind reports nothing wrong.
	FILE *stream = fmemopen(NULL, strlen(input), "r+");

	fwrite(input, sizeof *input, strlen(input), stream);
	rewind(stream);

	return stream;
}

static void test_pro_parse_fail(void) {
	const char *inputs[] = {
		"     ",
		"\n\n\n\t\t\t\r\n",
		"\r\n",
		"TEST\r\n",
	};

	const size_t inputs_count = sizeof inputs / sizeof *inputs;

	for (size_t i = 0; i < inputs_count; i++) {
		FILE *stream = stream_setup(inputs[i]);

		Protocol actual = pro_parse(stream);

		ASSERT(actual.type == PRO_ERROR);

		fclose(stream);
	}

}

static void test_pro_parse_join(void) {
	const char *inputs[] = {
		"JOIN\r\n",
		"   \t\t  JOIN\t\t\r\n",
		"JOIN",
		" JOIN\r\n",
	};

	const size_t inputs_count = sizeof inputs / sizeof *inputs;

	for (size_t i = 0; i < inputs_count; i++) {
		FILE *stream = stream_setup(inputs[i]);

		Protocol actual = pro_parse(stream);

		ASSERT(actual.type == PRO_JOIN);

		fclose(stream);
	}
}

static void test_pro_parse_join_fail(void) {
	const char *inputs[] = {
		"JOIN oh no\r\n",
		"h JOIN\r\n",
		"JOIN 1 2\r\n",
	};

	const size_t inputs_count = sizeof inputs / sizeof *inputs;

	for (size_t i = 0; i < inputs_count; i++) {
		FILE *stream = stream_setup(inputs[i]);

		Protocol actual = pro_parse(stream);

		ASSERT(actual.type == PRO_ERROR);

		fclose(stream);
	}
}

static void test_pro_parse_leave(void) {
	const char *inputs[] = {
		"LEAVE\r\n",
		"   \t\t  LEAVE\t\t\r\n",
		"LEAVE",
		" LEAVE\r\n"
	};

	const size_t inputs_count = sizeof inputs / sizeof *inputs;

	for (size_t i = 0; i < inputs_count; i++) {
		FILE *stream = stream_setup(inputs[i]);

		Protocol actual = pro_parse(stream);

		ASSERT(actual.type == PRO_LEAVE);

		fclose(stream);
	}
}

static void test_pro_parse_leave_fail(void) {
	const char *inputs[] = {
		"LEAVE oh no\r\n",
		"h LEAVE\r\n",
		"LEAVE 1 2\r\n",
	};

	const size_t inputs_count = sizeof inputs / sizeof *inputs;

	for (size_t i = 0; i < inputs_count; i++) {
		FILE *stream = stream_setup(inputs[i]);

		Protocol actual = pro_parse(stream);

		ASSERT(actual.type == PRO_ERROR);

		fclose(stream);
	}
}

static void test_pro_parse_move(void) {
	const char *inputs[] = {
		"MOVE 1 23\r\n",
		"MOVE 1 1",
		"MOVE 12345679 123746895",
		"\t\t\t\t\t MOVE \t\t\n\n 123\t456\r\n"
	};

	const char *expect[][2] = {
		{ "1", "23" },
		{ "1", "1" },
		{ "12345679", "123746895" },
		{ "123", "456" },
	};

	const size_t inputs_count = sizeof inputs / sizeof *inputs;

	for (size_t i = 0; i < inputs_count; i++) {
		FILE *stream = stream_setup(inputs[i]);

		Protocol actual = pro_parse(stream);

		ASSERT(actual.type == PRO_MOVE);
		ASSERT(strcmp(expect[i][0], actual.arg1) == 0);
		ASSERT(strcmp(expect[i][1], actual.arg2) == 0);

		fclose(stream);
	}
}

static void test_pro_parse_move_fail(void) {
	const char *inputs[] = {
		"MOVE\r\n",
		"  MOVE  abc  ",
		"MOVE 123 ab\r\n",
		"MOVE a 1\r\n",
	};

	const size_t inputs_count = sizeof inputs / sizeof *inputs;

	for (size_t i = 0; i < inputs_count; i++) {
		FILE *stream = stream_setup(inputs[i]);

		Protocol actual = pro_parse(stream);

		ASSERT(actual.type == PRO_ERROR);

		fclose(stream);
	}
}

int main(void) {
	test_pro_parse_fail();
	test_pro_parse_join();
	test_pro_parse_join_fail();
	test_pro_parse_leave();
	test_pro_parse_leave_fail();
	test_pro_parse_move();
	test_pro_parse_move_fail();
}
