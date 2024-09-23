/*
 * pthread_testcancel.c unit test
 */
#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TESTE(f, g) \
	do { \
		if ((f) != 0) { \
			t_error(#f " failed: %s in test %s\n", strerror(errno), g); \
			break; \
		} \
	} while (0)

#define THREAD_CANCELED 1
#define THREAD_FINISHED 2

typedef struct {
	void *(*run)(void *);
	const char *info;
	sem_t sync;
	int status;
} test_definition;

static void cleanup(void *arg) { *(int *)arg = THREAD_CANCELED; }

static void *test_cancel(void *arg)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	test_definition *test = arg;
	pthread_cleanup_push(cleanup, &test->status);
	sem_wait(&test->sync);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_testcancel();
	test->status = THREAD_FINISHED;
	pthread_cleanup_pop(0);
	return 0;
}

static void *test_cancel_disabled(void *arg)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	test_definition *test = arg;
	pthread_cleanup_push(cleanup, &test->status);
	sem_wait(&test->sync);

	// Cancelability is disabled, function should do nothing
	pthread_testcancel();
	test->status = THREAD_FINISHED;
	pthread_cleanup_pop(0);
	return 0;
}

static void *test_cancel_async(void *arg)
{
	test_definition *test = arg;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(cleanup, &test->status);

	sem_wait(&test->sync);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_testcancel();
	test->status = THREAD_FINISHED;
	pthread_cleanup_pop(0);
	return 0;
}

static void *test_cancel_loop(void *arg)
{
	test_definition *test = arg;
	pthread_cleanup_push(cleanup, &test->status);
	for (;;) {
		pthread_testcancel();
	}
	pthread_cleanup_pop(0);
	return 0;
}

int main(void)
{
	sem_t sync;
	TESTE(sem_init(&sync, 0, 0), "initialising semaphore");

	test_definition tests[] = {
	    {test_cancel, "test default cancel", sync, 0},
	    {test_cancel_disabled, "test disabled cancel", sync, 0},
	    {test_cancel_async, "test asynchronous cancel", sync, 0},
	    {test_cancel_loop, "test cancel loop", sync, 0}};

	int expected_args[] = {THREAD_CANCELED, THREAD_FINISHED, THREAD_CANCELED,
	                       THREAD_CANCELED};
	const size_t num_tests = sizeof(tests) / sizeof(*tests);

	for (int test = 0; test < num_tests; ++test) {
		pthread_t test_thread = 0;
		const char *current_info = tests[test].info;
		TESTE(pthread_create(&test_thread, 0, tests[test].run, &tests[test]),
		      current_info);

		TESTE(pthread_cancel(test_thread), current_info);
		TESTE(sem_post(&tests[test].sync), current_info);

		void *thread_status = 0;
		TESTE(pthread_join(test_thread, &thread_status), current_info);

		if (expected_args[test] == THREAD_CANCELED) {
			TEST(thread_status == PTHREAD_CANCELED,
			     "Expected cancel exit status, instead got: %d\n",
			     thread_status);
		}

		TEST(tests[test].status == expected_args[test],
		     "Unexpected execution of thread cleanup function, "
		     "expected thread %s to exit normally, status: %d\n",
		     tests[test].info, tests[test].status);
	}

	return t_status;
}
