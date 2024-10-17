/*
 * pthread_mutexattr_setprotocol unit test
 *
 * Note: PTHREAD_PRIO_PROTECT is not supported in musl
 */

#include "test.h"
#include <pthread.h>

#define INVALID_PROTOCOL (-1)
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void test_valid_protocol(const int protocol)
{
	pthread_mutexattr_t attr;
	int retval = pthread_mutexattr_init(&attr);
	if (retval != 0) {
		t_error("pthread_mutexattr_init() failed. Returned %d\n", retval);
		return;
	}

	retval = pthread_mutexattr_setprotocol(&attr, protocol);
	if (retval != 0) {
		t_error("pthread_mutexattr_setprotocol() failed. Returned %d\n");
		pthread_mutexattr_destroy(&attr);
		return;
	}

	int protocol_value = INVALID_PROTOCOL;

	retval = pthread_mutexattr_getprotocol(&attr, &protocol_value);
	if (retval != 0) {
		t_error("pthread_mutexattr_getprotocol() failed. Returned %d\n",
		        retval);
		pthread_mutexattr_destroy(&attr);
		return;
	}
	TEST(protocol_value == protocol,
	     "Failed to set the correct protocol value. Expected %d, got %d\n",
	     protocol, protocol_value);

	pthread_mutexattr_destroy(&attr);
}

int main(void)
{
	test_valid_protocol(PTHREAD_PRIO_NONE);
	test_valid_protocol(PTHREAD_PRIO_INHERIT);
	return t_status;
}
