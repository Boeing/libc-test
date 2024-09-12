/*
 * asctime_r test
 */
#include "test.h"
#include "utils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TBUFSIZE 100
#define MAX_THREADS 10

// ISO C defines the tm_year member of the tm struct as 'years after 1900'
// The nearest value to overflow is 10000 - 1900 = 8100.
#define OVERFLOW_YEAR (10000 - 1900)

static int run_buffer_overflow(void)
{
	struct tm date_time;
	memset(&date_time, 0, sizeof(date_time));

	// tm_year is formatted with %d in asctime_r, and writing a 5 digit year
	// will cause the buffer to overflow
	date_time.tm_year = OVERFLOW_YEAR;

	char time_buffer[TBUFSIZE] = {0};

	// musl implementation will expect the function to crash, others will return
	// NULL
	const char *result = asctime_r(&date_time, time_buffer);
	return result == NULL ? 0 : 1;
}

static void *get_asctime(void *args)
{
	struct tm date_time = {0};
	date_time.tm_year = *(int *)args;

	char result_buffer[TBUFSIZE] = {0};
	const char *result_date = asctime_r(&date_time, result_buffer);

	if (result_date == NULL) {
		t_error("asctime_r returned NULL, expected a populated buffer\n");
		return 0;
	}

	const int year_offset = 20;
	const int expected_year = 1900 + date_time.tm_year;
	const int result_year = atoi(&result_date[year_offset]);

	TEST(result_year == expected_year, "Expected year to be %d, got %d\n",
	     expected_year, result_year);

	return 0;
}

static void test_thread_safety(void)
{
	int asctime_years[MAX_THREADS] = {0};

	for (int year = 0; year < MAX_THREADS; ++year) {
		asctime_years[year] = year;
	}

	create_and_join_threads(MAX_THREADS, NULL, get_asctime, asctime_years,
	                        sizeof(*asctime_years));
}

static void test_datetime_format(void)
{
	const struct tm date_time = {
	    .tm_sec = 0,    // Seconds after the minute
	    .tm_min = 7,    // Minutes after the hour
	    .tm_hour = 10,  // Hours since midnight
	    .tm_mday = 4,   // Day of the month
	    .tm_mon = 6,    // Months since January
	    .tm_year = 97,  // Years since 1990
	    .tm_wday = 5,   // Days since Sunday
	    .tm_yday = 184, // Days since January 1
	    .tm_isdst = 0,  // Daylight savings
	};

	char time_buffer[TBUFSIZE] = {0};
	const char *result_date = asctime_r(&date_time, time_buffer);

	if (result_date == NULL) {
		t_error("asctime_r returned NULL, expected a populated buffer\n");
		return;
	}

	TEST(result_date == time_buffer,
	     "asctime_r returned the pointer %p, not the supplied buffer %p\n",
	     result_date, time_buffer);

	/* Using strcmp instead of strncmp, as asctime_r should always return a 26
	 * byte string. strcmp will check if the returned string is over length
	 * (indicating a buffer overflow), and fail the test, whereas strncmp would
	 * just check the 26 bytes.
	 */
	const char *expected_date = "Fri Jul  4 10:07:00 1997\n";
	TEST(!strcmp(expected_date, result_date), "Date %s does not match %s\n",
	     result_date, expected_date);
}

int main(void)
{
	test_datetime_format();
	test_thread_safety();
	test_buffer_overflow(run_buffer_overflow);

	return t_status;
}

