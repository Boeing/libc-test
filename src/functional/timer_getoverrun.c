/*
 * timer_getoverrun.c test
 */
#include "test.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TIMER_EXPIRY 100000
#define SIG_BLOCKTIME 1000000
#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

static inline void dummy_signal_handler(int signum) {}
static void sigev_thread_cb(union sigval sig)
{
	timer_t *timer_id = sig.sival_ptr;
	if (*timer_id) {
		int overrun = timer_getoverrun(*timer_id);
		TEST(overrun >= 0,
		     "Expected timer SIGEV_THREAD timer to overrun zero or "
		     "more times, got %d, %p\n",
		     overrun, *timer_id);
	}
}

static void sigev_signal_cb(int sig, siginfo_t *si, void *uc)
{
	timer_t *timer_id = si->si_value.sival_ptr;
	int overrun = timer_getoverrun(*timer_id);
	TEST(overrun > 0,
	     "Expected timer to overrun at least %d times, instead got %d\n",
	     SIG_BLOCKTIME / TIMER_EXPIRY, overrun);
	struct sigaction sa = {.sa_handler = SIG_IGN};
	sigaction(SIGRTMIN, &sa, NULL);
}

static void test_timer_getoverrun(clockid_t clock_id, struct sigevent *sev)
{
	timer_t *timer_id = sev->sigev_value.sival_ptr;
	struct sigaction sa = {.sa_flags = SA_SIGINFO,
	                       .sa_sigaction = sigev_signal_cb};
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) != 0) {
		t_error("Failed to set signal action SIGRTMIN\n");
		return;
	}

	// Temporarily block signal to generate timer overrun
	sigset_t mask_sigalrm;
	sigemptyset(&mask_sigalrm);
	sigaddset(&mask_sigalrm, SIGRTMIN);

	if (sigprocmask(SIG_SETMASK, &mask_sigalrm, NULL) != 0) {
		t_error("Failed to mask SIGRTMIN\n");
		return;
	}

	if (timer_create(clock_id, sev, timer_id) != 0) {
		t_error("Failed to create timer\n");
		return;
	}

	const struct itimerspec timer_spec = {.it_value.tv_sec = 0,
	                                      .it_value.tv_nsec = TIMER_EXPIRY,
	                                      .it_interval.tv_sec = 0,
	                                      .it_interval.tv_nsec = TIMER_EXPIRY};

	if (timer_settime(*timer_id, 0, &timer_spec, NULL) != 0) {
		t_error("Error setting timer\n");
		timer_delete(*timer_id);
		return;
	}

	clock_nanosleep(clock_id, 0, &(struct timespec){0, SIG_BLOCKTIME}, NULL);

	if (sigprocmask(SIG_UNBLOCK, &mask_sigalrm, NULL) != 0) {
		t_error("Failed to unblock SIGRTMIN signal\n");
	}

	// Clean up timer
	TEST(timer_delete(*timer_id) == 0, "hello\n");
}

static void test_expired_timer(clockid_t clock_id, struct sigevent *sev)
{
	timer_t timer_id = 0;
	if (timer_create(clock_id, sev, &timer_id) != 0) {
		t_error("Failed to create timer\n");
		return;
	}

	const struct itimerspec timer_spec = {.it_value = {0, 0},
	                                      .it_interval = {0, 0}};

	if (timer_settime(timer_id, 0, &timer_spec, NULL) != 0) {
		t_error("Error setting timer\n");
		timer_delete(timer_id);
		return;
	}

	int overrun = timer_getoverrun(timer_id);
	TEST(overrun == 0,
	     "Expected timer to produce no overruns, instead got %d\n", overrun);

	timer_delete(timer_id);
}

static void test_created_timer(clockid_t clock_id, struct sigevent *sev)
{
	timer_t timer_id = 0;
	if (timer_create(clock_id, sev, &timer_id) != 0) {
		t_error("Failed to create timer\n");
		return;
	}

	int overrun = timer_getoverrun(timer_id);
	TEST(overrun == 0,
	     "Expected timer to produce no overruns, instead got %d\n", overrun);

	timer_delete(timer_id);
}

static void test_completed_timer(clockid_t clock_id, struct sigevent *sev)
{
	struct sigaction sa = {.sa_handler = dummy_signal_handler};
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) != 0) {
		t_error("Error setting up signal handler: %s\n", strerror(errno));
		return;
	}

	timer_t timer_id = 0;
	if (timer_create(clock_id, sev, &timer_id) < 0) {
		t_error("Failed to created SIGEV_SIGNAL timer: %s\n", strerror(errno));
		return;
	}

	if (timer_settime(
	        timer_id, 0,
	        &(struct itimerspec){{0, TIMER_EXPIRY}, {0, TIMER_EXPIRY}},
	        NULL) < 0) {
		t_error("Failed to arm SIGEV_SIGNAL timer: %s\n", strerror(errno));
	}

	// Wait for SIGRTMIN
	pause();

	int overrun = timer_getoverrun(timer_id);
	TEST(overrun >= 0,
	     "Expected that the timer may produce overruns, instead got %d\n",
	     overrun);

	timer_delete(timer_id);
}

static void test_timer_states(clockid_t clock_id, struct sigevent *sev)
{
	test_timer_getoverrun(clock_id, sev);
	test_expired_timer(clock_id, sev);
	test_created_timer(clock_id, sev);
}

int main(void)
{
	static const clockid_t clocks_to_test[] = {CLOCK_REALTIME, CLOCK_MONOTONIC};
	const size_t number_of_tests =
	    sizeof(clocks_to_test) / sizeof(*clocks_to_test);

	for (int clock = 0; clock < number_of_tests; ++clock) {
		timer_t timer_id = 0;
		struct sigevent sev = {.sigev_notify = SIGEV_SIGNAL,
		                       .sigev_signo = SIGRTMIN,
		                       .sigev_value.sival_ptr = &timer_id};

		// Test a SIGEV_SIGNAL timer
		test_completed_timer(clocks_to_test[clock], &sev);
		test_timer_states(clocks_to_test[clock], &sev);

		sev.sigev_notify = SIGEV_THREAD;
		sev.sigev_notify_function = sigev_thread_cb;

		// Test a SIGEV_THREAD timer
		test_timer_states(clocks_to_test[clock], &sev);
	}

	return t_status;
}
