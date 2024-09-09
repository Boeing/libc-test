/*
 * timer_gettime unit test
 */

#include "test.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

#define IT_INTERVAL_SEC 123
#define IT_INTERVAL_NSEC 456

static void dummy_timer_notify_function(union sigval value) {};

static void test_gettime(struct itimerspec its)
{
	// setup sigevent struct
	struct sigevent sev = {
	    .sigev_notify = SIGEV_NONE,
	};

	timer_t timerid = NULL;
	struct itimerspec res;

	// create new timer
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
		t_error("Failed to create timer: %s\n", strerror(errno));
		return;
	}

	// check timer_gettime succeeds with valid timerid
	// check it_value=0 when timer is disarmed
	TEST(timer_gettime(timerid, &res) == 0 && res.it_value.tv_sec == 0 &&
	         res.it_value.tv_nsec == 0,
	     "Failed disarmed timer test. Errno: %s\n", strerror(errno));

	// arm timer
	if (timer_settime(timerid, 0, &its, NULL) == -1) {
		t_error("Failed to set timer: %s\n", strerror(errno));
		timer_delete(timerid);
		return;
	}

	// check timer_gettime correctly sets values within itimerspec struct
	TEST(timer_gettime(timerid, &res) == 0 && res.it_value.tv_sec == 0 &&
	         res.it_value.tv_nsec != 0 &&
	         res.it_interval.tv_sec == IT_INTERVAL_SEC &&
	         res.it_interval.tv_nsec == IT_INTERVAL_NSEC,
	     "Failed armed timer test. Errno : %s\n", strerror(errno));

	// delete timer
	timer_delete(timerid);
}

/*
 *	When the sigevent struct attribute sigev_notify is set to SIGEV_THREAD,
 *	timer_create() passes a pthread id value into the timerid field. This test
 *	checks that the timer_gettime() function performs correctly in this
 *	circumstance.
 */
static void test_sigev_thread(struct itimerspec its)
{
	// setup sigevent struct
	struct sigevent sev = {
	    .sigev_notify = SIGEV_THREAD,
	    .sigev_notify_function = dummy_timer_notify_function,
	    .sigev_notify_attributes = NULL,
	};

	// create timer
	timer_t timerid = NULL;
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
		t_error("Failed to create timer: %s\n", strerror(errno));
		return;
	}

	// arm timer
	if (timer_settime(timerid, 0, &its, NULL) == -1) {
		t_error("Failed to set timer: %s\n", strerror(errno));
		timer_delete(timerid);
		return;
	}

	struct itimerspec res;

	// check timer_gettime correctly sets values within itimerspec struct
	TEST(timer_gettime(timerid, &res) == 0 && res.it_value.tv_sec == 0 &&
	         res.it_value.tv_nsec != 0 &&
	         res.it_interval.tv_sec == IT_INTERVAL_SEC &&
	         res.it_interval.tv_nsec == IT_INTERVAL_NSEC,
	     "Failed SIGEV_THREAD armed timer test. Errno: %s\n", strerror(errno));

	// delete timer
	timer_delete(timerid);
}

int main(void)
{
	// setup itimerspec struct
	struct itimerspec its = {
	    .it_value.tv_sec = 1,
	    .it_value.tv_nsec = 0,
	    .it_interval.tv_sec = IT_INTERVAL_SEC,
	    .it_interval.tv_nsec = IT_INTERVAL_NSEC,
	};

	test_gettime(its);
	test_sigev_thread(its);

	return t_status;
}
