/*
 * pthread_attr_setschedparam unit test
 *
 * Note: The return of ENOTSUP when an attempt is made to set the attribute to
 * an unsupported value has not been tested as the "unsupported" values are
 * unclear.
 *
 */

#include "test.h"
#include <pthread.h>
#include <sched.h>

#define INVALID_PRIO (-1)
#define SCHED_MAX (SCHED_RR + 1)

#define TEST(c, ...) ((c) || (t_error("TEST(" #c ") failed " __VA_ARGS__), 0))

static void test_valid_schedparam(pthread_attr_t *pattr,
                                  const struct sched_param *schedparam)
{
	int retval = pthread_attr_setschedparam(pattr, schedparam);
	if (!retval) {
		struct sched_param schedparam_value;
		schedparam_value.sched_priority = INVALID_PRIO;
		retval = pthread_attr_getschedparam(pattr, &schedparam_value);
		if (!retval) {
			TEST(schedparam->sched_priority == schedparam_value.sched_priority,
			     "[Expected %d, got %d]\n", schedparam->sched_priority,
			     schedparam_value.sched_priority);
		} else {
			t_error("pthread_attr_getschedparam() failed. Returned %d\n",
			        retval);
		}
	} else {
		t_error("pthread_attr_setschedparam() failed. Returned %d\n", retval);
	}
	return;
}

int main(void)
{
	pthread_attr_t attr;

	int retval = pthread_attr_init(&attr);
	if (!retval) {

		for (int policy = 0; policy < SCHED_MAX; ++policy) {

			struct sched_param param;

			param.sched_priority = sched_get_priority_min(policy);
			test_valid_schedparam(&attr, &param);

			param.sched_priority = sched_get_priority_max(policy);
			test_valid_schedparam(&attr, &param);
		}

		pthread_attr_destroy(&attr);
	} else {
		t_error("pthread_attr_init() failed. Returned %d\n", retval);
	}
	return t_status;
}
