#include <string.h>
#include <sys/socket.h>

#include "message.h"
#include "player.h"
#include "queue.h"


long player_write(const struct Player *p, const void *buf, size_t size) {
	if (!p->msgq) {
		return (long)send(p->fd, buf, size, 0);
	}

	if (size > MSG_MAX_SIZE) {
		return -1;
	}

	Message msg = { .to = { p->fd }, .to_len = 1, .data_len = (long)size };
	memcpy(msg.data, buf, size);
	queue_put(p->msgq, &msg);
	return (long)size;
}

long player_read(const struct Player *p, void *buf, size_t size) {
	return (long)recv(p->fd, buf, size, 0);
}
