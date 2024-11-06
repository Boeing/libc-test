/*
 * mq_setattr unit test
 */

#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define MQ_NAME "/testqueue"
#define PERMISSIONS (mode_t)0644
#define IGNORED_VALUE 123

static const struct mq_attr attr = {
    .mq_flags = O_NONBLOCK,
    .mq_maxmsg = IGNORED_VALUE,
    .mq_msgsize = IGNORED_VALUE,
    .mq_curmsgs = IGNORED_VALUE,
};

static void test_invalid_mq_descriptor(void)
{
	TEST(mq_setattr((mqd_t)-1, &attr, NULL) == -1 && errno == EBADF,
	     "Invalid mq descriptor test failed. Expected %s, got %s\n",
	     strerror(EBADF), strerror(errno));
}

static void test_omqstat(void)
{
	// open mq with default attributes
	mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, PERMISSIONS, NULL);
	if (mqdes == (mqd_t)-1) {
		t_error("mq_open() failed with errno: %s\n", strerror(errno));
	}

	struct mq_attr old_attr = {0};

	if (mq_getattr(mqdes, &old_attr) == -1) {
		t_error("mq_getattr() failed with errno %s\n", strerror(errno));
	}

	struct mq_attr omqstat = {0};

	TEST(mq_setattr(mqdes, &attr, &omqstat) != -1,
	     "mq_setattr() failed with errno %s\n", strerror(errno));

	TEST(omqstat.mq_flags == old_attr.mq_flags,
	     "omqstat.mq_flags did not match the original value. Expected %ld, got "
	     "%ld\n",
	     old_attr.mq_flags, omqstat.mq_flags);
	TEST(
	    omqstat.mq_maxmsg == old_attr.mq_maxmsg,
	    "omqstat.mq_maxmsg did not match the original value. Expected %ld, got "
	    "%ld\n",
	    old_attr.mq_maxmsg, omqstat.mq_maxmsg);
	TEST(omqstat.mq_msgsize == old_attr.mq_msgsize,
	     "omqstat.mq_msgsize did not match the original value. Expected %ld, "
	     "got "
	     "%ld\n",
	     old_attr.mq_msgsize, omqstat.mq_msgsize);
	TEST(omqstat.mq_curmsgs == old_attr.mq_curmsgs,
	     "omqstat.mq_curmsgs did not match the original value. Expected %ld, "
	     "got "
	     "%ld\n",
	     old_attr.mq_curmsgs, omqstat.mq_curmsgs);

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
}

static void test_mq_setattr(void)
{
	// open mq with default attributes
	mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, PERMISSIONS, NULL);
	if (mqdes == (mqd_t)-1) {
		t_error("mq_open() failed with errno: %s\n", strerror(errno));
	}

	// set new attributes and store previous attributes
	struct mq_attr prev_attr = {0};
	TEST(mq_setattr(mqdes, &attr, &prev_attr) != -1,
	     "mq_setattr() failed with errno %s\n", strerror(errno));

	// get new attributes
	struct mq_attr new_attr = {0};
	if (mq_getattr(mqdes, &new_attr) == -1) {
		t_error("mq_getattr() failed with errno %s\n", strerror(errno));
	}

	// check that mq_flags was updated
	TEST(new_attr.mq_flags == attr.mq_flags,
	     "mq_flags was not updated correctly. Expected %ld, got %ld\n",
	     attr.mq_flags, new_attr.mq_flags);

	// check that the following members were not modified
	TEST(new_attr.mq_maxmsg == prev_attr.mq_maxmsg,
	     "The mq_maxmsg was modified erroneously. Expected value of %ld, got "
	     "%ld\n",
	     prev_attr.mq_maxmsg, new_attr.mq_maxmsg);
	TEST(new_attr.mq_msgsize == prev_attr.mq_msgsize,
	     "The mq_msgsize was modified erroneously. Expected value of %ld, got "
	     "%ld\n",
	     prev_attr.mq_msgsize, new_attr.mq_msgsize);

	TEST(new_attr.mq_curmsgs == prev_attr.mq_curmsgs,
	     "The mq_curmsgs was modified erroneously. Expected value of %ld, got "
	     "%ld\n",
	     prev_attr.mq_curmsgs, new_attr.mq_curmsgs);

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
}

int main(void)
{
	test_invalid_mq_descriptor();
	test_omqstat();
	test_mq_setattr();
	return t_status;
}
