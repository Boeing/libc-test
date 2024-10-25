/*
 * sched_rr_get_interval unit test
 */

#include "test.h"
#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define RETURN_FAILURE (-1)
#define RETURN_SUCCESS (0)
#define INVALID_PID (-1)
#define MAX_NSEC (1000000000)
#define ZERO_PID (0)
#define CURRENT_PID (1)
#define MAX_PID (2)

#define TEST(c, ...) ((c) || (t_error("TEST(" #c ") failed " __VA_ARGS__), 0))

static void test_sched_rr_get_interval_success(void)
{
	struct timespec sched_interval = {-1, -1};
	int valid_pid = getpid();

	errno = 0;
	int retval = sched_rr_get_interval(valid_pid, &sched_interval);
	if (!retval) {
		// Check if returned time values are correct.
		TEST(sched_interval.tv_sec >= 0,
		     "Failed to get correct timespec.tv_sec values. expected [tv_sec "
		     ">=0 ], got [%ld]\n",
		     sched_interval.tv_sec);
		TEST(sched_interval.tv_nsec >= 0 && sched_interval.tv_nsec < MAX_NSEC,
		     "Failed to get correct timespec.tv_nsec values. expected [0 =< "
		     "tv_nsec < %d], got [%ld]\n",
		     MAX_NSEC, sched_interval.tv_nsec);
	} else {
		t_error("test_sched_rr_get_interval_success: "
		        "sched_rr_get_interval() did not work correctly for PID: %d "
		        "Error: %s\n",
		        valid_pid, strerror(errno));
	}
	return;
}

static void test_sched_rr_get_interval_failure(void)
{
	// Create a child process which exit immediately
	pid_t child_pid = fork();
	if (child_pid == -1) {
		t_error("test_sched_rr_get_interval_failure fork failed\n");
	} else {
		if (child_pid == 0) {
			exit(0);
		} else {
			int status = 0;
			// Wait for the child process to exit
			waitpid(child_pid, &status, 0);

			// Assume the pid is not yet given to another process
			struct timespec sched_interval;
			errno = 0;
			int retval = sched_rr_get_interval(child_pid, &sched_interval);
			TEST(retval == RETURN_FAILURE && errno == ESRCH,
			     "sched_rr_get_interval() did not fail with invalid PID: %d. "
			     "expected "
			     "return: -1, got: %d. expected error: ESRCH, got: %d\n",
			     child_pid, retval, errno);
		}
	}
	return;
}

static void test_sched_rr_get_interval_pid_zero(void)
{
	struct timespec sched_interval[MAX_PID] = {{-1, -1}, {-1, -1}};
	int pid_zero = 0;
	errno = 0;

	int retval = sched_rr_get_interval(pid_zero, &sched_interval[ZERO_PID]);
	if (!retval) {
		int pid_current = getpid();
		retval =
		    sched_rr_get_interval(pid_current, &sched_interval[CURRENT_PID]);
		if (!retval) {
			TEST(sched_interval[ZERO_PID].tv_sec ==
			             sched_interval[CURRENT_PID].tv_sec &&
			         sched_interval[ZERO_PID].tv_nsec ==
			             sched_interval[CURRENT_PID].tv_nsec,
			     "Expected pid_zero interval tv_sec:%d, tv_nsec:%d and "
			     "pid_current interval tv_sec:%d, tv_nsec:%d to match\n",
			     sched_interval[ZERO_PID].tv_sec,
			     sched_interval[ZERO_PID].tv_nsec,
			     sched_interval[CURRENT_PID].tv_sec,
			     sched_interval[CURRENT_PID].tv_nsec);
		} else {
			t_error("test_sched_rr_get_interval_pid_zero sched_rr_get_interval "
			        "failed for PID: %d and returned: %d with error: %s\n",
			        pid_current, retval, strerror(errno));
		}
	} else {
		t_error("test_sched_rr_get_interval_pid_zero sched_rr_get_interval "
		        "failed for PID: %d and returned: %d with error: %s\n",
		        pid_zero, retval, strerror(errno));
	}
	return;
}

int main(void)
{
	test_sched_rr_get_interval_success();
	test_sched_rr_get_interval_failure();
	test_sched_rr_get_interval_pid_zero();
	return t_status;
}
