/*
 * pthread_attr_setstack unit test
 */
#include "test.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define INVALID_STACK_SIZE (size_t)(0)
#define DEFAULT_STACK_SIZE (size_t)(PTHREAD_STACK_MIN)
#define MINIMUM_STACK_SIZE (size_t)(PTHREAD_STACK_MIN)
#define MAXIMUM_STACK_SIZE (size_t)(SIZE_MAX / 4)
#define UNDERSIZED_STACK_SIZE (size_t)(PTHREAD_STACK_MIN - 1)
#define OVERSIZED_STACK_SIZE (size_t)((SIZE_MAX / 4) + PTHREAD_STACK_MIN + 1)

// if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

struct test_cases {
	size_t stack_size;
	int exp_retval;
};

// Test case: Valid Input - Set and Retrieve Stack Attributes
static void test_valid_input_set_and_retrieve_stack_attributes(void)
{
	size_t stack_size = DEFAULT_STACK_SIZE;
	void *stack_addr = calloc(stack_size, sizeof(char));
	if (stack_addr == NULL) {
		t_error("test_valid_input_set_and_retrieve_stack_attributes: calloc() "
		        "did not work correctly, Error: %s\n",
		        strerror(errno));
		return;
	}

	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("test_valid_input_set_and_retrieve_stack_attributes: "
		        "pthread_attr_init() did not work correctly [errno: %d]\n",
		        retval);
		free(stack_addr);
		return;
	}

	retval = pthread_attr_setstack(&attr, stack_addr, stack_size);
	if (retval != 0) {
		t_error("test_valid_input_set_and_retrieve_stack_attributes: "
		        "pthread_attr_setstack() did not work correctly [errno: %d]\n",
		        retval);
		pthread_attr_destroy(&attr);
		free(stack_addr);
		return;
	}

	void *retrieved_stack_addr = NULL;
	size_t retrieved_stack_size = 0;

	retval = pthread_attr_getstack(&attr, &retrieved_stack_addr,
	                               &retrieved_stack_size);
	TEST(retval == 0,
	     "test_valid_input_set_and_retrieve_stack_attributes: "
	     "pthread_attr_getstack() failed with error: %s\n",
	     strerror(retval));
	TEST(retrieved_stack_addr == stack_addr,
	     "test_valid_input_set_and_retrieve_stack_attributes: "
	     "retrieved_stack_addr (%d) did not match the stack_addr (%d)\n",
	     retrieved_stack_addr, stack_addr);
	TEST(retrieved_stack_size == stack_size,
	     "test_valid_input_set_and_retrieve_stack_attributes: "
	     "retrieved_stack_size (%d) did not match the stack_size (%d)\n",
	     retrieved_stack_size, stack_size);

	pthread_attr_destroy(&attr);
	free(stack_addr);
	return;
}

static void test_stack_size(struct test_cases test)
{
	size_t stack_size = DEFAULT_STACK_SIZE;
	void *stack_addr = calloc(stack_size, sizeof(char));
	if (stack_addr == NULL) {
		t_error("test_invalid_stack_size_zero: calloc() "
		        "did not work correctly, Error: %s\n",
		        strerror(errno));
		return;
	}

	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("test_invalid_stack_size_zero: "
		        "pthread_attr_init() did not work correctly with Error: %s\n",
		        strerror(retval));
		free(stack_addr);
		return;
	}

	retval = pthread_attr_setstack(&attr, stack_addr, test.stack_size);
	TEST(retval == test.exp_retval,
	     "test_invalid_stack_size_zero: "
	     "pthread_attr_setstack() did not return %d as expected\n",
	     test.exp_retval);

	pthread_attr_destroy(&attr);
	free(stack_addr);
	return;
}

int main(void)
{
	test_valid_input_set_and_retrieve_stack_attributes();

	struct test_cases tests[] = {
	    {DEFAULT_STACK_SIZE, 0},         {PTHREAD_STACK_MIN, 0},
	    {MAXIMUM_STACK_SIZE, 0},         {OVERSIZED_STACK_SIZE, EINVAL},
	    {UNDERSIZED_STACK_SIZE, EINVAL}, {INVALID_STACK_SIZE, EINVAL},
	};

	const size_t num_tests = sizeof(tests) / sizeof(*tests);

	for (int test = 0; test < num_tests; ++test) {
		test_stack_size(tests[test]);
	}

	return t_status;
}
