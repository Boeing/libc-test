/*
 * pthread_attr_getinherit unit test
 */

#include "test.h"
#include <pthread.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void test_valid_inheritsched(const int inheritsched)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
		return;
	}
	retval = pthread_attr_setinheritsched(&attr, inheritsched);
	if (retval != 0) {
		t_error("pthread_attr_setinheritsched() failed. Returned %d\n", retval);
		pthread_attr_destroy(&attr);
		return;
	}

	int inheritsched_value = -1;

	retval = pthread_attr_getinheritsched(&attr, &inheritsched_value);

	TEST(retval == 0, "Failed to get inheritsched. Expected %d, got %d\n",
	     inheritsched, retval);

	TEST(inheritsched_value == inheritsched,
	     "Failed to get correct inheritsched value. Expected %d, got %d\n",
	     inheritsched, inheritsched_value);

	pthread_attr_destroy(&attr);
}

int main(void)
{
	test_valid_inheritsched(PTHREAD_EXPLICIT_SCHED);
	test_valid_inheritsched(PTHREAD_INHERIT_SCHED);
	return t_status;
}
