/*
 * pthread_attr_getdetachstate unit test
 */

#include "test.h"
#include <pthread.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define INVALID_DETACHSTATE (-1)

static void test_valid_detachstate(const int detachstate)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
		return;
	}

	retval = pthread_attr_setdetachstate(&attr, detachstate);
	if (retval != 0) {
		t_error("pthread_attr_setdetachstate failed when setting detachstate "
		        "to %d. Returned %d\n",
		        detachstate, retval);
		pthread_attr_destroy(&attr);
		return;
	}

	int detachstate_value = INVALID_DETACHSTATE;
	retval = pthread_attr_getdetachstate(&attr, &detachstate_value);

	TEST(retval == 0 && detachstate_value == detachstate,
	     "pthread_attr_getdetachstate failed. Returned %d. Expected "
	     "detachstate = %d, got %d\n",
	     retval, detachstate, detachstate_value);

	pthread_attr_destroy(&attr);
}

int main(void)
{
	test_valid_detachstate(PTHREAD_CREATE_DETACHED);
	test_valid_detachstate(PTHREAD_CREATE_JOINABLE);
	return t_status;
}
