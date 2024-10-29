/*
 * mq_open unit test
 *
 * Note: The following cases have not been tested:
 *	* EINTR should be returned when mq_open() is interupted by a signal. Since
 *	mq_open() does not block it would be impossible to consistently test this
 *	result.
 *	* EINVAL is returned when the given message queue name is not supported.
 *	Unsupported names are unclear.
 *	* ENFILE is returned when the number of open message queues exceed the
 *	system limit. This is dependant of the system.
 *	* ENOSPC is returned when there is insufficient space to create a new
 *	message queue. This is dependant on the system state.
 */

#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>
#include <sys/resource.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define MQ_NAME "/testqueue"
#define PERMISSIONS (mode_t)0200

static void test_max_file_descriptors(void)
{
	struct mq_attr attr = {
	    .mq_flags = 0,
	    .mq_maxmsg = 1,
	    .mq_msgsize = 1,
	    .mq_curmsgs = 0,
	};

	struct rlimit prev_limit = {0};
	int retval = getrlimit(RLIMIT_NOFILE, &prev_limit);
	if (retval != 0) {
		t_error("getrlimit() failed with errno: %s\n", strerror(errno));
	}
	struct rlimit temp_limit = {
	    .rlim_cur = 0,
	    .rlim_max = prev_limit.rlim_max,
	};

	retval = setrlimit(RLIMIT_NOFILE, &temp_limit);
	if (retval != 0) {
		t_error("setrlimit() failed with errno: %s\n", strerror(errno));
	}

	TEST(mq_open(MQ_NAME, O_CREAT | O_WRONLY, PERMISSIONS, &attr) == -1 &&
	         errno == EMFILE,
	     "Failed to return EMFILE when file descriptor limit is reached. "
	     "Returned %s\n",
	     strerror(errno));

	retval = setrlimit(RLIMIT_NOFILE, &prev_limit);
	if (retval != 0) {
		t_error("setrlimit() failed with errno: %s\n", strerror(errno));
	}
}

static void test_mq_open(void)
{
	struct mq_attr attr = {
	    .mq_flags = 0,
	    .mq_maxmsg = 1,
	    .mq_msgsize = 1,
	    .mq_curmsgs = 0,
	};

	// test opening non-existant mq without setting O_CREAT
	TEST(mq_open(MQ_NAME, O_WRONLY) == -1 && errno == ENOENT,
	     "Failed to return ENOENT when attempting to open a non-existant mq "
	     "without O_CREAT set. Errno: %s\n",
	     strerror(errno));

	// test successful opening of a message queue
	mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_WRONLY, PERMISSIONS, &attr);
	TEST(mqdes != -1, "mq_open() failed with errno %s\n", strerror(errno));

	// Attempting to open an existing mq with oflags O_CREAT and O_EXCL should
	// fail and set errno to EEXIST
	errno = 0;
	TEST(mq_open(MQ_NAME, O_CREAT | O_EXCL | O_WRONLY, PERMISSIONS, &attr) ==
	             -1 &&
	         errno == EEXIST,
	     "Failed to return EEXIST when both O_CREAT and O_EXCL are set and the "
	     "named message queue already exists. Returned %s\n",
	     strerror(errno));

	// mq was created with write permissions only. Attempting to open in as both
	// read and write should fail and set errno to EACCES.
	errno = 0;
	TEST(mq_open(MQ_NAME, O_RDWR) == -1 && errno == EACCES,
	     "Failed to return EACCES when the message queue exists and the "
	     "permission specified by oflag are denied. Returned %s\n",
	     strerror(errno));

	mq_close(mqdes);
	mq_unlink(MQ_NAME);

	// test attempting to create mq with invalid attr
	errno = 0;
	attr.mq_maxmsg = 0;

	int retval = mq_open(MQ_NAME, O_CREAT | O_WRONLY, PERMISSIONS, &attr);

	TEST(retval == -1 && errno == EINVAL,
	     "Failed to return EINVAL when mq_maxmsg is set to <=0. Returned %s\n",
	     strerror(errno));

	if (retval != -1) {
		mq_close(mqdes);
		mq_unlink(MQ_NAME);
	}

	attr.mq_maxmsg = 1;
	attr.mq_msgsize = 0;

	retval = mq_open(MQ_NAME, O_CREAT | O_WRONLY, PERMISSIONS, &attr);

	TEST(retval == -1 && errno == EINVAL,
	     "Failed to return EINVAL when mq_msgsize is set to <=0. Returned %s\n",
	     strerror(errno));

	if (retval != -1) {
		mq_close(mqdes);
		mq_unlink(MQ_NAME);
	}
}

int main(void)
{
	test_mq_open();
	test_max_file_descriptors();
	return t_status;
}
