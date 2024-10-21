/*
 * pthread_mutexattr_init unit test
 */

#include "test.h"
#include <errno.h>
#include <pthread.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

int main(void)
{
	pthread_mutexattr_t attr;

	int retval = pthread_mutexattr_init(&attr);

	if (retval != 0 && retval != ENOMEM) {
		t_error("pthread_mutexattr_init failed with unexpected return value. "
		        "Returned %d\n",
		        retval);
		return t_status;
	}

	retval = pthread_mutexattr_destroy(&attr);
	if (retval != 0) {
		t_error("pthread_mutexattr_destroy() failed. Returned %d\n", retval);
		return t_status;
	}

	// a destroyed attributes object can be reinitialized
	retval = pthread_mutexattr_init(&attr);
	TEST(retval == 0 || retval == ENOMEM,
	     "pthread_mutexattr_init failed when initializing a destroyed "
	     "attribute object. Returned "
	     "%d\n",
	     retval);

	pthread_mutexattr_destroy(&attr);

	return t_status;
}
