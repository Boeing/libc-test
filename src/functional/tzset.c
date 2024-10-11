/*
 * tzset test
 */

#define _GNU_SOURCE
#include "test.h"
#include "utils.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

// if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define HOURS_IN_A_DAY 24

struct test_cases {
	char *name; // readable timezone name
	int daylight;
	long timezone;      // offset from UTC in seconds
	long base_timezone; // record of absolute TZ offset from UTC without DST in
	                    // seconds
	char *tzname[2]; // timezone abbreviation ([1] is linux known abbreviations)
};

static struct test_cases test_timezones[] = {
    // {name, daylight saving amount, timezone, tzname}
    {"Universal", 0, 0, 0, {"UTC", ""}},
    {"Australia/Queensland", 0, 10, 36000, {"AEST", ""}},
    {"Europe/Amsterdam", 1, 1, 3600, {"CET", "CEST"}},
    {"Europe/Belfast", 1, 0, 0, {"GMT", "BST"}},
    {"America/Chicago", 1, -6, -21600, {"CST", "CDT"}},
    {"America/Indianapolis", 1, -5, -18000, {"EST", "EDT"}},
    {"America/Los_Angeles", 1, -8, -28800, {"PST", "PDT"}},
    // Approaching largest leading UTC limit
    {"Pacific/Auckland", 1, 12, 43200, {"NZST", "NZDT"}},
    // Approaching largest lagging UTC limits (NOTE Samoa change from UTC+13 to
    // UTC-11 in 2011)
    {"Pacific/Samoa", 0, -11, -39600, {"SST", ""}},
    {NULL, 0, 0, 0, {"", ""}}};

static int test_overflow_timezone_string(void)
{
	char str[PATH_MAX + 1];
	char long_buffer[PATH_MAX + 1];
	const char ascii_A = 0x41;
	memset(long_buffer, ascii_A, (PATH_MAX + 1) * sizeof(char));

	int result = setenv("TZ", str, 1);

	return result == 0 ? -1 : 0;
}

static void generate_current_tzs(void)
{
	time_t check_time = 0;
	struct test_cases *case_pt = {0};

	for (case_pt = test_timezones; case_pt->name != NULL; case_pt++) {
		// get tm_isdst of each timezone
		if (setenv("TZ", case_pt->name, 1) != 0) {
			t_error("Unable to setenv `TZ` as %s error %s\n",
			        case_pt->tzname[1], strerror(errno));
		}
		tzset();

		check_time = time(NULL);
		struct tm dst_check;
		localtime_r(&check_time, &dst_check);
		if (dst_check.tm_zone == NULL) {
			t_error("Could not check current time.\n");
		}

		// timezone entires contingent on daylight savings logic
		if (dst_check.tm_isdst > 0) { // positive value if dst is in effect
			case_pt->timezone += case_pt->daylight;
		}
		// 0 if dst not set, -1 if no information
	}
}

void set_confirmed_timezone(char *timezone_name, struct tm *time_result)
{
	if (setenv("TZ", timezone_name, 1) != 0 ||
	    setenv("TZ", timezone_name, 0) != 0) {
		t_error("Unable to setenv `TZ` as %s error %s\n", timezone_name,
		        strerror(errno));
	}

	char *check_settz = getenv("TZ");
	if (check_settz == NULL) {
		check_settz = "Universal"; // default
	}

	TEST(strcmp(check_settz, timezone_name) == 0,
	     "Getenv `TZ` returned an unexpected result of %s when expecting %s.\n",
	     check_settz, timezone_name);
	tzset();

	time_t result_unix_time = time(NULL);
	if (result_unix_time == -1) {
		t_error("time was not set as expected, %s.\n", strerror(errno));
	}

	struct tm *time_check = localtime_r(&result_unix_time, time_result);
	TEST(time_check != NULL,
	     "timezone struct was not set as expected using localtime_r, %s.\n",
	     strerror(errno));

	struct test_cases *case_pt = {0};
	for (case_pt = test_timezones; case_pt->name != NULL; case_pt++) {
		if (case_pt->name == timezone_name) {
			TEST(daylight == case_pt->daylight,
			     "The existence of daylight savings for this TZ is "
			     "incorrect.\n");
			TEST((-1 * timezone) == case_pt->base_timezone,
			     "The %i vs %i timezone seconds difference were not as "
			     "expected.\n",
			     (-1 * timezone), case_pt->base_timezone);
			TEST(strcmp(tzname[0], case_pt->tzname[0]) == 0,
			     "The actual %s vs expected %s timezone standard strings were "
			     "not as "
			     "expected.\n",
			     tzname[0], case_pt->tzname[0]);
			TEST(strcmp(tzname[1], case_pt->tzname[1]) == 0,
			     "The actual %s vs expected %s timezone standard strings were "
			     "not as "
			     "expected.\n",
			     tzname[1], case_pt->tzname[1]);
		}
	}

	return;
}

static void valid_timezone_compare(const struct test_cases *case_pt)
{
	int hours_difference = 0;

	// check that the timezone - UTC equals the expected number of seconds
	// difference
	struct tm utc_check;
	set_confirmed_timezone("Universal", &utc_check);

	// get Unix timezone time to compare
	struct tm local_check;
	set_confirmed_timezone(case_pt->name, &local_check);

	if (local_check.tm_mday == utc_check.tm_mday) {
		hours_difference = local_check.tm_hour - utc_check.tm_hour;
	} else {
		hours_difference =
		    local_check.tm_hour < utc_check.tm_hour
		        ? HOURS_IN_A_DAY + local_check.tm_hour - utc_check.tm_hour
		        : -1 * (utc_check.tm_hour - local_check.tm_hour +
		                HOURS_IN_A_DAY);
	}

	int impossible_time =
	    ((mktime(&local_check) > 0) && (mktime(&utc_check) > 0)) ? 0 : 1;

	int hours_alignment = (hours_difference == case_pt->timezone) ? 0 : 1;

	int minutes_alignment = local_check.tm_min - utc_check.tm_min;

	TEST(hours_alignment == 0 && minutes_alignment == 0 && impossible_time == 0,
	     "Timezone %s expected diff %i was %i difference, error %s\n",
	     case_pt->name, (case_pt->timezone + case_pt->daylight),
	     hours_difference, strerror(errno));

	return;
}

static void invalid_timezone_compare(void)
{
	// default state
	struct tm utc_check;
	set_confirmed_timezone("Universal", &utc_check);

	// Test case 2: setting timezone using invalid string
	struct tm neverland_check;
	set_confirmed_timezone("Neverland/Time", &neverland_check);

	// ensure time set is actually still UTC for invalid timezone string
	time_t utc_seconds = mktime(&utc_check);
	time_t neverland_seconds = mktime(&neverland_check);
	if (utc_seconds == -1 || neverland_seconds == -1) {
		t_error("mktime function did not return normally, %s.\n",
		        strerror(errno));
	}

	// 1 second tolerance to mitigate edge case on second tick-over
	TEST(utc_seconds == neverland_seconds ||
	         utc_seconds == (neverland_seconds - 1),
	     "Expected UTC time upon invalid timezone string use was not achieved. "
	     "Expected %i, got %i, %s.\n",
	     utc_seconds, neverland_seconds, strerror(errno));

	// Test case 3: setting timezone using empty string (also proxy for testing
	// timzone reset to default UTC) reset to UTC time in case undefined
	// behaviour has occured in test 2
	set_confirmed_timezone("Universal", &utc_check);

	int success = setenv("TZ", "", 1);
	tzset();

	time_t result_empty_tz = time(NULL);
	if (result_empty_tz == -1) {
		t_error("time function did not return normally, %s.\n",
		        strerror(errno));
	}
	struct tm emptytz_check;
	struct tm *result = localtime_r(&result_empty_tz, &emptytz_check);
	if (result == NULL) {
		t_error("localtime_r function did not return normally, %s.\n",
		        strerror(errno));
	}
	utc_seconds = mktime(&utc_check);
	time_t test_seconds = mktime(result);
	if (utc_seconds == -1 || test_seconds == -1) {
		t_error("mktime function did not return normally, %s.\n",
		        strerror(errno));
	}

	TEST(success == 0,
	     "Expected UTC time upon invalid timezone string use was not achieved. "
	     "Expected %i, got %i, %s.\n",
	     utc_seconds, test_seconds, strerror(errno));

	// Test case 4: setting timezone to a random string of size PATH_MAX+1 that
	// causes string buffer overflow
	test_buffer_overflow(test_overflow_timezone_string);

	// reset errno
	errno = 0;
	return;
}

int main(void)
{
	const struct test_cases *pt = {0};

	// start from a known UTC state
	struct tm utc_check;
	set_confirmed_timezone("Universal", &utc_check);

	generate_current_tzs();

	// Test case 1: valid inputs nominal behaviour
	for (pt = test_timezones; pt->name != NULL; pt++) {
		valid_timezone_compare(pt);
	};

	// Test case 2 - 4: invalid inputs and edge cases
	invalid_timezone_compare();

	// NOTE: tzset() as per the POSIX standard is not thread safe

	return t_status;
}
