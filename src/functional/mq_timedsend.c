/*
 * mq_timedsend unit test
 *
 * Note: Competing threads of different priorities have not been tested due to
 *       sudo privileges being required to change a threads priority. The POSIX
 *       standard states: If more than one thread is waiting to send when space
 *       becomes available in the message queue and the Priority Scheduling
 *       option is supported, then the thread of the highest priority that has
 *       been waiting the longest shall be unblocked to send its message.
 */

#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define SLEEP_NANO 500000
#define MAX_NSEC 1000000000
#define MSG_SIZE 8
#define MQ_NAME "/testqueue"
#define MSG "testing"
#define PERMISSIONS (mode_t)0644
#define PRIO_TEST_MAXMSG 2
#define PRIO_TEST_MSGSIZE 10

static void dummy_signal_handler(int signum) {}

static mqd_t open_mq_helper(int oflag)
{
	const struct mq_attr attr = {
	    .mq_flags = 0,
	    .mq_maxmsg = 1,
	    .mq_msgsize = MSG_SIZE,
	    .mq_curmsgs = 0,
	};
	mqd_t mqdes = mq_open(MQ_NAME, oflag, PERMISSIONS, &attr);

	if (mqdes == (mqd_t)-1) {
		t_error("mq_open() failed with errno: %s\n", strerror(errno));
	}

	return mqdes;
}

static struct timespec abstime_helper(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
		t_error("clock_gettime() failed with errno %s\n", strerror(errno));
	}

	if (ts.tv_nsec + SLEEP_NANO >= MAX_NSEC) {
		ts.tv_sec += 1;
		ts.tv_nsec = SLEEP_NANO - (MAX_NSEC - ts.tv_nsec);
	} else {
		ts.tv_nsec += SLEEP_NANO;
	}

	return ts;
}

static void test_send_message(void)
{
	mqd_t mqdes = open_mq_helper(O_CREAT | O_RDWR);

	struct timespec ts = abstime_helper();
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &ts) == 0,
	     "mq_timedsend() failed with errno: %s\n", strerror(errno));

	char received_msg[MSG_SIZE] = {0};

	ssize_t retval = mq_receive(mqdes, received_msg, sizeof(received_msg), 0);
	if (retval == -1) {
		t_error("mq_receive() failed with errno: %s\n", strerror(errno));
	}

	TEST(strcmp(received_msg, MSG) == 0,
	     "The message received did not match the message sent. Expected %s, "
	     "got %s\n",
	     MSG, received_msg);

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	return;
}

static void test_full_queue(void)
{
	mqd_t mqdes = open_mq_helper(O_CREAT | O_RDWR | O_NONBLOCK);

	struct timespec ts = abstime_helper();
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &ts) == 0,
	     "mq_timedsend() failed with errno: %s\n", strerror(errno));

	// mq can only hold 1 message
	errno = 0;
	ts = abstime_helper();
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &ts) == -1 && errno == EAGAIN,
	     "Full message queue test failed. Expected %s, got %s\n",
	     strerror(EAGAIN), strerror(errno));

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	return;
}

static void test_invalid_mqdes(void)
{
	errno = 0;
	struct timespec ts = abstime_helper();
	TEST(mq_timedsend((mqd_t)-1, MSG, strlen(MSG), 0, &ts) == -1 &&
	         errno == EBADF,
	     "Invalid mqdes test failed. Expected %s, got %s\n", strerror(EBADF),
	     strerror(errno));

	return;
}

static void test_signal_interupt()
{
	mqd_t mqdes = open_mq_helper(O_CREAT | O_RDWR);

	struct timespec ts = abstime_helper();
	// fill up mq in order to force next mq_timedsend to block
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &ts) == 0,
	     "mq_timedsend() failed with errno: %s\n", strerror(errno));

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = dummy_signal_handler;
	sigaction(SIGALRM, &sa, NULL);

	timer_t timerid = 0;
	if (timer_create(CLOCK_MONOTONIC, 0, &timerid) == -1) {
		t_error("timer_create() failed with errno: %s\n", strerror(errno));
	}

	struct itimerspec its = {
	    .it_value.tv_sec = 0,
	    .it_value.tv_nsec = SLEEP_NANO,
	};
	if (timer_settime(timerid, 0, &its, 0) == -1) {
		t_error("timer_settime() failed with errno: %s\n", strerror(errno));
		timer_delete(timerid);
	}

	// set timeout to 2 seconds in the future to avoid edge cases where the next
	// second is sooner than the timer timeout
	struct timespec long_ts = {time(NULL) + 2, 0};
	errno = 0;
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &long_ts) == -1 &&
	         errno == EINTR,
	     "Signal interupt test failed. Expected %s, got %s\n", strerror(EINTR),
	     strerror(errno));

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	timer_delete(timerid);
	return;
}

static void test_invalid_msg_prio(void)
{
	mqd_t mqdes = open_mq_helper(O_CREAT | O_RDWR);

	errno = 0;
	struct timespec ts = abstime_helper();
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), MQ_PRIO_MAX, &ts) == -1 &&
	         errno == EINVAL,
	     "Msg_prio out of range test failed. Expected %s, got %s\n",
	     strerror(EINVAL), strerror(errno));

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	return;
}

static void test_invalid_block_time(void)
{
	mqd_t mqdes = open_mq_helper(O_CREAT | O_RDWR);

	struct timespec ts = abstime_helper();
	// fill up mq in order to force next mq_timedsend to block
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &ts) == 0,
	     "mq_timedsend() failed with errno: %s\n", strerror(errno));

	struct timespec invalid_ts = {0, -1};
	errno = 0;
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &invalid_ts) == -1 &&
	         errno == EINVAL,
	     "Invalid abstime parameter test failed (ts.tv_nsec < 0). Expected %s, "
	     "got %s\n",
	     strerror(EINVAL), strerror(errno));

	errno = 0;
	invalid_ts.tv_nsec = MAX_NSEC;
	TEST(
	    mq_timedsend(mqdes, MSG, strlen(MSG), 0, &invalid_ts) == -1 &&
	        errno == EINVAL,
	    "Invalid abstime parameter test failed (ts.tv_nsec > %d). Expected %s, "
	    "got %s\n",
	    MAX_NSEC, strerror(EINVAL), strerror(errno));

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	return;
}

static void test_oversized_msg(void)
{
	mqd_t mqdes = open_mq_helper(O_CREAT | O_RDWR);

	const char *oversized_msg = "This string is oversized";

	errno = 0;
	struct timespec ts = abstime_helper();
	TEST(mq_timedsend(mqdes, oversized_msg, strlen(oversized_msg), 0, &ts) ==
	             -1 &&
	         errno == EMSGSIZE,
	     "Oversized message test failed. Expected %s, got %s\n",
	     strerror(EMSGSIZE), strerror(errno));

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	return;
}

static void test_send_timeout(void)
{
	mqd_t mqdes = open_mq_helper(O_CREAT | O_RDWR);

	struct timespec ts = abstime_helper();
	// fill up mq in order to force next mq_timedsend to block
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &ts) == 0,
	     "mq_timedsend() failed with errno: %s\n", strerror(errno));

	errno = 0;
	ts = abstime_helper();
	TEST(mq_timedsend(mqdes, MSG, strlen(MSG), 0, &ts) == -1 &&
	         errno == ETIMEDOUT,
	     "mq_timedsend() timeout test failed. Expected %s, got %s\n",
	     strerror(ETIMEDOUT), strerror(errno));

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	return;
}

static void test_priority_ordering(void)
{
	const struct mq_attr attr2 = {
	    .mq_flags = 0,
	    .mq_maxmsg = PRIO_TEST_MAXMSG,
	    .mq_msgsize = PRIO_TEST_MSGSIZE,
	    .mq_curmsgs = 0,
	};
	mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, PERMISSIONS, &attr2);

	if (mqdes == (mqd_t)-1) {
		t_error("mq_open() failed with errno: %s\n", strerror(errno));
	}

	// insert low priority message first
	char *prio_msg = "Low Prio";
	struct timespec ts = abstime_helper();
	TEST(mq_timedsend(mqdes, prio_msg, strlen(prio_msg), 1, &ts) == 0,
	     "mq_timedsend() failed with errno: %s\n", strerror(errno));

	// insert high priority message second (should be placed in front of low
	// priority message)
	prio_msg = "High Prio";
	ts = abstime_helper();
	TEST(mq_timedsend(mqdes, prio_msg, strlen(prio_msg), 2, &ts) == 0,
	     "mq_timedsend() failed with errno: %s\n", strerror(errno));

	char received_msg[PRIO_TEST_MSGSIZE] = {0};

	ssize_t retval = mq_receive(mqdes, received_msg, sizeof(received_msg), 0);
	if (retval == -1) {
		t_error("mq_receive() failed with errno: %s\n", strerror(errno));
	}

	TEST(strcmp(received_msg, prio_msg) == 0,
	     "The messages were not inserted into the mq in the correct order. "
	     "Expected High Prio, "
	     "got %s\n",
	     received_msg);

	mq_close(mqdes);
	mq_unlink(MQ_NAME);
	return;
}

int main(void)
{
	test_send_message();
	test_full_queue();
	test_invalid_mqdes();
	test_signal_interupt();
	test_invalid_msg_prio();
	test_invalid_block_time();
	test_oversized_msg();
	test_send_timeout();
	test_priority_ordering();
	return t_status;
}
