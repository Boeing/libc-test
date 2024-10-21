/*
 * pthread_mutexattr_destroy unit test
 */

#include "test.h"
#include <pthread.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

int main(void)
{
	pthread_mutexattr_t attr;

	int retval = pthread_mutexattr_init(&attr);
	if (retval != 0) {
		t_error("pthread_mutexattr_init() failed. Returned %d\n", retval);
		return t_status;
	}

	retval = pthread_mutexattr_destroy(&attr);
	TEST(retval == 0, "pthread_mutexattr_destroy() failed. Returned %d\n",
	     retval);

	return t_status;
}
