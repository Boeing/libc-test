/*
 * pthread_attr_setinherit unit test
 *
 * Note: The return of ENOTSUP when an attempt is made to set the attribute to
 * an unsupported value has not been tested as the "unsupported" values are
 * unclear.
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

	TEST(retval == 0, "Failed to set inheritsched to %d. Returned %d\n",
	     inheritsched, retval);

	int inheritsched_value = -1;

	retval = pthread_attr_getinheritsched(&attr, &inheritsched_value);

	if (retval != 0) {
		t_error("pthread_attr_getinheritsched() failed. Returned %d\n", retval);
		pthread_attr_destroy(&attr);
		return;
	}

	TEST(inheritsched_value == inheritsched,
	     "Failed to set correct inheritsched value. Expected %d, got %d\n",
	     inheritsched, inheritsched_value);

	pthread_attr_destroy(&attr);
}

int main(void)
{
	test_valid_inheritsched(PTHREAD_EXPLICIT_SCHED);
	test_valid_inheritsched(PTHREAD_INHERIT_SCHED);
	return t_status;
}
