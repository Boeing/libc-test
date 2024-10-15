/*
 * pthread_attr_getschedpolicy unit test
 */

#include "test.h"
#include <pthread.h>

#define SCHED_INVALID (-1)
#define TEST(c, ...) ((c) || (t_error("TEST(" #c ") failed " __VA_ARGS__), 0))

static void test_valid_schedpolicy(pthread_attr_t *pattr, const int schedpolicy)
{
	int retval = pthread_attr_setschedpolicy(pattr, schedpolicy);
	if (!retval) {
		int schedpolicy_value = SCHED_INVALID;
		retval = pthread_attr_getschedpolicy(pattr, &schedpolicy_value);
		if (!retval) {
			TEST(schedpolicy_value == schedpolicy, "[Expected %d, got %d]\n",
			     schedpolicy, schedpolicy_value);

		} else {
			t_error("pthread_attr_getschedpolicy() failed. Returned %d\n",
			        retval);
		}
	} else {
		t_error("pthread_attr_setschedpolicy() failed. Returned %d\n", retval);
	}
	return;
}

int main(void)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (!retval) {
		test_valid_schedpolicy(&attr, SCHED_RR);
		test_valid_schedpolicy(&attr, SCHED_FIFO);
		test_valid_schedpolicy(&attr, SCHED_OTHER);
		pthread_attr_destroy(&attr);
	} else {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
	}
	return t_status;
}
