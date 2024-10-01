/*
 * pthread_attr_setstacksize test
 */

#include "test.h" // Common test header (needed for all unit tests written)
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

// if c evaluates to 0, execute t_error with the specified error message
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

#define DEFAULT_STACK_SIZE \
	(131072) // arbitrarily same as pthread_attr_setstacksize.c
#define DEFAULT_STACK_MAX \
	(8 << 20) // same as pthread_attr_setstacksize.c
	          // bitwise left shift equivalent to (8*(2.^20))
#define UNDERSIZED_STACK_SIZE (PTHREAD_STACK_MIN - 1)

void check_setstacksize(void)
{
	static pthread_attr_t stacksize;
	size_t test_size_value = 0;
	errno = 0;

	// Note: Passing set/ get function an uninitialized attribute should return
	// undefined behaviour
	int attrresult = pthread_attr_init(&stacksize);
	if (attrresult != 0) {
		t_error("pthread_attr_init failed with status %i, %s\n", attrresult,
		        strerror(errno));
	}
	// Test case 1: Set an acceptable stack size
	int setresult =
	    pthread_attr_setstacksize(&stacksize, (size_t)DEFAULT_STACK_SIZE);
	TEST(setresult == 0, "pthread_attr_setstacksize failed with status %i\n",
	     setresult);
	// errnos are handled in the set function only
	int getresult = pthread_attr_getstacksize(&stacksize, &test_size_value);
	if (getresult != 0 || test_size_value != (size_t)DEFAULT_STACK_SIZE) {
		t_error("pthread_attr_getstacksize failed. Returned status %i with "
		        "test_size_value %ld expected %ld.\n",
		        getresult, test_size_value, DEFAULT_STACK_SIZE);
	}

	// Test case 2: Boundary values for stacksize (should return EINVAL error if
	// stacksize < specified minimum or > system-imposed limit)
	setresult = pthread_attr_setstacksize(&stacksize, PTHREAD_STACK_MIN);
	getresult = pthread_attr_getstacksize(&stacksize, &test_size_value);
	TEST(setresult == 0,
	     "pthread_attr_setstacksize failed to set minimum stack size %i, %s.\n",
	     setresult, strerror(errno));
	if (getresult != 0 || test_size_value != (size_t)PTHREAD_STACK_MIN) {
		t_error("pthread_attr_getstacksize failed. Returned status %i with "
		        "test_size_value %ld.\n",
		        getresult, test_size_value);
	}

	setresult = pthread_attr_setstacksize(&stacksize, DEFAULT_STACK_MAX);
	getresult = pthread_attr_getstacksize(&stacksize, &test_size_value);
	TEST(setresult == 0,
	     "pthread_attr_setstacksize failed to set maximum %ld stack size %i.\n",
	     DEFAULT_STACK_MAX, setresult);
	if (getresult != 0 || test_size_value != (size_t)DEFAULT_STACK_MAX) {
		t_error("pthread_attr_getstacksize failed. Returned status %i with "
		        "test_size_value %ld.\n",
		        getresult, test_size_value);
	}

	// NOTE: There is no logic to reject a valid stack size as long as it is
	// greater than the minimum requirement, and no requirement to impose a
	// maximum other than the agnostic system imposed limit.

	setresult = pthread_attr_setstacksize(&stacksize, UNDERSIZED_STACK_SIZE);
	TEST(setresult == EINVAL,
	     "pthread_attr_setstacksize failed to error EINVAL setting undersized "
	     "stack size, returned %i.\n",
	     setresult);

	// Test case 3: Error handling for null attribute value (undersize extreme)
	setresult = pthread_attr_setstacksize(&stacksize, (intptr_t)NULL);
	TEST(setresult == EINVAL,
	     "pthread_attr_setstacksize failed to error EINVAL setting NULL stack "
	     "size, returned %i.\n",
	     setresult);

	// cleanup
	int cleanupresult = pthread_attr_destroy(&stacksize);
	if (cleanupresult != 0) {
		t_error("the cleanupresult, %i, did not succeed as expected\n",
		        cleanupresult);
	}
}

int main(void)
{
	check_setstacksize();
	return t_status;
}
