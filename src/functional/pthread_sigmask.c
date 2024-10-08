/*
 * pthread_sigmask.c unit test
 */
#include "test.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TESTR(f, g) \
	do { \
		if ((f) != 0) { \
			t_error(#f " failed: %s in test %s\n", strerror(errno), g); \
			return t_status; \
		} \
	} while (0)
#define TESTRT(f) ((f) || (t_error(#f " failed: %s", strerror(errno)), 0))

static volatile sig_atomic_t unblocked_sig = 0;
static volatile sig_atomic_t unmasked_sig = 0;
static pthread_t unblocked_thread;
static pthread_t unmasked_thread;

static void sigusr_handler(int signum)
{
	if (signum == SIGUSR1) {
		if (pthread_self() == unblocked_thread) {
			// Expect that SIGUSR gets delivered instead of blocked and pending
			unblocked_sig = 1;
		}

		if (pthread_self() == unmasked_thread) {
			unmasked_sig = 1;
		}
	}
}

static void *inherited_sigmask(void *args)
{
	sigset_t sigset;
	int status = pthread_sigmask(0, 0, &sigset);

	TEST(status == 0, "Setting sigmask status: %d, expected 0\n", status);

	TEST(sigismember(&sigset, SIGUSR1),
	     "Expected SIGUSR1 in current sigmask\n");

	return 0;
}

static void *unblock_signal(void *args)
{
	sigset_t sigmask;
	TESTRT(sigemptyset(&sigmask) == 0);
	TESTRT(sigaddset(&sigmask, SIGUSR1) == 0);
	TESTRT(pthread_sigmask(SIG_UNBLOCK, &sigmask, 0) == 0);

	while (!unblocked_sig) {
		// Busy wait for signal to be delivered
	}

	return 0;
}

static void *sigmask_union(void *args)
{
	sigset_t thread_sigmask;
	TESTRT(sigemptyset(&thread_sigmask) == 0);
	TESTRT(sigaddset(&thread_sigmask, SIGUSR2) == 0);

	sigset_t old_set;
	int status = pthread_sigmask(SIG_BLOCK, &thread_sigmask, &old_set);

	TEST(status == 0, "Setting sigmask status: %d, expected 0\n", status);

	TEST(sigismember(&old_set, SIGUSR1),
	     "Expected SIGUSR1 in current sigmask\n");

	sigset_t union_set;
	status = pthread_sigmask(0, 0, &union_set);
	TEST(status == 0, "Getting current sigmask status: %d, expected 0\n",
	     status);

	TEST(sigismember(&union_set, SIGUSR1),
	     "Expected SIGUSR1 in current sigmask\n");

	TEST(sigismember(&union_set, SIGUSR2),
	     "Expected SIGUSR2 in current sigmask\n");

	return 0;
}

static void *mask_signal(void *args)
{
	sem_t *sync = (sem_t *)args;
	sigset_t thread_sigmask, old_set;
	TESTRT(sigemptyset(&thread_sigmask) == 0);
	TESTRT(sigaddset(&thread_sigmask, SIGUSR2) == 0);

	int status = pthread_sigmask(SIG_SETMASK, &thread_sigmask, &old_set);

	TEST(status == 0, "Setting sigmask status: %d, expected 0\n", status);

	TEST(!sigismember(&old_set, SIGUSR1),
	     "Expected SIGUSR1 to be removed from old sigmask\n");

	TESTRT(sem_post(sync) == 0);
	int signal_caught = 0;
	TESTRT(sigwait(&thread_sigmask, &signal_caught) == 0);
	TEST(signal_caught == SIGUSR2,
	     "Expected sig catch SIGUSR2, instead caught: %d\n", signal_caught);

	return 0;
}

static int test_signal_deliveries(void)
{
	TESTR(pthread_create(&unblocked_thread, 0, unblock_signal, 0),
	      "creating thread");
	TESTR(pthread_kill(unblocked_thread, SIGUSR1), "sending SIGUSR1 to thread");

	TESTR(pthread_join(unblocked_thread, 0), "joining thread");

	TEST(unblocked_sig == 1, "Expected SIGUSR1 signal handler to execute\n",
	     unblocked_sig);
	return 0;
}

static int test_masked_signal(void)
{
	sem_t sync;
	TESTR(sem_init(&sync, 0, 0), "initialising semaphore");
	TESTR(pthread_create(&unmasked_thread, 0, mask_signal, &sync),
	      "creating thread");
	TESTR(sem_wait(&sync), "waiting for semaphore");
	TESTR(pthread_kill(unmasked_thread, SIGUSR1), "sending SIGUSR1 to thread");
	TESTR(pthread_kill(unmasked_thread, SIGUSR2), "sending SIGUSR2 to thread");
	TESTR(pthread_join(unmasked_thread, 0), "joining thread");
	TEST(unmasked_sig == 1, "Expected SIGUSR1 signal handler to execute\n");

	return 0;
}

static void test_invalid_param(void)
{
	const int how = SIG_SETMASK + 1;
	sigset_t sigmask;
	int status = sigemptyset(&sigmask);
	if (status != 0)
		t_error("Unable to set empty set, error: %d, %s", status,
		        strerror(status));

	status = pthread_sigmask(how, &sigmask, 0);
	TEST(status == EINVAL,
	     "Expected EINVAL on invalid how argument, instead got: %d\n", status);
}

static void test_currently_blocked(void)
{
	sigset_t current_set;
	int status = pthread_sigmask(0, 0, &current_set);
	TEST(status == 0,
	     "Expected pthread_sigmask to return currently blocked signals, "
	     "instead got status: %d\n",
	     status);
	TEST(sigismember(&current_set, SIGUSR1),
	     "Expected SIGUSR1 to be a member of currently blocked signals\n");
}

int main(void)
{
	test_invalid_param();

	const struct sigaction sa = {.sa_handler = sigusr_handler};
	struct sigaction restore;

	TESTR(sigaction(SIGUSR1, &sa, &restore), "setting sigaction");
	test_signal_deliveries();
	test_masked_signal();
	TESTR(sigaction(SIGUSR1, &restore, 0), "setting sigaction");

	sem_t sync;
	TESTR(sem_init(&sync, 0, 0), "initialising semaphore");

	sigset_t thread_sigmask;
	TESTR(sigemptyset(&thread_sigmask), "creating empty sigset");
	TESTR(sigaddset(&thread_sigmask, SIGUSR1), "adding SIGUSR1 to set");

	int status = pthread_sigmask(SIG_BLOCK, &thread_sigmask, 0);
	TEST(status == 0, "Setting sigmask status: %d, expected 0\n", status);
	test_currently_blocked();

	pthread_t thread_id = 0, thread_overwrite_id = 0;
	TESTR(pthread_create(&thread_id, 0, inherited_sigmask, &thread_sigmask),
	      "creating thread");
	TESTR(pthread_create(&thread_overwrite_id, 0, sigmask_union, &sync),
	      "creating thread");

	// Wait for the threads to finish
	TESTR(pthread_join(thread_id, 0), "joining thread");
	TESTR(pthread_join(thread_overwrite_id, 0), "joining thread");

	return t_status;
}
