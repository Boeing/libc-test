/*
 * pthread_attr_getscope unit test
 *
 * Note: Musl does not support a contention scope with value
 * PTHREAD_SCOPE_PROCESS
 */

#include "test.h"
#include <pthread.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void test_valid_scope(int scope)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
		return;
	}
	retval = pthread_attr_setscope(&attr, scope);
	if (retval != 0) {
		t_error("pthread_attr_setscope failed. Returned %d\n", retval);
		pthread_attr_destroy(&attr);
		return;
	}
	int contentionscope = -1;
	retval = pthread_attr_getscope(&attr, &contentionscope);

	TEST(retval == 0 && contentionscope == scope,
	     "pthread_attr_getscope failed with return value %d. Expected "
	     "contentionscope was %d, got %d\n",
	     scope, contentionscope);

	pthread_attr_destroy(&attr);
}

int main(void)
{
	test_valid_scope(PTHREAD_SCOPE_SYSTEM);
	return t_status;
}
