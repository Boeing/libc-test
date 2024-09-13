/*
 * test_times unit test
 */
#include "test.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TEST_FOR_VALID_TIME(a) ((a) >= 0 ? 1 : 0)
#define TEST_FOR_INCREASE_IN_TIME(a, b) ((a) >= (b) ? 1 : 0)
#define TEST_FOR_CHANGE_IN_TIME(a, b) ((a) > (b) ? 1 : 0)
#define CPU_CYCLES_TICKS 1000000L

static void busy_loop_to_consume_cpu_time(const long int ticks)
{
	for (long int i = 0; i < ticks; ++i) {
		getpid(); // sytem call that forces kernel interaction
	}
}

static void test_time_valid_data(struct tms sample)
{
	TEST(TEST_FOR_VALID_TIME(sample.tms_utime),
	     "time_sample.tms_utime is negative\n");
	TEST(TEST_FOR_VALID_TIME(sample.tms_stime),
	     "time_sample.tms_stime is negative\n");
	TEST(TEST_FOR_VALID_TIME(sample.tms_cutime),
	     "time_sample.tms_cutime is negative\n");
	TEST(TEST_FOR_VALID_TIME(sample.tms_cstime),
	     "time_sample.tms_cstime is negative\n");
	return;
}

static void test_time_increase(struct tms time1, struct tms time2)
{
	TEST(TEST_FOR_INCREASE_IN_TIME(time1.tms_utime, time2.tms_utime),
	     "time_sample2.tms_stime did not increase as expected\n");
	TEST(TEST_FOR_INCREASE_IN_TIME(time1.tms_stime, time2.tms_stime),
	     "time_sample2.tms_stime did not increase as expected\n");

	TEST(TEST_FOR_INCREASE_IN_TIME(time1.tms_cutime, time2.tms_cutime),
	     "time_sample2.tms_stime did not increase as expected\n");
	TEST(TEST_FOR_INCREASE_IN_TIME(time1.tms_cstime, time2.tms_cstime),
	     "time_sample2.tms_stime did not increase as expected\n");
	return;
}

static void test_parent_time_increase(struct tms time1, struct tms time2)
{
	test_time_valid_data(time1);

	test_time_valid_data(time2);

	TEST(TEST_FOR_CHANGE_IN_TIME(time1.tms_utime, time2.tms_utime),
	     "time_sample2.tms_stime did not increase as expected\n");
	TEST(TEST_FOR_CHANGE_IN_TIME(time1.tms_stime, time2.tms_stime),
	     "time_sample2.tms_stime did not increase as expected\n");
	return;
}

static void test_child_time_increase(struct tms time1, struct tms time2)
{
	test_time_valid_data(time1);

	test_time_valid_data(time2);

	TEST(TEST_FOR_CHANGE_IN_TIME(time1.tms_cutime, time2.tms_cutime),
	     "time_sample2.tms_stime did not increase as expected\n");
	TEST(TEST_FOR_CHANGE_IN_TIME(time1.tms_cstime, time2.tms_cstime),
	     "time_sample2.tms_stime did not increase as expected\n");
	return;
}

static void test_times(void)
{
	struct tms time_sample;

	// Test case 1: Basic functionality
	clock_t result = times(&time_sample);
	TEST(result != (clock_t)-1 && errno == 0, "%s\n",
	     strerror(errno)); // Should not return an error

	test_time_valid_data(time_sample);

	// Test case 2: Handling NULL pointer
	// FIXME: the times function currently does not return an error no when
	// attempting to write to a NULL pointer. This is an area that could be
	// reworked in the future to handle NULL parameters with error no.
	result = times(NULL);
	TEST(result != (time_t)-1 && errno == 0, "%s\n",
	     strerror(errno)); // Should not return an error, even if the argument
	                       // is NULL

	// Test case 3: Multiple invocations consistency
	// Capture initial times
	result = times(&time_sample);
	TEST(result != (time_t)-1 && errno == 0, "%s\n",
	     strerror(errno)); // Should not return an error

	// Introduce CPU load
	busy_loop_to_consume_cpu_time(
	    CPU_CYCLES_TICKS); // Simple busy loop to consume some CPU time

	struct tms time_sample2;
	clock_t result2 = times(&time_sample2);
	TEST(result2 != (time_t)-1 && errno == 0, "%s\n", strerror(errno));

	TEST(result2 > result && errno == 0, "times did not progress. Error: %s\n",
	     strerror(errno));

	// Validate that times are recorded
	test_time_increase(time_sample2, time_sample);

	// Validate that the parent process times are recorded
	test_parent_time_increase(time_sample2, time_sample);

	// Test case 4: Handling child processes
	struct tms parent_time_before;

	clock_t result_before = times(&parent_time_before);
	TEST(result_before != (time_t)-1 && errno == 0, "%s\n",
	     strerror(errno)); // Should not return an error

	// Create a child process
	pid_t pid = fork();
	if (pid == -1) {
		t_error("Fork failed\n");
	}
	if (pid == 0) {
		// Child process: simulate some work
		busy_loop_to_consume_cpu_time(
		    CPU_CYCLES_TICKS); // Simple busy loop to consume some CPU time
		exit(0);
	} else {
		// Parent process: wait for the child process to terminate
		wait(NULL);

		struct tms parent_time_after;
		// Capture times after child process has terminated
		clock_t result_after = times(&parent_time_after);
		TEST(result_after != (time_t)-1 && errno == 0, "%s\n",
		     strerror(errno)); // Should not return an error

		TEST(result_after > result_before && errno == 0,
		     "times did not progress. Error: %s\n", strerror(errno));

		// Validate that times are recorded
		test_time_increase(parent_time_after, parent_time_before);

		// Validate that the child process times are recorded
		test_child_time_increase(parent_time_after, parent_time_before);
	}
	return;
}

int main(void)
{
	test_times();
	return t_status;
}

