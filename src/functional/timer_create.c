/*
 * timer_create test
 */

#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

// if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define NOT_A_REAL_CLOCK 99

volatile static int thread_cpu_timer_flag = 0;

static void alarm_handler(int signal) { thread_cpu_timer_flag = 1; }

static void test_timer_creation(void)
{
	clockid_t clock_ids[] = {
	    CLOCK_REALTIME,
#if defined _POSIX_CPUTIME
	    CLOCK_PROCESS_CPUTIME_ID,
#endif
#if defined _POSIX_THREAD_CPUTIME
	    CLOCK_THREAD_CPUTIME_ID,
#endif
	};

	// setup mapping to execute alarm_handler when SIGALRM signal is received
	struct sigaction timer_sigaction = {
	    .sa_handler = alarm_handler,
	};

	sigemptyset(&timer_sigaction.sa_mask); // initialise an empty a signal
	                                       // sigaction bitmask set
	int sigaction_status = sigaction(SIGALRM, &timer_sigaction, NULL);

	if (sigaction_status != 0) {
		t_error("Sigaction for signal alarm could not be created, %s.\n",
		        strerror(errno));
		return;
	}

	const struct itimerspec timer_run = {
	    .it_value = {0, 500000},
	    .it_interval = {0, 0}}; // setup timer to expire after 0.0005 sec

	const size_t size_of_clock_ids = sizeof(clock_ids) / sizeof(clock_ids[0]);

	for (int i = 0; i < size_of_clock_ids; i++) {
		timer_t timer_id = 0;
		int status = timer_create(clock_ids[i], NULL, &timer_id);

		TEST(status == 0 && errno == 0,
		     "Timer %i couldn't be created with status code %i, %s.\n",
		     clock_ids[i], status, strerror(errno));
		TEST(timer_id >= 0, "Timer %i returned negative timer id, %s.\n",
		     clock_ids[i], strerror(errno));
		timer_settime(timer_id, 0, &timer_run,
		              NULL); // apply the expiration increment to the timer

		while (1) {
			if (thread_cpu_timer_flag == 1) {
				TEST(thread_cpu_timer_flag == 1,
				     "SIGALARM signal not caught: %i.\n",
				     thread_cpu_timer_flag);
				break;
			}
		}

		thread_cpu_timer_flag = 0; // reset flag
		errno = 0;                 // reset errno
		timer_delete(timer_id);
	}
	return;
}

static void test_signal_pending_resource_exhaustion(void)
{
#if defined RLIMIT_SIGPENDING
	struct rlimit rlim = {0, 0};
	errno = 0; // reset errno
	struct rlimit original_limit = {0, 0};

	// Test case 3: trigger return of EAGAIN from lack of queuing resources
	int get_status = getrlimit(RLIMIT_SIGPENDING, &original_limit);
	rlim.rlim_cur = 0;
	rlim.rlim_max = original_limit.rlim_max;

	int set_status =
	    setrlimit(RLIMIT_SIGPENDING,
	              &rlim); // set resource limit from RLIMIT_SIGPENDING to 0

	if (set_status != 0 || get_status != 0) {
		t_error("RLIMIT_SIGPENDING was not altered as expected.\n");
	}

	timer_t timer_id = 0;
	int status = timer_create(CLOCK_REALTIME, NULL, &timer_id);

	TEST(status == -1 && errno == EAGAIN,
	     "Timer resource limit test did not trigger %i errno EAGAIN as "
	     "expected, %s.\n",
	     status, strerror(errno));

	if (status == 0) {
		timer_delete(timer_id);
	}
	errno = 0;

	int revert_set_status = setrlimit(RLIMIT_SIGPENDING,
	                                  &original_limit); // reset resource limit
	if (revert_set_status != 0) {
		t_error("RLIMIT_SIGPENDING was not reverted as expected, %i.\n",
		        strerror(errno));
	}
#endif
	return;
}

static void test_allowed_timer_clocks(void)
{
	errno = 0; // reset errno

	// NOTE: The driver implementing CLOCK_SGI_CYCLE (10) got removed. The clock
	// ID is kept as a place holder but is not to be used.

	clockid_t clock_ids[] = {CLOCK_REALTIME,
	                         CLOCK_MONOTONIC,
#if defined _POSIX_CPUTIME
	                         CLOCK_PROCESS_CPUTIME_ID,
#endif
#if defined _POSIX_THREAD_CPUTIME
	                         CLOCK_THREAD_CPUTIME_ID,
#endif
	                         CLOCK_MONOTONIC_RAW,
	                         CLOCK_REALTIME_COARSE,
	                         CLOCK_MONOTONIC_COARSE,
	                         CLOCK_BOOTTIME,
	                         /* CLOCK_REALTIME_ALARM,
	                         CLOCK_BOOTTIME_ALARM, */ // Require CAP_WAKE_ALARM permissions for ENOTSUP else EPERM
	                         CLOCK_TAI,
	                         NOT_A_REAL_CLOCK};

	const size_t size_of_clock_ids = sizeof(clock_ids) / sizeof(clock_ids[0]);

	// Test case 2: trigger return of errnos (all possible clock ids +1)
	for (int i = 0; i < size_of_clock_ids; i++) {
		timer_t timer_id = 0;
		int status = timer_create(clock_ids[i], NULL, &timer_id);

		switch (clock_ids[i]) {
		// allowed
		case CLOCK_REALTIME:
		case CLOCK_MONOTONIC:
#if defined _POSIX_CPUTIME
		case CLOCK_PROCESS_CPUTIME_ID:
#endif
#if defined _POSIX_THREAD_CPUTIME
		case CLOCK_THREAD_CPUTIME_ID:
#endif
		case CLOCK_BOOTTIME:
		case CLOCK_TAI:
			TEST(
			    status == 0,
			    "Allowed timer %i could not be successfully created, %i: %s.\n",
			    clock_ids[i], errno, strerror(errno));
			break;
		// not allowed
		case CLOCK_MONOTONIC_RAW:
		case CLOCK_REALTIME_COARSE:
		case CLOCK_MONOTONIC_COARSE:
			TEST(status == -1 && errno == ENOTSUP,
			     "Unsupported timer %i expected failure as ENOTSUP.\n",
			     clock_ids[i]);
			break;
		// invalid
		case NOT_A_REAL_CLOCK:
			TEST(status == -1 && errno == EINVAL,
			     "Invalid timer %i expected failure as EINVAL.\n",
			     clock_ids[i]);
			break;
		}
		int deleted = timer_delete(timer_id);
		// delete timers that successfully get made
		if (status == 0 && deleted != 0) {
			t_error("Timer could not be deleted, %s.\n", strerror(errno));
		}
		errno = 0; // reset errno
	};
	return;
}

int main(void)
{
	// Test case 1: Nominal creation of timer for minimally supported clocks
	test_timer_creation();

	test_allowed_timer_clocks();

	test_signal_pending_resource_exhaustion();

	// NOTE: ENOTSUP error based on implementation defined permission for an
	// alternate thread to use another thread's clock_id for its own timer is
	// not tested based on mutually exclusive nature of how clock_id enums are
	// processed which is also not functionality directly related to
	// timer_create.

	return t_status;
}
