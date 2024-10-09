/*
 * pthread_attr_destroy unit test
 */
#include "test.h"
#include <pthread.h>
#include <string.h>

// if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void test_destroy_initialized_attr(void)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("test_destroy_initialized_attr: "
		        "pthread_attr_init() did not work correctly with Error: %s\n",
		        strerror(retval));
		return;
	}

	retval = pthread_attr_destroy(&attr);
	TEST(retval == 0,
	     "test_destroy_initialized_attr: "
	     "pthread_attr_destroy() did not work correctly with Error: %s\n",
	     strerror(retval));
	return;
}

int main(void)
{
	test_destroy_initialized_attr();
	return t_status;
}
