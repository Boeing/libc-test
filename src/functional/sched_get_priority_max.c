/*
 * sched_get_priority_max unit test
 */
#include "test.h"
#include <errno.h>
#include <sched.h>
#include <string.h>

#define RETURN_FAILURE (-1)
#define RETURN_SUCCESS (0)
#define INVALID_SCHED_POLICY (-1)

#define TEST(c, ...) ((c) || (t_error("TEST(" #c ") failed " __VA_ARGS__), 0))

static void test_valid_scheduling_policy(const int sched_policy)
{
	int max_priority = sched_get_priority_max(sched_policy);
	TEST(max_priority != RETURN_FAILURE && errno == RETURN_SUCCESS,
	     "[Returned %d with error %s]\n", max_priority, strerror(errno));
	return;
}

static void test_invalid_scheduling_policy(const int sched_policy)
{
	int max_priority = sched_get_priority_max(sched_policy);
	TEST(max_priority == RETURN_FAILURE && errno == EINVAL,
	     "[did not return %d with error %s]\n", max_priority, strerror(errno));
	return;
}

int main(void)
{
	test_valid_scheduling_policy(SCHED_FIFO);
	test_valid_scheduling_policy(SCHED_RR);
	test_valid_scheduling_policy(SCHED_OTHER);

	test_invalid_scheduling_policy(INVALID_SCHED_POLICY);

	return t_status;
}
