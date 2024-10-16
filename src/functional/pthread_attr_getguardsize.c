/*
 * pthread_attr_getguardsize unit test
 */

#include "test.h"
#include <pthread.h>

#define INVALID_GUARD_SIZE (-1)
#define ZERO_GUARD_SIZE (size_t)(0)
#define DEFAULT_GUARD_SIZE (size_t)(8192)

#define TEST(c, ...) ((c) || (t_error("TEST(" #c ") failed " __VA_ARGS__), 0))

static void test_valid_guardsize(pthread_attr_t *pattr, const size_t guardsize)
{
	int retval = pthread_attr_setguardsize(pattr, guardsize);
	if (!retval) {
		size_t guardsize_value = INVALID_GUARD_SIZE;
		retval = pthread_attr_getguardsize(pattr, &guardsize_value);
		if (!retval) {
			TEST(guardsize_value == guardsize, "[Expected %d, got %d]\n",
			     guardsize, guardsize_value);
		} else {
			t_error("pthread_attr_getguardsize() failed. Returned %d\n",
			        retval);
		}
	} else {
		t_error("pthread_attr_getguardsize() failed. Returned %d\n", retval);
	}
	return;
}

int main(void)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (!retval) {
		test_valid_guardsize(&attr, ZERO_GUARD_SIZE);
		test_valid_guardsize(&attr, DEFAULT_GUARD_SIZE);
		pthread_attr_destroy(&attr);
	} else {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
	}
	return t_status;
}
