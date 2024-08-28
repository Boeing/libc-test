/*
 * clock_getres unit test
 */
#include "test.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void test_coarse_resolution(clockid_t clock_id,
                                   struct timespec *const original_timespec)
{
	// Test both the realtime and monotonic coarse clocks
	clockid_t coarse_clock_id = clock_id == CLOCK_REALTIME
	                                ? CLOCK_REALTIME_COARSE
	                                : CLOCK_MONOTONIC_COARSE;

	struct timespec coarse_timespec = {0, 0};
	TEST(clock_getres(coarse_clock_id, &coarse_timespec) == 0,
	     "clock_getres failed with clock id %d\n", coarse_clock_id);

	TEST(coarse_timespec.tv_nsec >= original_timespec->tv_nsec,
	     "Coarse time shouldn't be more precise than non-coarse clock time\n");
}

static void test_clock_resolution(clockid_t clock_id)
{
	struct timespec ts = {0, 0};

	TEST(clock_getres(clock_id, &ts) == 0,
	     "clock_getres failed with clock id %d\n", clock_id);

	if (clock_id == CLOCK_REALTIME || clock_id == CLOCK_MONOTONIC) {
		test_coarse_resolution(clock_id, &ts);

		/* POSIX specifies that the maximum allowable resolution for CLOCK_REALTIME
		 * and CLOCK_MONOTONIC is 20ms (1/50 of a second)
		 */
		TEST(ts.tv_sec == 0, "Resolution is too large: %d seconds\n",
		     ts.tv_sec);
		TEST(ts.tv_nsec <= 20000000, "Resolution greater than 20ms: %d\n",
		     ts.tv_nsec);
	}
}

static void test_clocks(void)
{
	// Coarse clocks are tested in the test_clock_resolution function
	clockid_t clocks_to_test[] = {
	    CLOCK_REALTIME, CLOCK_MONOTONIC,          CLOCK_MONOTONIC_RAW,
	    CLOCK_BOOTTIME, CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID};

	const size_t number_of_tests =
	    sizeof(clocks_to_test) / sizeof(*clocks_to_test);
	for (int clock = 0; clock < number_of_tests; ++clock) {
		test_clock_resolution(clocks_to_test[clock]);
	}
}

int main(void)
{
	// Test the available clocks
	test_clocks();

	// Test passing NULL into timespec argument
	TEST(clock_getres(CLOCK_MONOTONIC, NULL) == 0,
	     "Error when passing in NULL timespec\n");

	// Test passing an invalid clock
	clock_getres(-1, NULL);
	TEST(errno == EINVAL, "Expected EINVAL, got %s\n", strerror(errno));

	return t_status;
}
