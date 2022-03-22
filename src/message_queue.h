#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <stdbool.h>

#define MSG_MAX_SIZE 512
#define MSG_MAX_RECIPIENTS 16

typedef struct MessageQueue MessageQueue;

typedef struct {
	int to_count; // Number of recipients.
	int data_count; // Number of bytes in data.

	int to[MSG_MAX_RECIPIENTS]; // Array of file descriptors to send data to.
	char data[MSG_MAX_SIZE]; // The data to send.
} Message;

/**
 * @brief Creates and initializes a message queue. Should be freed with
 * accompanying destroy function when done.
 * 
 * @return MessageQueue* Opaque pointer to MessageQueue.
 */
MessageQueue *msgq_create(void);

/**
 * @brief Free memory allocated by create.
 * 
 * @param msgq The MessageQueue to free.
 */
void msgq_destroy(MessageQueue *msgq);

/**
 * @brief Puts an item into the queue.
 * 
 * @param msgq The MessageQueue instance to put the message in.
 * @param msg The item that will be placed in queue.
 * @return int -1 if an error occured (failed to allocate more space for queue).
 * 0 if the item was successfully add to the queue.
 */
int msgq_put(MessageQueue *msgq, Message *msg);

/**
 * @brief Gets and consumes an item from the queue. Consuming an item from an
 * empty queue is undefined.
 * 
 * @param msgq The MessageQueue instance to get the message from.
 * @return Message The item that was consumed from the queue.
 */
Message msgq_get(MessageQueue *msgq);

/**
 * @brief Return wether the queue is empty or not.
 * 
 * @param msgq The MessageQueue instance to check.
 * @return true 
 * @return false 
 */
bool msgq_empty(MessageQueue *msgq);

#endif
