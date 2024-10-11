/*
 * pthread_attr_setscope unit test
 */

#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>

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

	TEST(retval == 0, "pthread_attr_setscope failed. Returned %d\n", retval);

	if (retval != 0) {
		pthread_attr_destroy(&attr);
		return;
	}

	int contentionscope = -1;
	retval = pthread_attr_getscope(&attr, &contentionscope);
	if (retval != 0) {
		t_error("pthread_attr_getscope failed. Returned %d\n", retval);
		pthread_attr_destroy(&attr);
		return;
	}

	TEST(contentionscope == scope,
	     "pthread_attr_setscope failed. Expected contentionscope was %d, got "
	     "%d\n",
	     scope, contentionscope);

	pthread_attr_destroy(&attr);
}

static void test_unsupported_scope(int scope)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
		return;
	}

	retval = pthread_attr_setscope(&attr, scope);

	TEST(retval == ENOTSUP,
	     "Unsupported contentionscope test failed. Expected %s, got %s\n",
	     strerror(ENOTSUP), strerror(retval));

	pthread_attr_destroy(&attr);
}

int main(void)
{
	test_valid_scope(PTHREAD_SCOPE_SYSTEM);
	test_unsupported_scope(PTHREAD_SCOPE_PROCESS);
	return t_status;
}
