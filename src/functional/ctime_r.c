/*
 * ctime_r unit test
 */

#include "test.h"
#include "utils.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TBUFSIZE 100
#define NUM_THREADS 10
#define NUM_REENTRANTS 1000
#define SECONDS_IN_A_DAY 86400

// ctime would return a time string like "Thu Jan  1 00:00:00 1970\n"
#define CTIME_STR_LEN 25                  // length of the ctime_r return string
#define CTIME_BUF_LEN (CTIME_STR_LEN + 1) // buffer size for returned string
#define CTIME_CHR_LEN 3 // Day and month only first 3 chars in the time string
#define CTIME_MON_IND (CTIME_CHR_LEN + 1) // Month index + ' '
#define CTIME_DOM_IND \
	(CTIME_MON_IND + CTIME_CHR_LEN + 1) // Day of month index + ' '
#define CTIME_DOM_LEN 2                 // Day of Month 2 chars
#define CTIME_TIM_IND (CTIME_DOM_IND + CTIME_DOM_LEN + 1) // Time index + ' '
#define CTIME_TIM_HH_IND (CTIME_TIM_IND)                  // Time HH index
#define CTIME_TIM_HH_LEN 2                                // HH takes 2 chars
#define CTIME_TIM_MM_IND \
	(CTIME_TIM_HH_IND + CTIME_TIM_HH_LEN + 1) // Time HH index
#define CTIME_TIM_MM_LEN 2                    // MM takes 2 chars
#define CTIME_TIM_SS_IND \
	(CTIME_TIM_MM_IND + CTIME_TIM_MM_LEN + 1) // Time HH index
#define CTIME_TIM_LEN 8                       // HH:MM:SS takes 28 chars
#define CTIME_YER_IND (CTIME_TIM_IND + CTIME_TIM_LEN + 1) // Year index
#define CTIME_YER_LEN 4                                   // YYYY takes 4 chars
#define CTIME_NUM_MONTHS 12                               // number of Months
#define CTIME_NUM_DAYS 7                                  // number of Days

// ISO C defines the tm_year member of the tm struct as 'years after 1900'
// The nearest value to overflow is 10000 - 1900 = 8100.
#define OVERFLOW_YEAR \
	(10000 - 1900) // Network Time Protocol epoch is 1900-01-01

struct result {
	pthread_t thread_id;
	char time_str[CTIME_BUF_LEN];
	time_t rawtime;
	int status;
	int index;
};

static int check_date_digit(char c)
{
	return ((c >= '0' && c <= '9') || c == ' ');
}

static int check_time_digit(char c)
{
	return ((c >= '0' && c <= '9') || c == ' ' || c == ':');
}

static void validate_ctime_r_output_for_valid_day(const char *time_str)
{
	TEST(time_str != NULL, "time_str is not valid\n");

	int valid_day = -1;
	// Check day of the week
	const char *valid_days[] = {"Sun", "Mon", "Tue", "Wed",
	                            "Thu", "Fri", "Sat"};

	for (int i = 0; i < CTIME_NUM_DAYS; ++i) {
		if (strncmp(time_str, valid_days[i], CTIME_CHR_LEN) == 0) {
			valid_day = 0;
			break;
		}
	}

	TEST(valid_day == 0, "Valid day was not found in the string: %s\n",
	     time_str);

	return;
}

static void validate_ctime_r_output_for_valid_month(const char *time_str)
{
	TEST(time_str != NULL, "time_str is not valid\n");

	int valid_month = -1;
	const char *valid_months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	// Check month
	for (int i = 0; i < CTIME_NUM_MONTHS; ++i) {
		if (strncmp(time_str + CTIME_MON_IND, valid_months[i], CTIME_CHR_LEN) ==
		    0) {
			valid_month = 0;
			break;
		}
	}
	TEST(valid_month == 0, "Valid month was not found in the string: %s\n",
	     time_str);

	return;
}

static void validate_ctime_r_output_for_valid_dom(const char *time_str)
{
	TEST(time_str != NULL, "time_str is not valid\n");

	// Check day of the month (DD), should be 2 chars
	int valid_dom = 0;
	for (int i = 0; i < CTIME_DOM_LEN; ++i) {
		if (check_date_digit(time_str[CTIME_DOM_IND + i]) == 0) {
			valid_dom = -1;
			break;
		}
	}
	TEST(valid_dom == 0,
	     "Valid day of the month was not found in the string: %s\n", time_str);

	return;
}

static void validate_ctime_r_output_for_valid_time(const char *time_str)
{
	TEST(time_str != NULL, "time_str is not valid\n");

	// Check time (HH:MM:SS), should be 8 chars
	int valid_time = 0;
	for (int i = 0; i < CTIME_TIM_LEN; ++i) {
		if (check_time_digit(time_str[CTIME_TIM_IND + i]) == 0) {
			valid_time = -1;
			break;
		}
	}
	TEST(valid_time == 0, "Valid time was not found in the string: %s\n",
	     time_str);

	TEST(time_str[CTIME_TIM_HH_IND + CTIME_TIM_HH_LEN] == ':',
	     "Valid time format ':' was not found in the string: %s\n", time_str);

	TEST(time_str[CTIME_TIM_MM_IND + CTIME_TIM_MM_LEN] == ':',
	     "Valid time format ':' was not found in the string: %s\n", time_str);

	return;
}

static void validate_ctime_r_output_for_valid_year(const char *time_str)
{
	TEST(time_str != NULL, "time_str is not valid\n");

	// Check year YYYY, should be 4 chars
	int valid_year = 0;
	for (int i = 0; i < CTIME_YER_LEN; ++i) {
		if (check_date_digit(time_str[CTIME_YER_IND + i]) == 0) {
			valid_year = -1;
			break;
		}
	}
	TEST(valid_year == 0, "Valid year was not found in the string: %s\n",
	     time_str);

	return;
}

static void validate_ctime_r_output(const char *time_str)
{
	TEST(time_str != NULL, "time_str is not valid\n");

	// Check the length of the string and new line char
	TEST(strlen(time_str) == CTIME_STR_LEN,
	     "Expected strlen: %d, but got: %d\n", CTIME_STR_LEN, strlen(time_str));
	// Check the  new line char at the end of the string
	TEST(time_str[CTIME_STR_LEN - 1] == '\n',
	     "Expected char: %d, but got: %d\n", '\n', time_str[CTIME_STR_LEN - 1]);

	validate_ctime_r_output_for_valid_day(time_str);

	validate_ctime_r_output_for_valid_month(time_str);

	validate_ctime_r_output_for_valid_dom(time_str);

	validate_ctime_r_output_for_valid_time(time_str);

	validate_ctime_r_output_for_valid_year(time_str);

	return;
}

static int compare_ctime_r_output(const char *actual, const char *expected)
{
	if (actual == NULL || expected == NULL) {
		return -1;
	}

	// Check the length of the string and new line char
	if (strlen(actual) != CTIME_STR_LEN || actual[CTIME_STR_LEN - 1] != '\n') {
		return -1;
	}

	if (strlen(expected) != CTIME_STR_LEN ||
	    expected[CTIME_STR_LEN - 1] != '\n') {
		return -1;
	}

	if (strncmp(actual, expected, CTIME_STR_LEN) != 0) {
		return -1;
	}

	return 0;
}

static void reentrant_call(int depth, time_t *rawtime, void *buffer)
{
	if (depth <= 0)
		return;

	for (int i = 0; i < depth; ++i) {
		errno = 0;
		char *result = ctime_r(rawtime, (char *)buffer);
		TEST(result != NULL && errno == 0, "%s\n", strerror(errno));
	}
	return;
}

// Thread function for testing ctime_r in a multiple-threaded context
static void *thread_safety_function(void *arg)
{
	struct result *res = (struct result *)arg;

	res->rawtime += (time_t)(res->index * SECONDS_IN_A_DAY);
	reentrant_call(NUM_REENTRANTS, &res->rawtime, res->time_str);

	res->thread_id = pthread_self();
	res->status = 0;
	return NULL;
}

// Function to test ctime_r thread safety
static void test_ctime_r_thread_safety_and_reentrancy(struct result results[])
{
	// Read the current time once before creating the threads
	time_t basetime = time(NULL);

	// Initialize thread data
	for (int i = 0; i < NUM_THREADS; ++i) {
		results[i].index = i;
		results[i].status = -1;
		results[i].rawtime = basetime;
	}
	// Create threads
	create_and_join_threads(NUM_THREADS, NULL, thread_safety_function, results,
	                        sizeof(struct result));

	char expected_time_str[CTIME_BUF_LEN] = {0};
	char *result = NULL;

	// Validate the results after all threads have finished
	for (int i = 0; i < NUM_THREADS; ++i) {
		TEST(results[i].status == 0,
		     "Thread safety function failed to execute correctly for "
		     "Thread_id: %d\n",
		     results[i].thread_id);

		if (results[i].status == 0) {
			// Verify that the ctime_r returns a 24-character string +'\n'+'\0'
			validate_ctime_r_output(results[i].time_str);

			errno = 0; // reset the errno before calling localtime_r
			result = ctime_r(&results[i].rawtime, expected_time_str);
			TEST(result != NULL && errno == 0, "%s\n", strerror(errno));

			// Verify that the actual values match the expected values
			TEST(compare_ctime_r_output(results[i].time_str,
			                            expected_time_str) == 0,
			     "actual values did not match the expected values\n");
		}
	}
	return;
}

static void test_ctime_r(void)
{
	time_t rawtime = 0;
	char buffer[CTIME_BUF_LEN] = {
	    0}; // ctime_r requires a buffer of at least 26 bytes

	// Test case 1: Valid time_t input (current time)
	time(&rawtime);
	char *result = ctime_r(&rawtime, buffer);
	TEST(result != NULL && errno == 0, "%s\n", strerror(errno));
	validate_ctime_r_output(buffer);

	// Test case 2: Edge case (time_t value of 0, the Unix epoch)
	rawtime = 0;
	result = ctime_r(&rawtime, buffer);
	TEST(result != NULL && errno == 0, "%s\n", strerror(errno));
	validate_ctime_r_output(buffer);
	TEST(strncmp(buffer, "Thu Jan  1 00:00:00 1970\n", CTIME_STR_LEN) == 0 &&
	         errno == 0,
	     "Returned string did not match the Unix epoch time. Error: %s\n",
	     strerror(errno)); // exact string for Unix epoch

	// Test case 3: Out-of-range time_t value (far future date)
	rawtime = INT_MAX; // This is the upper bound for a time_t value
	result = ctime_r(&rawtime, buffer);
	TEST(result != NULL && errno == 0, "%s\n", strerror(errno));
	validate_ctime_r_output(buffer);

	// Test case 4: Far past date (time_t minimum value)
	rawtime = INT_MIN; // Minimum time_t value
	result = ctime_r(&rawtime, buffer);
	TEST(result != NULL && errno == 0, "%s\n", strerror(errno));
	validate_ctime_r_output(buffer);

#if 0
	// These tests are turned off for the moment due to ctime_r
	// not handling invalid arguments gracefully
	// Test case 5: Invalid time_t pointer (NULL)
	result = ctime_r(NULL, buffer);
	TEST(result == NULL && errno == 0, "%s\n", strerror(errno));
	validate_ctime_r_output(buffer);

	// Test case 6: Invalid buffer pointer (NULL)
	time(&rawtime);
	result = ctime_r(&rawtime, NULL);
	TEST(result == NULL && errno == 0, "%s\n", strerror(errno));
	validate_ctime_r_output(buffer);

	// Test case 7: Both time_t and buffer pointer are NULL
	result = ctime_r(NULL, NULL);
	TEST(result == NULL && errno == 0, "%s\n", strerror(errno));
	validate_ctime_r_output(buffer);
#endif

	// Test case 8: Test for year overflow (EOVERFLOW)
	errno = 0;
	rawtime = (time_t)LONG_MAX; // Assuming this causes overflow
	result = ctime_r(&rawtime, buffer);
	TEST(result == NULL && errno == EOVERFLOW, "%s\n", strerror(errno));

	// Test case 9: Test for year overflow (EOVERFLOW)
	errno = 0;
	rawtime = (time_t)LONG_MIN; // Assuming this causes overflow
	result = ctime_r(&rawtime, buffer);
	TEST(result == NULL && errno == EOVERFLOW, "%s\n", strerror(errno));

	return;
}

static int test_ctime_r_buffer_overflow(void)
{
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(timeinfo));
	// tm_year is formatted with %d in asctime_r, and writing a 5 digit year
	// will cause the buffer to overflow
	timeinfo.tm_year = OVERFLOW_YEAR;

	time_t seconds = mktime(&timeinfo);
	char buffer[CTIME_BUF_LEN] = {
	    0}; // ctime_r requires a buffer of at least 26 bytes

	const char *result = NULL;
	if (seconds != -1) {
		// mktime() returns a day shorter than 10000 years when converting
		// 'tm_year = OVERFLOW_YEAR' to seconds
		seconds += SECONDS_IN_A_DAY;
		// musl implementation will expect the function to crash, others will
		// return NULL
		result = ctime_r(&seconds, buffer);
	}
	return (result == NULL ? 0 : 1);
}

int main(void)
{
	struct result results[NUM_THREADS];
	test_ctime_r();
	test_ctime_r_thread_safety_and_reentrancy(results);
	test_buffer_overflow(test_ctime_r_buffer_overflow);
	return t_status;
}

