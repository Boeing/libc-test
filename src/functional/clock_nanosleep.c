/*
 * clock_nanosleep unit test
 */

#include "test.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define MAX_NSEC 1000000000
#define SLEEP_NANO 500000
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void dummy_signal_handler(int signum) {}

static void test_time_elapsed(void)
{
	// get start time
	struct timespec time1;
	if (clock_gettime(CLOCK_MONOTONIC, &time1) == -1) {
		t_error("clock_gettime() failed. Errno: %s\n", strerror(errno));
		return;
	}

	struct timespec ts = {0, SLEEP_NANO};
	TEST(clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts) == 0,
	     "clock_nanosleep failed with clock id %d\n", CLOCK_MONOTONIC);

	// get finish time
	struct timespec time2;
	if (clock_gettime(CLOCK_MONOTONIC, &time2) == -1) {
		t_error("clock_gettime() failed. Errno: %s\n", strerror(errno));
		return;
	}

	// calculate elapsed time
	time_t elapsed = time2.tv_sec > time1.tv_sec
	                     ? time2.tv_nsec + (MAX_NSEC - time1.tv_nsec)
	                     : time2.tv_nsec - time1.tv_nsec;

	TEST(elapsed >= SLEEP_NANO,
	     "Elapsed time test failed: expected >= %d "
	     "nanoseconds, got %ld nanoseconds.\n",
	     SLEEP_NANO, elapsed);
}

static void test_invalid_inputs(void)
{
	struct timespec ts = {1, 0};

	// check EINVAL is returned when tv_nsec is too big
	ts.tv_nsec = MAX_NSEC;
	int retval = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
	TEST(retval == EINVAL,
	     "Oversized tv_nsec test failed: Expected %s, got %s\n",
	     strerror(EINVAL), strerror(retval));

	// check EINVAL is returned when tv_nsec is too small
	ts.tv_nsec = -1;
	retval = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
	TEST(retval == EINVAL,
	     "Undersized tv_nsec test failed: Expected %s, got %s\n",
	     strerror(EINVAL), strerror(retval));

	// check EINVAL is returned when given CPU_time clock of calling thread
	retval = clock_nanosleep(CLOCK_THREAD_CPUTIME_ID, 0, &ts, NULL);
	TEST(retval == EINVAL,
	     "Calling threads cpu clock id test failed: Expected %s, got %s\n",
	     strerror(EINVAL), strerror(retval));

	const int unknown_clock = 123;

	// check EINVAL is returned when given unknown clock
	retval = clock_nanosleep(unknown_clock, 0, &ts, NULL);
	TEST(retval == EINVAL,
	     "Unknown clock id test failed: Expected %s, got %s\n",
	     strerror(EINVAL), strerror(retval));

	// check ENOTSUP is returned when given an unsupported clock
	retval = clock_nanosleep(CLOCK_MONOTONIC_COARSE, 0, &ts, NULL);
	TEST(retval == ENOTSUP,
	     "Unsupported clock test failed. Expected %s, got %s\n",
	     strerror(ENOTSUP), strerror(retval));
}

static void test_interupted_sleep()
{
	// check EINTR is returned when sleep is interupted by a signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = dummy_signal_handler;
	sigaction(SIGALRM, &sa, NULL);

	timer_t timerid = NULL;
	if (timer_create(CLOCK_MONOTONIC, NULL, &timerid) == -1) {
		t_error("timer_create() failed. Errno: %s\n", strerror(errno));
		return;
	}

	struct itimerspec its = {
	    .it_value.tv_sec = 0,
	    .it_value.tv_nsec = SLEEP_NANO,
	};
	if (timer_settime(timerid, 0, &its, NULL) == -1) {
		t_error("timer_settime() failed. Errno: %s\n", strerror(errno));
		timer_delete(timerid);
		return;
	}

	struct timespec ts = {2, 0};
	struct timespec rmtp = {0, 0};
	int retval = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &rmtp);

	TEST(retval == EINTR, "Interupted sleep test failed: Expected %s, got %s\n",
	     strerror(EINTR), strerror(retval));

	// check that the remaining unslept time is returned into rmtp
	TEST(rmtp.tv_sec > 0 || rmtp.tv_nsec > 0,
	     "rmtp test failed: Expected value > 0, got rmpt.tv_sec = %lu, "
	     "rmtp.tv_nsec = %lu\n",
	     rmtp.tv_sec, rmtp.tv_nsec);

	timer_delete(timerid);
}

int main(void)
{
	test_time_elapsed();
	test_invalid_inputs();
	test_interupted_sleep();
	return t_status;
}
