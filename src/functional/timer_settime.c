/*
 * timer_settime unit test
 */

#include "test.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TIME_NANO 500000
#define OVERSIZED_NANOSECONDS 1000000001
#define NUM_REPEATS 5

volatile int timeout_count = 0;
static void timeout_signal_counter(int signum) { timeout_count++; }

static void test_settime(void)
{
	timer_t timerid = NULL;
	if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1) {
		t_error("Failed to create timer. Errno: %s\n", strerror(errno));
		return;
	}

	struct itimerspec its = {
	    .it_value = {1, TIME_NANO},
	    .it_interval = {1, TIME_NANO},
	};

	// test timer_settime() returns 0 when successful
	TEST(timer_settime(timerid, 0, &its, NULL) == 0,
	     "Failed to set timer. Errno: %s\n", strerror(errno));

	struct itimerspec res;
	if (timer_gettime(timerid, &res) == -1) {
		t_error("Failed to get timer time. Errno: %s\n", strerror(errno));
		timer_delete(timerid);
		return;
	}

	// test values from timer_settime() are set properly
	TEST(res.it_value.tv_sec == 1 && res.it_value.tv_nsec != 0 &&
	         res.it_interval.tv_sec == 1 &&
	         res.it_interval.tv_nsec == TIME_NANO,
	     "Failed to set correct timer values\n");

	timer_delete(timerid);
}

static void test_periodic_timer(void)
{
	// setup signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = timeout_signal_counter;
	sigaction(SIGALRM, &sa, NULL);

	timer_t timerid = NULL;
	if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1) {
		t_error("Failed to create timer. Errno: %s\n", strerror(errno));
		return;
	}

	struct itimerspec its = {
	    .it_value = {0, TIME_NANO},
	    .it_interval = {0, TIME_NANO},
	};

	TEST(timer_settime(timerid, 0, &its, NULL) == 0,
	     "Failed to set timer. Errno: %s\n", strerror(errno));

	struct timespec sleep_time = {
	    .tv_sec = 1,
	    .tv_nsec = 0,
	};

	// sleep for 1 second or until 5 timer timeouts have occured
	while (
	    clock_nanosleep(CLOCK_REALTIME, 0, &sleep_time, &sleep_time) == EINTR &&
	    timeout_count < NUM_REPEATS) {
		continue;
	};

	// test that the timer repeats after timeout
	TEST(timeout_count >= NUM_REPEATS,
	     "Failed periodic timer test. Expected timeout_count value >= 5, got "
	     "%d\n",
	     timeout_count);

	timer_delete(timerid);
}

static void test_invalid_inputs(void)
{
	timer_t timerid = NULL;
	if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1) {
		t_error("Failed to create timer. Errno: %s\n", strerror(errno));
		return;
	}

	struct itimerspec its = {
	    .it_value = {0, OVERSIZED_NANOSECONDS},
	    .it_interval = {0, 0},
	};

	TEST(timer_settime(timerid, 0, &its, NULL) == -1 && errno == EINVAL,
	     "Oversized nanoseconds test failed, expected %s,got %s\n",
	     strerror(EINVAL), strerror(errno));

	errno = 0;
	its.it_value.tv_nsec = -1;

	TEST(timer_settime(timerid, 0, &its, NULL) == -1 && errno == EINVAL,
	     "Undersized nanoseconds test failed, expected %s,got %s\n",
	     strerror(EINVAL), strerror(errno));

	timer_delete(timerid);
}

static void test_timer_resets(void)
{
	timer_t timerid = NULL;
	if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1) {
		t_error("Failed to create timer. Errno: %s\n", strerror(errno));
		return;
	}

	struct itimerspec its = {
	    .it_value = {0, TIME_NANO},
	    .it_interval = {0, 0},
	};

	TEST(timer_settime(timerid, 0, &its, NULL) == 0,
	     "Failed to set time. Errno: %s\n", strerror(errno));

	its.it_value.tv_sec = 1;
	its.it_value.tv_nsec = 0;

	struct itimerspec ovalue = {.it_value.tv_nsec = 0};

	TEST(timer_settime(timerid, 0, &its, &ovalue) == 0,
	     "Failed to set time. Errno: %s\n", strerror(errno));

	struct itimerspec res;
	if (timer_gettime(timerid, &res) == -1) {
		t_error("Failed to get timer time. Errno: %s\n", strerror(errno));
		timer_delete(timerid);
		return;
	}

	// check that it_value has been overwritten
	TEST(res.it_value.tv_nsec > TIME_NANO,
	     "Timer reset test failed, expected time until expiry > %d, got %ld\n",
	     TIME_NANO, res.it_value.tv_nsec);

	// check ovalue is set correctly
	TEST(ovalue.it_value.tv_nsec != 0,
	     "timer_settime() failed to set ovalue correctly\n");

	timer_delete(timerid);
}

static void test_disarm_timer(void)
{
	timer_t timerid = NULL;
	if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1) {
		t_error("Failed to create timer. Errno: %s\n", strerror(errno));
		return;
	}

	struct itimerspec its = {
	    .it_value = {0, TIME_NANO},
	    .it_interval = {0, 0},
	};

	TEST(timer_settime(timerid, 0, &its, NULL) == 0,
	     "Failed to set time. Errno: %s\n", strerror(errno));

	its.it_value.tv_nsec = 0;

	TEST(timer_settime(timerid, 0, &its, NULL) == 0,
	     "Failed to set time. Errno: %s\n", strerror(errno));

	struct itimerspec res;

	if (timer_gettime(timerid, &res) == -1) {
		t_error("Failed to get timer time. Errno: %s\n", strerror(errno));
		timer_delete(timerid);
		return;
	}

	// setting the it_value of an armed timer to 0 should result in the timer
	// being disarmed
	TEST(res.it_value.tv_sec == 0 && res.it_value.tv_nsec == 0,
	     "Disarm timer test failed, expected it_value.tv_sec = 0, got %ld, "
	     "expected it_value.tv_nsec = 0, got %ld\n",
	     res.it_value.tv_sec, res.it_value.tv_nsec);

	timer_delete(timerid);
}

int main(void)
{
	test_settime();
	test_periodic_timer();
	test_invalid_inputs();
	test_timer_resets();
	test_disarm_timer();
	return t_status;
}
