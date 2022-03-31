#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MSG_MAX_SIZE 512
#define MSG_MAX_RECIPIENTS 16

typedef struct Message {
	int to[MSG_MAX_RECIPIENTS]; // Array of file descriptors to send data to.
	char data[MSG_MAX_SIZE]; // The data to send.

	long to_len; // Number of recipients.
	long data_len; // Number of bytes in data.
} Message;

#endif
