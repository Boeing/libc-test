/*
 * mq_receive unit test
 * Note: This is only a simple functionality test, as most of the functionality
 *       has been previously tested in mq_timedreceive.c
 */
#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

#define MSGQ_NAME "/test_msgq"
#define TEST_MESSAGE "testing"
#define MSG_SIZE sizeof(TEST_MESSAGE)
#define PERM 0644

int main(void)
{
	static const struct mq_attr attr = {
	    .mq_flags = 0,
	    .mq_maxmsg = 1,
	    .mq_msgsize = MSG_SIZE,
	    .mq_curmsgs = 0,
	};

	mqd_t mqdes = mq_open(MSGQ_NAME, O_CREAT | O_RDWR, PERM, &attr);
	if (mqdes == -1) {
		t_error("mq_open() failed with errno: %s\n", strerror(errno));
	}

	int status = mq_send(mqdes, TEST_MESSAGE, strlen(TEST_MESSAGE), 1);
	if (status != 0) {
		t_error("mq_open() failed with status %d, errno: %s\n", status,
		        strerror(errno));
	}

	char receive_buffer[MSG_SIZE] = {0};
	unsigned priority = 0;

	ssize_t len =
	    mq_receive(mqdes, receive_buffer, sizeof(receive_buffer), &priority);

	TEST(strcmp(TEST_MESSAGE, receive_buffer) == 0,
	     "Expected sent and received message to be the same, instead got sent "
	     "message: %s, received message: %s\n",
	     TEST_MESSAGE, receive_buffer);

	TEST(len == MSG_SIZE - 1,
	     "Expected received length to equal %d, instead got %d, errno: %s\n",
	     MSG_SIZE - 1, len, strerror(errno));

	TEST(priority == 1,
	     "Expected the received message priority to be 1, instead got %d\n",
	     priority);

	mq_close(mqdes);
	mq_unlink(MSGQ_NAME);

	return t_status;
}
