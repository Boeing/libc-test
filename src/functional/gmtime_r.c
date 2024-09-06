/*
 * gmtime_r.c unit test
 */
#include "test.h"
#include "utils.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <time.h>


#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

// Compare the values of two tm structures for equality
#define TM_COMPARE(x, y) \
	(((x) == (y)) || t_error("Expected " #x " %d to equal " #y " %d\n", x, y))

#define TM_BOUNDS(lower, x, upper) \
	((((x) <= (upper)) && ((x) >= (lower))) || \
	 t_error("Expected %d to be less than upper bound %d, and greater than " \
	         "lower bound %d", \
	         x, upper, lower))

#define TM(ss, mm, hh, md, mo, yr, wd, yd, dst) \
	(struct tm) \
	{ \
		.tm_sec = (ss), .tm_min = (mm), .tm_hour = (hh), .tm_mday = (md), \
		.tm_mon = (mo), .tm_year = (yr), .tm_wday = (wd), .tm_yday = (yd), \
		.tm_isdst = (dst) \
	}

#define MAX_THREADS 59
#define MAX_ITERATIONS 100

// 00:00:00 UTC, 1st January 1970
#define TM_EPOCH TM(0, 0, 0, 1, 0, 70, 4, 0, 0)

// 3:14:07 UTC, 19th January 2038
#define TM_Y2038_1S TM(7, 14, 3, 19, 0, 138, 2, 18, 0)

// 3:14:08 UTC, 19th January 2038
#define TM_Y2038 TM(8, 14, 3, 19, 0, 138, 2, 18, 0)

// Leap year 29th Feb 2024
#define TM_LEAPYEAR TM(0, 0, 0, 29, 1, 124, 4, 59, 0)

static void compare_tm_structs(const struct tm *tm1, const struct tm *tm2)
{
	TM_COMPARE(tm1->tm_sec, tm2->tm_sec);
	TM_COMPARE(tm1->tm_min, tm2->tm_min);
	TM_COMPARE(tm1->tm_hour, tm2->tm_hour);
	TM_COMPARE(tm1->tm_mday, tm2->tm_mday);
	TM_COMPARE(tm1->tm_mon, tm2->tm_mon);
	TM_COMPARE(tm1->tm_year, tm2->tm_year);
	TM_COMPARE(tm1->tm_wday, tm2->tm_wday);
	TM_COMPARE(tm1->tm_yday, tm2->tm_yday);
	TM_COMPARE(tm1->tm_isdst, tm2->tm_isdst);
}

static void test_tm_bounds(const struct tm *tm)
{
	// See ISO C99 Section 7.23.1 for more information
	// 60 to account for leap seconds on certain systems
	TM_BOUNDS(0, tm->tm_sec, 60);
	TM_BOUNDS(0, tm->tm_min, 59);
	TM_BOUNDS(0, tm->tm_hour, 23);
	TM_BOUNDS(1, tm->tm_mday, 31);
	TM_BOUNDS(0, tm->tm_mon, 11);
	TM_BOUNDS(0, tm->tm_wday, 6);
	TM_BOUNDS(0, tm->tm_yday, 365);
}

static void test_tm(time_t time, const struct tm *expected,
                    int overflow_expected)
{
	struct tm gmt;
	struct tm *result = gmtime_r(&time, &gmt);

	if (overflow_expected) {
		TEST(result == NULL, "gmtime_r expected NULL return, got %p", result);
		TEST(errno == EOVERFLOW, "gmtime_r expected overflow, got %s",
		     strerror(errno));
	} else {
		TEST(&gmt == result,
		     "gmtime_r returned the pointer %p, not the supplied stucture %p\n",
		     result, gmt);

		compare_tm_structs(result, expected);
	}
}

static void *get_gmtime(void *arguments)
{
	const time_t thread_time = *(time_t *)arguments;

	for (int i = 0; i < MAX_ITERATIONS; ++i) {
		struct tm tm = {0};
		struct tm *result = gmtime_r((time_t *)&thread_time, &tm);

		if (result == NULL) {
			t_error("gmtime_r returned NULL, expected populated tm struct");
			continue;
		}

		struct tm expected = TM_EPOCH;
		expected.tm_sec = (int)thread_time;
		compare_tm_structs(result, &expected);
	}

	return 0;
}

static void test_gmtime_concurrency(void)
{
	time_t gmtimes[MAX_THREADS] = {0};
	for (unsigned index = 0; index < MAX_THREADS; ++index) {
		gmtimes[index] = index;
	}

	create_and_join_threads(MAX_THREADS, NULL, get_gmtime, gmtimes,
	                        sizeof(*gmtimes));
}

static void test_current_time(void)
{
	const time_t current_time = time(NULL);
	struct tm current_tm = {0};
	const struct tm *result = gmtime_r(&current_time, &current_tm);

	if (result == NULL) {
		t_error("gmtime_r returned NULL, expected populated tm struct");
		return;
	}

	test_tm_bounds(result);
}

static void test_gmt_times(void)
{
	// Test the UNIX epoch
	const time_t epoch = 0LL;
	test_tm(epoch, &TM_EPOCH, 0);

	// Test one second before 2038
	const time_t before_2038 = INT_MAX;
	test_tm(before_2038, &TM_Y2038_1S, 0);

	// 32 bit time architectures can overflow
	const int can_overflow = (time_t)LLONG_MAX != LLONG_MAX;

	// Test the year 2038 problem
	const time_t overflow_2038 = (long long)INT_MAX + 1;
	test_tm(overflow_2038, &TM_Y2038, can_overflow);

	// Test a complete overflow
	const time_t overflow = LLONG_MAX;
	test_tm(overflow, &TM_Y2038, 1);

	// Test February 29th 2024
	const time_t leap_year = 1709164800;
	test_tm(leap_year, &TM_LEAPYEAR, 0);
}

int main(void)
{
	test_gmt_times();
	test_current_time();
	test_gmtime_concurrency();

	return t_status;
}
