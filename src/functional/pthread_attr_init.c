/*
 * Test that pthread_attr_init()
 * Upon successful completion, pthread_attr_init() shall return a value of 0.
 *
 * ENOMEM is the only error it returns, so if it doesn't return that error,
 * the return number should be 0.
 *
 * Note: Testing for ENOMEM is not feasible in a controlled manner, as it
 * depends on the system's memory state at the time of the call. This test
 * assumes that sufficient memory is available in the environment where it is
 * run.
 */

#include "test.h"
#include <limits.h>
#include <pthread.h>

#define DEFAULT_STACK_SIZE (size_t)(81920)
#define DEFAULT_GUARD_SIZE (size_t)(8192)

// if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

// Test case: Basic Initialization
static void test_valid_initialization(void)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("test_valid_initialization: "
		        "pthread_attr_init() did not work correctly with RETVAL: %d\n",
		        retval);
		return;
	}
	int detach_state = 0;
	retval = pthread_attr_getdetachstate(&attr, &detach_state);
	TEST(retval == 0 && detach_state == PTHREAD_CREATE_JOINABLE,
	     "test_valid_initialization: "
	     "retrieved detach_state (%d) did not match the expected detach_state "
	     "(%d), RETVAL: %d\n",
	     detach_state, PTHREAD_CREATE_JOINABLE, retval);

	int sched_policy = 0;
	retval = pthread_attr_getschedpolicy(&attr, &sched_policy);
	TEST(retval == 0 && sched_policy == SCHED_OTHER,
	     "test_valid_initialization: "
	     "retrieved sched_policy (%d) did not match the expected detach_state "
	     "(%d), RETVAL: %d\n",
	     sched_policy, SCHED_OTHER, retval);

	struct sched_param schedparam;
	retval = pthread_attr_getschedparam(&attr, &schedparam);
	TEST(retval == 0 && schedparam.sched_priority == 0,
	     "test_valid_initialization: "
	     "retrieved schedparam.sched_priority (%d) did not match the expected "
	     "schedparam.sched_priority (%d), RETVAL: %d\n",
	     schedparam.sched_priority, 0, retval);

	int inherit_sched = 0;
	retval = pthread_attr_getinheritsched(&attr, &inherit_sched);
	TEST(retval == 0 && inherit_sched == PTHREAD_INHERIT_SCHED,
	     "test_valid_initialization: "
	     "retrieved inherit_sched (%d) did not match the expected "
	     "inherit_sched (%d), RETVAL: %d\n",
	     inherit_sched, PTHREAD_INHERIT_SCHED, retval);

	int scope = 0;
	retval = pthread_attr_getscope(&attr, &scope);
	TEST(retval == 0 && scope == PTHREAD_SCOPE_SYSTEM,
	     "test_valid_initialization: "
	     "retrieved inherit_scope (%d) did not match the expected "
	     "inherit_scope (%d), RETVAL: %d\n",
	     scope, PTHREAD_SCOPE_SYSTEM, retval);

	size_t stack_size = 0;
	retval = pthread_attr_getstacksize(&attr, &stack_size);
	TEST(retval == 0 && stack_size >= DEFAULT_STACK_SIZE,
	     "test_valid_initialization: "
	     "retrieved stack_size (%d) did not match the expected stack_size >= "
	     "(%d), RETVAL: %d\n",
	     stack_size, PTHREAD_STACK_MIN, retval);

	size_t guard_size = 0;
	retval = pthread_attr_getguardsize(&attr, &guard_size);
	TEST(retval == 0 && guard_size == DEFAULT_GUARD_SIZE,
	     "test_valid_initialization: "
	     "retrieved guard_size (%d) did not match the expected guard_size >= "
	     "(%d), RETVAL: %d\n",
	     guard_size, 0, retval);

	pthread_attr_destroy(&attr);
	return;
}

int main(void)
{
	test_valid_initialization();

	return t_status;
}
