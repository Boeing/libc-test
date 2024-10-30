/*
 * mq_unlink unit test
 *
 * Note: EINTR will not be tested, as it requires waiting for mq_unlink to
 *       be blocked before sending a signal.
 *
 * Note: EACCES will not be tested, as it requires elevated privileges.
 */
#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define MQ_NAME "/test_msgq"
#define PERM 0644

int test_mq_opened(void)
{
	mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, PERM, 0);
	if (mqdes < 0) {
		t_error("mq_open failed: %d, %s\n", mqdes, strerror(errno));
	}

	int status = mq_unlink(MQ_NAME);
	TEST(status == 0,
	     "Expected mq_unlink to succeed, instead got status %d, error: %s\n",
	     status, strerror(errno));

	// Unlinked: MQ should not exist on the filesystem any more
	status = access("/dev/mqueue" MQ_NAME, F_OK);
	TEST(status == -1 && errno == ENOENT,
	     "Expected access to mqueue to return -1 and ENOENT, instead got %d, "
	     "error: %s\n",
	     status, strerror(errno));

	// MQ has not been closed, so should still be able to interact with it
	const struct sigevent sev = {.sigev_signo = SIGEV_NONE};
	status = mq_notify(mqdes, &sev);
	TEST(status == 0,
	     "Expected registration to unlinked mqueue to succeed, instead got %d, "
	     "error: %s\n",
	     status, strerror(errno));

	status = mq_close(mqdes);
	if (status == -1) {
		t_error("mq_close failed: %d, error: %s\n", status, strerror(errno));
	}

	// MQ now closed and unlinked, notification should fail with EBADF
	status = mq_notify(mqdes, 0);
	TEST(status == -1 && errno == EBADF,
	     "Expected registration to fail with EBADF after mqueue close, instead "
	     "got %d, error: %s",
	     status, strerror(errno));

	return 0;
}

int main(void)
{
	int status = mq_unlink("Nonexistent MQ");
	TEST(status == -1, "Expected status to be -1, instead got %d\n", status);
	TEST(errno == ENOENT, "Expected errno to be ENOENT, instead got %s\n",
	     strerror(errno));

	errno = 0;
	test_mq_opened();

	return t_status;
}
