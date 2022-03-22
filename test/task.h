#ifndef TASK_H_
#define TASK_H_

#include <assert.h>

#define ASSERT(expr) \
do { \
	if (!(expr)) { \
		assert(expr); \
	} \
} while (0)

#endif
