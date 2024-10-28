/*
 * sched_yield unit test
 *
 */

#include "test.h"
#include <errno.h>
#include <sched.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error("TEST(" #c ") failed " __VA_ARGS__), 0))

static void test_sched_yield_return(void)
{
	int retval = sched_yield();

	TEST(retval == 0,
	     "test_sched_yield_return: sched_yield() failed with error: %s\n",
	     strerror(errno));

	return;
}

int main(void)
{
	test_sched_yield_return();
	return t_status;
}
