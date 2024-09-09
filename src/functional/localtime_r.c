/*
 * localtime_r unit test
 */

#include "test.h"
#include "utils.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define NUM_THREADS 100
#define NUM_REENTRANTS 100
#define SECONDS_IN_A_DAY 86400

// Structure to hold results for the thread safety test
struct result {
	pthread_t thread_id;
	struct tm timeinfo;
	time_t rawtime;
	int status;
	int index;
};

// Function to verify that the struct tm fields are within expected ranges
static void validate_tm_fields(const struct tm *timeinfo)
{
	TEST(timeinfo->tm_year >= 70,
	     "Years since 1900, should be atleast 70 (1970+).\n");
	TEST(timeinfo->tm_mon >= 0 && timeinfo->tm_mon < 12,
	     "Months range 0-11.\n"); // Months range 0-11
	TEST(timeinfo->tm_mday > 0 && timeinfo->tm_mday <= 31,
	     "Days of the month range 1-31.\n"); // Days of the month
	TEST(timeinfo->tm_hour >= 0 && timeinfo->tm_hour < 24,
	     "Hours within range.\n"); // Hours within range
	TEST(timeinfo->tm_min >= 0 && timeinfo->tm_min < 60,
	     "Minutes within range.\n"); // Minutes within range
	TEST(timeinfo->tm_sec >= 0 && timeinfo->tm_sec < 60,
	     "Seconds within range.\n"); // Seconds within range

	return;
}

// Function to verify that the struct tm fields are as expected
static void compare_tm_fields(const struct tm *actual,
                              const struct tm *expected)
{
	TEST(actual->tm_year == expected->tm_year, "Years did not match.\n");
	TEST(actual->tm_mon == expected->tm_mon,
	     "Months did not match.\n"); // Months range 0-11
	TEST(actual->tm_mday == expected->tm_mday,
	     "Days of the month did not match.\n"); // Days of the month
	TEST(actual->tm_hour == expected->tm_hour,
	     "Hours did not match.\n"); // Hours within range
	TEST(actual->tm_min == expected->tm_min,
	     "Minutes did not match.\n"); // Minutes within range
	TEST(actual->tm_sec == expected->tm_sec,
	     "Seconds did not match.\n"); // Seconds within range

	// Check the Daylight Saving Time flag (tm_isdst)
	TEST((actual->tm_isdst == expected->tm_isdst),
	     "Daylight Saving Time flag did not match.\n");
	return;
}

void reentrant_call(int depth, time_t *rawtime, void *buffer)
{
	if (depth <= 0)
		return;

	for (int i = 0; i < depth; ++i) {
		errno = 0;
		// Call localtime_r and store the result
		struct tm *result = localtime_r(rawtime, (struct tm *)buffer);
		TEST(result != NULL && errno == 0, "%s\n", strerror(errno));
	}
	return;
}

// Function for each thread to execute on the thread safety test
static void *thread_safety_function(void *arg)
{
	struct result *res = (struct result *)arg;

	errno = 0; // reset the errno before calling localtime_r
	// Call localtime_r and store the result
	res->rawtime += (time_t)(res->index * SECONDS_IN_A_DAY);
	reentrant_call(NUM_REENTRANTS, &res->rawtime, (void *)&res->timeinfo);

	res->thread_id = pthread_self();
	res->status = 0;
	return NULL;
}

// Function to test localtime_r in a multi threaded context
static void test_localtime_r_thread_safety_and_reentrancy(
    struct result results[])
{
	// Read the current time once before creating the threads
	time_t basetime = time(NULL);

	// create threads
	for (int i = 0; i < NUM_THREADS; ++i) {
		results[i].index = i;
		results[i].status = -1;
		results[i].rawtime = basetime;
	}

	create_and_join_threads(NUM_THREADS, NULL, thread_safety_function, results,
	                        sizeof(struct result));

	struct tm expected_timeinfo;
	struct tm *result = NULL;

	// Validate the results after all threads have finished
	for (int i = 0; i < NUM_THREADS; ++i) {
		TEST(results[i].status == 0,
		     "Thread safety function failed to execute correctly for "
		     "Thread_id: %d\n",
		     results[i].thread_id);

		if (results[i].status == 0) {
			// Verify that the struct tm fields are within expected ranges
			validate_tm_fields(&results[i].timeinfo);

			errno = 0; // reset the errno before calling localtime_r
			result = localtime_r(&results[i].rawtime, &expected_timeinfo);
			TEST(result != NULL && errno == 0, "%s\n", strerror(errno));
			// Verify that the actual values match the expected values
			compare_tm_fields(&results[i].timeinfo, &expected_timeinfo);
		}
	}
	return;
}

static void test_localtime_r(void)
{
	struct tm timeinfo;

	// Test case 1: Valid time_t inputs
	// Get the current time as time_t
	time_t rawtime = time(NULL);

	// Convert time_t to local time representation using localtime_r
	struct tm *result = localtime_r(&rawtime, &timeinfo);

	// Verify that the result is not NULL
	TEST(result != NULL && errno == 0, "%s\n", strerror(errno));

	// Verify that the struct tm fields are within expected ranges
	validate_tm_fields(&timeinfo);

#if 0
	// FIXME: Test cases 2 to 4 are commented out because the localtime_r function
	// currently segfaults when attempting to write to a NULL pointer.
	// This is an area that could be reworked in the future to handle NULL
	// parameters gracefully.

	// Test case 2: Invalid time_t input (NULL pointer)
	result = localtime_r(NULL, &timeinfo);
	// Verify the result is NULL and errno is set
	TEST(result == NULL && errno == 0, "%s\n", strerror(errno));

	// Test case 3: Invalid struct tm pointer (NULL pointer)
	result = localtime_r(&rawtime, NULL);
	// Verify that the result is NULL and errno is set
	TEST(result == NULL && errno == 0, "%s\n", strerror(errno));

	// Test case 4: Invalid time_t and Invalid struct tm pointer (NULL pointers)
	result = localtime_r(NULL, NULL);
	// Verify that the result is NULL and errno is set
	TEST(result == NULL && errno == 0, "%s\n", strerror(errno));
#endif

	// Test case 5: Edge case (time_t value of 0, the Unix epoch)
	rawtime = 0;
	result = localtime_r(&rawtime, &timeinfo);
	// Verify that the result is not NULL
	TEST(result != NULL && errno == 0, "%s\n", strerror(errno));

	// Verify that the result is not NULL and fields are within expected ranges
	TEST(timeinfo.tm_year == 70 && errno == 0,
	     "Years since 1900, should be 70 (1970). Error: %s\n",
	     strerror(errno)); // 1970
	TEST(timeinfo.tm_mon == 0 && errno == 0,
	     "Months should be January. Error: %s\n",
	     strerror(errno)); // January
	TEST(timeinfo.tm_mday == 1 && errno == 0,
	     "Month day should be 1st. Error: %s\n", strerror(errno)); // 1st
	validate_tm_fields(&timeinfo);

	// Test case 6: time_t max 32-bit value (far future date)
	rawtime = INT_MAX;
	result = localtime_r(&rawtime, &timeinfo);
	// Verify that the result is not NULL
	TEST(result != NULL && errno == 0, "%s\n", strerror(errno));

	// Test case 7: time_t overflow check max
	rawtime = LONG_MAX;
	result = localtime_r(&rawtime, &timeinfo);
	// Verify that the result is NULL and errno EOVERFLOW is returned
	TEST(result == NULL && errno == EOVERFLOW, "%s\n", strerror(errno));

	// Test case 8: time_t overflow check min
	rawtime = LONG_MIN;
	result = localtime_r(&rawtime, &timeinfo);
	// Verify that the result is NULL and errno EOVERFLOW is returned
	TEST(result == NULL && errno == EOVERFLOW, "%s\n", strerror(errno));
	return;
}

int main(void)
{
	struct result results[NUM_THREADS];
	test_localtime_r();
	test_localtime_r_thread_safety_and_reentrancy(results);

	return t_status;
}

