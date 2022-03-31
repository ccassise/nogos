#include <stdlib.h>
#include <string.h>

#include "queue.h"

#define START_SIZE 2

struct Queue {
	size_t elem_size;
	size_t size; // Number of element sized elements buffer can hold.
	size_t len;

	unsigned char *buffer;
	unsigned char *head;
	unsigned char *tail;
	unsigned char *end;
};

/**
 * @brief Checks if current buffer is full and allocates more room if it is.
 * 
 * @param q The Queue to resize.
 * @return int -1 if the function failed to allocate extra space. 0 if it was successful.
 */
static int resize(Queue *q) {
	if (q->len == q->size) {
		const size_t head_offset = (size_t)(q->head - q->buffer);
		const size_t tail_offset = (size_t)(q->tail - q->buffer);
		const size_t old_size = q->size * q->elem_size;

		q->buffer = realloc(q->buffer, q->elem_size * q->size * 2);
		if (!q->buffer) {
			return -1;
		}

		q->size = q->size * 2;
		q->end = q->buffer + (q->size * q->elem_size);

		if (head_offset == old_size) {
			// Nothing needs to be moved.
			q->head = q->buffer + head_offset;
			q->tail = q->buffer + tail_offset;
		} else if (head_offset < old_size - tail_offset) {
			// Move head section to the end of tail section.
			memmove(q->buffer + old_size, q->buffer, head_offset);
			q->head = q->buffer + old_size + head_offset;
			q->tail = q->buffer + tail_offset;
		} else {
			// Move tail section to end of newly allocated buffer.
			size_t tail_size = old_size - tail_offset;
			q->tail = memmove(q->buffer + old_size + tail_offset, q->buffer + tail_offset, tail_size);
			q->head = q->buffer + head_offset;
		}
	}

	return 0;
}

Queue *queue_create(size_t elem_size) {
	Queue *result = malloc(sizeof *result);
	if (result) {
		result->buffer = malloc(elem_size * START_SIZE);
		if (!result->buffer) {
			free(result);
			return NULL;
		}
		result->len = 0;
		result->size = START_SIZE;
		result->elem_size = elem_size;
		result->head = result->buffer;
		result->tail = result->buffer;
		result->end = result->buffer + (result->size * elem_size);
	}
	return result;
}

void queue_free(Queue *q) {
	free(q->buffer);
	free(q);
}

int queue_put(Queue *q, const void *elem) {
	if (resize(q) == -1) {
		return -1;
	}

	if (q->head == q->end) {
		q->head = q->buffer;
	}

	memcpy(q->head, elem, q->elem_size);
	q->head += q->elem_size;
	q->len++;

	return 0;
}

void *queue_get(Queue *q) {
	void *result = q->tail;
	if (q->len == 0) {
		memset(q->tail, 0, q->elem_size);
		return result;
	}

	q->tail += q->elem_size;
	q->len--;
	
	if (q->tail == q->end) {
		q->tail = q->buffer;
	}

	return result;
}

bool queue_isempty(Queue *q) {
	return q->len == 0;
}
