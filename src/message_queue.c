#include <stdlib.h>
#include <string.h>

#include "message_queue.h"

#define START_SIZE 5

// #define DEBUG_ENABLE 1

#ifdef DEBUG_ENABLE
	#include <stdio.h>
	#define DEBUG_PRINT() fprintf(stderr, "[%ld] %ld :: %ld\n", msgq->count, msgq->head - msgq->buffer, msgq->tail - msgq->buffer)
#else
	#define DEBUG_PRINT() do {} while (0)
#endif

struct MessageQueue {
	size_t size;
	size_t count;

	Message *head;
	Message *tail;
	Message *end;
	Message *buffer;
};

MessageQueue *msgq_create(void) {
	MessageQueue *result = malloc(sizeof *result);
	if (result) {
		result->buffer = malloc(sizeof *(result->buffer) * START_SIZE);
		if (!result->buffer) {
			free(result);
			return NULL;
		}
		result->count = 0;
		result->size = START_SIZE;
		result->head = result->buffer;
		result->tail = result->buffer;
		result->end = result->buffer + result->size;
	}
	return result;
}

void msgq_destroy(MessageQueue *msgq) {
	free(msgq->buffer);
	free(msgq);
}

/**
 * @brief Checks if current buffer is full and allocates more room if it is.
 * 
 * @param msgq The MessageQueue to check.
 * @return int -1 if the function failed to allocate extra space.
 * @return int 0 if it was successful.
 */
static int resize(MessageQueue *msgq) {
	if (msgq->count == msgq->size) {
		const size_t head_offset = (size_t)(msgq->head - msgq->buffer);
		const size_t tail_offset = (size_t)(msgq->tail - msgq->buffer);
		const size_t old_size = msgq->size;

		msgq->buffer = realloc(msgq->buffer, sizeof *(msgq->buffer) * msgq->size * 2);
		if (!msgq->buffer) {
			return -1;
		}

		msgq->size = msgq->size * 2;
		msgq->end = msgq->buffer + msgq->size;

		if (head_offset == old_size) {
			// Nothing needs to be moved.
			msgq->head = msgq->buffer + head_offset;
			msgq->tail = msgq->buffer + tail_offset;
		} else if (head_offset < old_size - tail_offset) {
			// Move head section to the end of tail section.
			memmove(msgq->buffer + old_size, msgq->buffer, sizeof *msgq->buffer * head_offset);
			msgq->head = msgq->buffer + old_size + head_offset;
			msgq->tail = msgq->buffer + tail_offset;
		} else {
			// Move tail section to end of newly allocated buffer.
			size_t tail_size = old_size - tail_offset;
			msgq->tail = memmove(msgq->end - tail_size, msgq->buffer + tail_offset, sizeof *msgq->buffer * tail_size);
			msgq->head = msgq->buffer + head_offset;
		}

	}

	return 0;
}

int msgq_put(MessageQueue *msgq, Message *msg) {
	if (resize(msgq) == -1) {
		return -1;
	}

	if (msgq->head == msgq->end) {
		msgq->head = msgq->buffer;
	}

	*msgq->head = *msg;
	msgq->head++;
	msgq->count++;

	// DEBUG_PRINT();
	return 0;
}

Message msgq_get(MessageQueue *msgq) {
	Message result = *msgq->tail;
	memset(msgq->tail, 0, sizeof *msgq->tail);

	msgq->tail++;
	msgq->count--;
	
	if (msgq->tail == msgq->end) {
		msgq->tail = msgq->buffer;
	}

	// DEBUG_PRINT();
	return result;
}

bool msgq_empty(MessageQueue *msgq) {
	// DEBUG_PRINT();
	return msgq->count == 0;
}
