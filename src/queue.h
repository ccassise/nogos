#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdbool.h>

typedef struct Queue Queue;

/**
 * @brief Creates and initializes a queue. Should be freed with accompanying
 * free function when done.
 * 
 * @param elem_size The size of the elements that will be placed in the queue.
 * @return Queue* Opaque pointer to a newly created Queue. NULL if error occured.
 */
Queue *queue_create(size_t elem_size);

/**
 * @brief Free memory allocated by create.
 * 
 * @param q The Queue to free.
 */
void queue_free(Queue *q);

/**
 * @brief Puts an element into the queue.
 * 
 * @param q The Queue instance to put the message in.
 * @param elem The element that will be placed in queue. Assumed to be the same
 * size as the element size the queue was instantiated with.
 * @return int -1 if an error occured (failed to allocate more space for queue).
 * 0 if the element was successfully added to the queue.
 */
int queue_put(Queue *q, const void *elem);

/**
 * @brief Gets and consumes an element from the queue. Consuming an element from
 * an empty buffer will return a zerod-out element. The pointed to data should
 * be copied or consumed in some way before more operations on the data
 * structure are done. Otherwise the data may be invalidated.
 * 
 * @param q The Queue instance to get the message from.
 * @return void* The element that was consumed from the queue.
 */
void *queue_get(Queue *q);

/**
 * @brief Return wether the queue is empty or not.
 * 
 * @param q The Queue instance to check.
 * @return true 
 * @return false 
 */
bool queue_isempty(Queue *q);

#endif
