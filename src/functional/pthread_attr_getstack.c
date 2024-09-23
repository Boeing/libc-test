/*
 * pthread_attr_getstack unit test
 */
#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE (size_t)(1024 * 1024); // 1MB

//if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

// Test case: Valid Input - Set and Retrieve Stack Attributes
static void test_valid_input_set_and_retrieve_stack_attributes(void)
{
	size_t stack_size = STACK_SIZE; // 1MB
	void *stack_addr = calloc(stack_size, sizeof(char));
	if (stack_addr == NULL) {
		t_error("test_valid_input_set_and_retrieve_stack_attributes: calloc() "
		        "did not work correctly\n");
		return;
	}

	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("test_null_stack_attributes: "
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

	pthread_attr_destroy(&attr); //Invalidate the attribute object
	free(stack_addr);
	return;
}

// Test case: Valid Attribute Object - Retrieve Stack attributes
static void test_valid_attr_obj_and_retrieve_stack_attributes(void)
{
	pthread_attr_t attr;
	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		t_error("test_valid_attr_obj_and_retrieve_stack_attributes: "
		        "pthread_attr_init() did not work correctly [errno: %d]\n",
		        retval);
		return;
	}

	void *retrieved_stack_addr = NULL;
	size_t retrieved_stack_size = 0;

	retval = pthread_attr_getstack(&attr, &retrieved_stack_addr,
	                               &retrieved_stack_size);
	TEST(retval == EINVAL,
	     "test_valid_attr_obj_and_retrieve_stack_attributes: "
	     "pthread_attr_getstack() was expected to fail [errno: %d]\n",
	     retval);

	pthread_attr_destroy(&attr);
	return;
}

int main(void)
{
	test_valid_input_set_and_retrieve_stack_attributes();
	test_valid_attr_obj_and_retrieve_stack_attributes();

	return t_status;
}
