/*
 * difftime unit test
 */
#include "test.h"
#include <errno.h>
#include <limits.h> // for LONG_MIN nad LONG_MAX
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SECONDS_IN_A_MINUTE 60
#define MINUTES_IN_AN_HOUR 60
#define HOURS_IN_A_DAY 24
#define DAYS_IN_A_WEEK 7
#define DAYS_IN_A_MONTH 30 // aproximated to 30
#define DAYS_IN_A_YEAR 365 // approximated to 365
#define SECONDS_IN_AN_HOUR (time_t)(MINUTES_IN_AN_HOUR * SECONDS_IN_A_MINUTE)
#define SECONDS_IN_A_DAY (time_t)(HOURS_IN_A_DAY * SECONDS_IN_AN_HOUR)
#define SECONDS_IN_A_WEEK (time_t)(DAYS_IN_A_WEEK * SECONDS_IN_A_DAY)
#define SECONDS_IN_A_MONTH (time_t)(DAYS_IN_A_MONTH * SECONDS_IN_A_DAY)
#define SECONDS_IN_A_YEAR (time_t)(DAYS_IN_A_YEAR * SECONDS_IN_A_DAY)
#define SECONDS_IN_10_YEARS (time_t)(10 * SECONDS_IN_A_YEAR)
#define SECONDS_IN_100_YEARS (time_t)(100 * SECONDS_IN_A_YEAR)

//if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

struct time_intervals {
	char *description;
	time_t seconds;
};

static void test_difftime_helper(double result, double expected)
{
	TEST(result == expected,
	     "difftime() test failed, expected %.0f (seconds), got  %.0f "
	     "(seconds)\n",
	     expected, result);
}

static void test_positive_difftime(time_t reference_time,
                                   struct time_intervals interval)
{
	time_t later_time = reference_time + interval.seconds;
	double result = difftime(later_time, reference_time);
	test_difftime_helper(result, (double)interval.seconds);
}

static void test_negative_difftime(time_t reference_time,
                                   struct time_intervals interval)
{
	time_t earlier_time = reference_time - interval.seconds;
	double result = difftime(earlier_time, reference_time);
	test_difftime_helper(result, (double)((-1) * interval.seconds));
}

static void test_difftime(void)
{
	// Array of time intervals to test (both positive and zero)
	struct time_intervals intervals[] = {
	    {"0 time difference (same time)", 0},
	    {"1 second", 1},
	    {"1 minute", SECONDS_IN_A_MINUTE},
	    {"1 hour", SECONDS_IN_AN_HOUR},
	    {"1 day", SECONDS_IN_A_DAY},
	    {"1 week", SECONDS_IN_A_WEEK},
	    {"1 month", SECONDS_IN_A_MONTH},
	    {"1 year", SECONDS_IN_A_YEAR},
	    {"10 year", SECONDS_IN_10_YEARS},
	    {"100 year", SECONDS_IN_100_YEARS},
	    {"LONG_MAX", LONG_MAX}, // edge case: large pos time
	    {"LONG_MIN", LONG_MIN}, // edge case: large neg time
	};

	// Get the current time as the reference point
	time_t current_time = time(NULL);
	if (current_time == (time_t)-1 || errno != 0) {
		t_error("time() did not work correctly, Errno: %s\n", strerror(errno));
		return;
	}

	// loop through the time intervals for positive and negative time differences
	int LOOP_LEN = sizeof(intervals) / sizeof(intervals[0]);
	for (int i = 0; i < LOOP_LEN; ++i) {

		test_positive_difftime(current_time, intervals[i]);

		test_negative_difftime(current_time, intervals[i]);
	}
	return;
}

int main(void)
{
	test_difftime();
	return t_status;
}
