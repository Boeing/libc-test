/*
 * pthread_self unit test
 */
#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TESTR(f, g) \
	do { \
		if ((f) != 0) { \
			t_error(#f " failed: %s in test %s\n", strerror(errno), g); \
			return t_status; \
		} \
	} while (0)

void *save_pthread_id(void *args)
{
	pthread_t *thread_id = (pthread_t *)args;
	*thread_id = pthread_self();
	return 0;
}

int main(void)
{
	pthread_t thread_id = 0;
	pthread_t expected_id = 0;

	TESTR(pthread_create(&thread_id, 0, save_pthread_id, &expected_id),
	      "creating thread");
	TESTR(pthread_join(thread_id, NULL), "joining thread");

	TEST(expected_id == thread_id,
	     "Expected thread id %p to equal thread id %p\n", thread_id,
	     expected_id);

	return t_status;
}
