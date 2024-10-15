/*
 * pthread_attr_getschedparam unit test
 */
#include "test.h"
#include <pthread.h>
#include <sched.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static void test_getschedparam(const int policy)
{
	const int priority = sched_get_priority_max(policy);
	if (priority < 0) {
		t_error("Invalid priority for policy %d\n", policy);
		return;
	}

	pthread_attr_t test_attributes;
	int status = pthread_attr_init(&test_attributes);
	if (status != 0) {
		t_error("Error initialising pthread attributes\n");
		return;
	}

	const struct sched_param test_param = {.sched_priority = priority};
	status = pthread_attr_setschedparam(&test_attributes, &test_param);
	if (status != 0) {
		t_error("pthread_attr_setschedparam failed, returned %d\n", status);
		pthread_attr_destroy(&test_attributes);
		return;
	}

	struct sched_param get_params;
	status = pthread_attr_getschedparam(&test_attributes, &get_params);

	TEST(status == 0,
	     "Expected pthread_attr_getschedparam with policy %d to return 0, got "
	     "%d\n",
	     policy, status);

	TEST(get_params.sched_priority == test_param.sched_priority,
	     "Expected scheduler parameter with policy %d to be %d, instead got "
	     "%d\n",
	     policy, test_param.sched_priority, get_params.sched_priority);

	pthread_attr_destroy(&test_attributes);
}

int main(void)
{
	test_getschedparam(SCHED_FIFO);
	test_getschedparam(SCHED_RR);
	return t_status;
}
