/*
 * pthread_mutex_trylock unit test
 */
#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TESTR(f, g) \
	do { \
		if ((f) != 0) { \
			t_error(#f " failed: %s in test %s\n", strerror(errno), g); \
			return t_status; \
		} \
	} while (0)

static void *trylock_robust(void *arg)
{
	void **args = arg;
	pthread_mutex_t *lock = args[0];
	int status = pthread_mutex_trylock(lock);
	TEST(status == 0, "Expected trylock to succeed, instead got: %d\n", status);

	pthread_barrier_t *barrier = args[3];
	pthread_barrier_wait(barrier);

	sem_t *sync = args[2];
	sem_wait(sync);

	return 0;
}

static void *trylock(void *arg)
{
	void **args = arg;
	pthread_mutex_t *lock = args[0];
	int status = pthread_mutex_trylock(lock);
	TEST(status == 0, "Expected trylock to succeed, instead got: %d\n", status);

	int type = 0;
	pthread_mutexattr_t *attributes = args[1];
	if (pthread_mutexattr_gettype(attributes, &type) != 0)
		t_error("Failed to get mutex attributes\n");

	if (type == PTHREAD_MUTEX_RECURSIVE) {
		// mutex currently owned by the calling thread, should return
		// success
		status = pthread_mutex_trylock(lock);
		TEST(status == 0, "Expected trylock to return 0, instead got: %d\n",
		     status);

		if (pthread_mutex_unlock(lock) != 0)
			t_error("Couldn't unlock trylocked mutex\n");
	}

	if (pthread_mutex_unlock(lock) != 0)
		t_error("Couldn't unlock trylocked mutex\n");

	if (pthread_mutex_lock(lock) != 0)
		t_error("Couldn't lock unlocked mutex\n");

	pthread_barrier_t *barrier = args[3];
	pthread_barrier_wait(barrier);

	sem_t *sync = args[2];
	sem_wait(sync);

	return 0;
}

static int spawn_and_test_mutex(void *(*run)(void *), pthread_mutex_t *mutex,
                                sem_t *sync, pthread_mutexattr_t *attributes)
{
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, 0, 2);

	pthread_t thread_id = 0;
	void *args[] = {mutex, attributes, sync, &barrier};
	TESTR(pthread_create(&thread_id, 0, run, args), "pthread create trylock");

	pthread_barrier_wait(&barrier);

	// Mutex should be locked by thread, calling here should return EBUSY
	int status = pthread_mutex_trylock(mutex);
	TEST(status == EBUSY, "Trylock should return EBUSY, instead got %d\n",
	     status);

	sem_post(sync);
	TESTR(pthread_join(thread_id, NULL), "pthread join trylock");

	return 0;
}

static int spawn_and_test_robust(void *(*run)(void *), pthread_mutex_t *mutex,
                                 sem_t *sync, pthread_mutexattr_t *attributes)
{
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, 0, 2);

	pthread_t thread_id = 0;
	void *args[] = {mutex, attributes, sync, &barrier};
	TESTR(pthread_create(&thread_id, 0, run, args), "pthread create trylock");

	pthread_barrier_wait(&barrier);
	// While thread is still alive, trylock should return EBUSY, as it is
	// currently owned by another thread
	int status = pthread_mutex_trylock(mutex);
	TEST(status == EBUSY, "Trylock should return EBUSY, instead got %d\n",
	     status);

	sem_post(sync);
	TESTR(pthread_join(thread_id, NULL), "pthread join trylock");

	// Thread terminated holding mutex, expect EOWNERDEAD
	status = pthread_mutex_trylock(mutex);
	TEST(status == EOWNERDEAD,
	     "Trylock should return EOWNERDEAD, instead got %d\n", status);

	status = pthread_mutex_unlock(mutex);
	TEST(status == 0, "Unlock should succeed, instead got %d\n", status);

	// Second lock after unlock should return ENOTRECOVERABLE
	status = pthread_mutex_trylock(mutex);
	TEST(status == ENOTRECOVERABLE,
	     "Trylock should return ENOTRECOVERABLE, instead got %d\n", status);
	return 0;
}

static const struct {
	int mutex;
	int robust;
	char *mutex_name;
} tests[] = {
    {PTHREAD_MUTEX_NORMAL, PTHREAD_MUTEX_STALLED, "normal mutex"},
    {PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_STALLED, "recursive mutex"},
    {PTHREAD_MUTEX_ERRORCHECK, PTHREAD_MUTEX_STALLED, "error checking mutex"},

    {PTHREAD_MUTEX_NORMAL, PTHREAD_MUTEX_ROBUST, "robust normal mutex"},
    {PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ROBUST, "robust recursive mutex"},
    {PTHREAD_MUTEX_ERRORCHECK, PTHREAD_MUTEX_ROBUST,
     "robust error checking mutex"},
};

int main(void)
{
	const size_t num_tests = sizeof(tests) / sizeof(*tests);

	for (int test = 0; test < num_tests; ++test) {
		pthread_mutexattr_t attributes;
		char *current_test = tests[test].mutex_name;
		TESTR(pthread_mutexattr_init(&attributes), current_test);
		TESTR(pthread_mutexattr_settype(&attributes, tests[test].mutex),
		      current_test);

		TESTR(pthread_mutexattr_setrobust(&attributes, tests[test].robust),
		      current_test);

		pthread_mutex_t mutex;
		TESTR(pthread_mutex_init(&mutex, &attributes), current_test);

		sem_t sync;
		TESTR(sem_init(&sync, 0, 0), current_test);
		if (tests[test].robust == PTHREAD_MUTEX_STALLED)
			TESTR(spawn_and_test_mutex(trylock, &mutex, &sync, &attributes),
			      current_test);
		else
			TESTR(spawn_and_test_robust(trylock_robust, &mutex, &sync,
			                            &attributes),
			      current_test);

		TESTR(sem_destroy(&sync), current_test);
		TESTR(pthread_mutex_destroy(&mutex), current_test);
		TESTR(pthread_mutexattr_destroy(&attributes), current_test);
	}

	return t_status;
}
