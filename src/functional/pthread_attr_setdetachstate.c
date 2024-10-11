/*
 * pthread_attr_setdetachstate unit test
 */

#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>

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

	TEST(retval == 0,
	     "pthread_attr_setdetachstate failed when setting detachstate to %d. "
	     "Returned %d\n",
	     detachstate, retval);

	if (retval != 0) {
		pthread_attr_destroy(&attr);
		return;
	}

	int detachstate_value = INVALID_DETACHSTATE;
	retval = pthread_attr_getdetachstate(&attr, &detachstate_value);
	if (retval != 0) {
		t_error("pthread_attr_getdetachstate failed. Returned %d\n", retval);
		pthread_attr_destroy(&attr);
		return;
	}

	TEST(detachstate_value == detachstate,
	     "Failed to set the correct detachstate. Expected %d, got %d\n",
	     detachstate, detachstate_value);

	pthread_attr_destroy(&attr);
}

static void test_invalid_detachstate(const int detachstate)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
		return;
	}

	retval = pthread_attr_setdetachstate(&attr, detachstate);

	TEST(retval == EINVAL,
	     "Invalid detachstate test failed. Expected %s, got %s\n",
	     strerror(EINVAL), strerror(retval));

	pthread_attr_destroy(&attr);
}

int main(void)
{
	test_valid_detachstate(PTHREAD_CREATE_DETACHED);
	test_valid_detachstate(PTHREAD_CREATE_JOINABLE);
	test_invalid_detachstate(INVALID_DETACHSTATE);
	return t_status;
}
