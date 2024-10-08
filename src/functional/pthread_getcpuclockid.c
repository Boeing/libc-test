/*
 * pthread_getcpuclockid test
 */

#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void *thread_function(void *arg)
{
	while (1) {
		pthread_testcancel();
	}

	return 0;
}

int main(void)
{
	pthread_t test_thread = 0;
	clockid_t clockid = 0;

	int retval = pthread_create(&test_thread, 0, thread_function, 0);
	if (retval != 0) {
		t_error("pthread_create() failed. Returned %d\n", retval);
		return t_status;
	}

	retval = pthread_getcpuclockid(test_thread, &clockid);
	TEST(retval == 0, "pthread_getcpuclockid() failed. Returned %d\n", retval);

	// test that the clockid returns a valid clock
	struct timespec tp;
	TEST(clock_gettime(clockid, &tp) == 0,
	     "clock_gettime() failed with errno: %s\n", strerror(errno));

	pthread_cancel(test_thread);
	pthread_join(test_thread, 0);
	return t_status;
}
