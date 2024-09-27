/*
 * pthread_attr_getstacksize test
 */

#include "test.h" // Common test header (needed for all unit tests written)
#include "utils.h"
#include <limits.h>
#include <pthread.h>

// if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

#define DEFAULT_STACK_SIZE \
	(PTHREAD_STACK_MIN * \
	 10) // valid condition: size-PTHREAD_STACK_MIN > SIZE_MAX/4 - as per
	     // pthread_attr_setstacksize.c

void check_stacksize(void)
{
	static pthread_attr_t stacksize;
	size_t test_size_value = 0;

	// Note: Passing set/ get function an uninitialized attribute should return
	// undefined behaviour
	int attrresult = pthread_attr_init(&stacksize);

	// Test case 1: Set and get an acceptable stack size
	int setresult =
	    pthread_attr_setstacksize(&stacksize, (size_t)DEFAULT_STACK_SIZE);
	if (setresult != 0 || attrresult != 0) {
		t_error("the attrresult, %i, and setresult, %i , did not succeed "
		        "as expected\n",
		        attrresult, setresult);
	}

	// get stack size which will return 0 if successful (errnos are handled in
	// the set function only)
	int getresult = pthread_attr_getstacksize(&stacksize, &test_size_value);
	TEST(getresult == 0 && test_size_value == (size_t)DEFAULT_STACK_SIZE,
	     "pthread_attr_getstacksize failed. Returned %ld but expected %ld with "
	     "status %i\n",
	     test_size_value, DEFAULT_STACK_SIZE, getresult);

	// cleanup
	int cleanupresult = pthread_attr_destroy(&stacksize);
	if (cleanupresult != 0) {
		t_error("the cleanupresult, %i, did not succeed as expected\n",
		        cleanupresult);
	}
}

int main(void)
{
	check_stacksize();
	return t_status;
}
