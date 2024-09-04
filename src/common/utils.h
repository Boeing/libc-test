#ifndef UTILS_H

#define UTILS_H

#include "test.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/wait.h>

/*
 *In this code, we are usig a busy loop to simulate a CPU-intensive task that
 *consumes processing time, specifically to measure the system time (stime)
 *during its execution.
 *
 *By using the 'getpid()' function inside a 'for' loop, we create a controlled,
 *predictable CPU workload that forces userspace and kernel interaction. This
 *allows us to measure the system time (stime) and user time (utime).
 *
 *A 'while' loop could also be used to create this busy loop. However, a
 *'for' loop is preferred here because with a 'for' loop, there's less
 *risk of accidentally creating an infinite loop due to missed condition.
 *
 *'sleep()' function is not suitable as it does not create any CPU load for
 *the stime to tick.
 */

static inline void __attribute__((unused)) create_and_join_threads(
    const int num_threads, pthread_attr_t *attr_data,
    void *(*thread_func)(void *), void *thread_data, size_t data_size)
{
	pthread_t threads[num_threads];

	// create threads and pass the data
	for (int i = 0; i < num_threads; ++i) {
		void *arg =
		    thread_data ? (void *)((char *)thread_data + i * data_size) : NULL;
		pthread_attr_t *thread_attr = attr_data ? &attr_data[i] : NULL;
		int rc = pthread_create(&threads[i], thread_attr, thread_func, arg);
		if (rc != 0) {
			t_error("pthread_create thread_safety_function error: %d\n", rc);
		}
	}

	// wait for all the threads to complete
	for (int i = 0; i < num_threads; ++i) {
		pthread_join(threads[i], NULL);
	}
}

/*
 *This function is designed to create and manage multiple threads using a
 *contiguous array of data, such as an array of structures. Each thread
 *receives its own chunk of data form the array based on its index.
 *
 *Parameters:
 *
 *int num_threads: The number of threads to be created.
 *void* (*thread_func)(void*): A pointer to the function that each thread
 *      will execute. This function should accept a void* argument, which will
 *      be the data passed to each thread.
 *void* thread_data: A pointer to the contiguous block of data. Each thread
 *      will be passed a different part of this block based on its index.
 *      size_t data_size: The size (in bytes) of each individual element in the
 *      thread_data array. This is used to calculate the offcset for each
 *      thread's data.
 *pthread_attr_t* attr_data: (Optional) An array of pthread_attr_t structures,
 *      allowing the caller to specify custom attributes (such as stack size)
 *      for each thread. If NULL, default thread attributes are used.
 */

static inline void __attribute__((unused)) test_buffer_overflow(
    int (*run_buffer_overflow)(void))
{
	if (run_buffer_overflow == NULL) {
		t_error(
		    "run_buffer_overflow is NULL, expected a valid function pointer\n");
		return;
	}

	pid_t child_pid = fork();

	if (child_pid == -1) {
		t_error("test_buffer_overflow fork failed\n");
	}

	if (child_pid == 0) {
		exit(run_buffer_overflow());
	} else {
		int status = 0;
		waitpid(child_pid, &status, 0);

		if ((WIFSIGNALED(status) && (WTERMSIG(status) != SIGSEGV)) ||
		    WEXITSTATUS(status) == 1) {
			t_error("Expected child to return an error due to buffer "
			        "overflow, instead child exited with %d\n",
			        WEXITSTATUS(status));
		}
	}
}

#endif

