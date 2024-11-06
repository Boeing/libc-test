/*
 * mq_getattr unit test
 */

#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define MQ_NAME "/testqueue"
#define PERMISSIONS (mode_t)0644
#define MAX_MSG 10
#define MSG_SIZE 256

static void test_invalid_mq_descriptor(void)
{
	struct mq_attr attr = {0};
	TEST(mq_getattr((mqd_t)-1, &attr) == -1 && errno == EBADF,
	     "Invalid mq descriptor test failed. Expected %s, got %s\n",
	     strerror(EBADF), strerror(errno));
}

static void test_mq_getattr(void)
{
	const struct mq_attr attr = {
	    .mq_flags = 0,
	    .mq_maxmsg = MAX_MSG,
	    .mq_msgsize = MSG_SIZE,
	    .mq_curmsgs = 0,
	};

	mqd_t mqdes =
	    mq_open(MQ_NAME, O_CREAT | O_RDWR | O_NONBLOCK, PERMISSIONS, &attr);
	if (mqdes == (mqd_t)-1) {
		t_error("mq_open() failed with errno: %s\n", strerror(errno));
	}

	struct mq_attr result_attr = {0};

	TEST(mq_getattr(mqdes, &result_attr) == 0,
	     "mq_getattr() failed with errno %s\n", strerror(errno));

	TEST(result_attr.mq_flags == O_NONBLOCK,
	     "result_attr.mq_flags did not match the original value. Expected %ld, "
	     "got "
	     "%ld\n",
	     O_NONBLOCK, result_attr.mq_flags);
	TEST(result_attr.mq_maxmsg == MAX_MSG,
	     "result_attr.mq_maxmsg did not match the original value. Expected "
	     "%ld, got "
	     "%ld\n",
	     MAX_MSG, result_attr.mq_maxmsg);
	TEST(result_attr.mq_msgsize == MSG_SIZE,
	     "result_attr.mq_msgsize did not match the original value. Expected "
	     "%ld, "
	     "got "
	     "%ld\n",
	     MSG_SIZE, result_attr.mq_msgsize);
	TEST(result_attr.mq_curmsgs == 0,
	     "result_attr.mq_curmsgs did not match the expected value. Expected 0, "
	     "got "
	     "%ld\n",
	     result_attr.mq_curmsgs);

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
}

int main(void)
{
	test_invalid_mq_descriptor();
	test_mq_getattr();
	return t_status;
}
