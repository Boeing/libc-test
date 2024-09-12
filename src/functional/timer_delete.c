/*
 * timer_delete.c unit test
 */
#include "test.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

#define SLEEP_NANO 500000
#define TIMER_SPEC \
	(struct itimerspec) \
	{ \
		.it_value = {0, SLEEP_NANO}, .it_interval = { 0, 0 } \
	}

typedef struct {
	clockid_t clock;
	int armed;
} timer_params;

static void dummy_signal_handler(int signum) {}

static void delete_timer(const timer_params *timer)
{
	timer_t timer_id = 0;
	if (timer_create(timer->clock, NULL, &timer_id) < 0) {
		t_error("Failed to created SIGEV_SIGNAL timer: %s\n", strerror(errno));
		return;
	}

	if (timer->armed) {
		if (timer_settime(timer_id, 0, &TIMER_SPEC, NULL) < 0) {
			t_error("Failed to arm SIGEV_SIGNAL timer: %s\n", strerror(errno));
		}
	}

	TEST(timer_delete(timer_id) == 0,
	     "Failed to delete SIGEV_SIGNAL timer: %d\n", timer_id);
}

static void delete_thread_timer(const timer_params *timer)
{
	timer_t timer_id = 0;
	struct sigevent sigev = {.sigev_signo = SIGRTMIN,
	                         .sigev_notify = SIGEV_THREAD};

	if (timer_create(timer->clock, &sigev, &timer_id) < 0) {
		t_error("Failed to create SIGEV_THREAD timer: %s\n", strerror(errno));
		return;
	}

	if (timer->armed) {
		if (timer_settime(timer_id, 0, &TIMER_SPEC, NULL) < 0) {
			t_error("Failed to arm SIGEV_THREAD timer: %s\n", strerror(errno));
		}
	}

	TEST(timer_delete(timer_id) == 0,
	     "Failed to delete SIGEV_THREAD timer: %d\n", timer_id);
}

static void delete_completed_timer(timer_t timer_id, clockid_t clock)
{
	struct sigaction sa = {.sa_handler = dummy_signal_handler};

	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		t_error("Error setting up signal handler: %s\n", strerror(errno));
		return;
	}

	if (timer_create(clock, NULL, &timer_id) < 0) {
		t_error("Failed to created SIGEV_SIGNAL timer: %s\n", strerror(errno));
		return;
	}

	if (timer_settime(timer_id, 0, &TIMER_SPEC, NULL) < 0) {
		t_error("Failed to arm SIGEV_SIGNAL timer: %s\n", strerror(errno));
	}

	// Wait for SIGARLM
	pause();

	TEST(timer_delete(timer_id) == 0,
	     "Failed to delete SIGEV_SIGNAL timer: %d\n", timer_id);
}

int main(void)
{
	static const clockid_t clocks_to_test[] = {CLOCK_REALTIME, CLOCK_MONOTONIC};
	static const size_t num_clocks =
	    sizeof(clocks_to_test) / sizeof(*clocks_to_test);

	for (int clock = 0; clock < num_clocks; ++clock) {
		const timer_params unarmed_timer = {0, clocks_to_test[clock]};
		const timer_params armed_timer = {1, clocks_to_test[clock]};
		// Delete an unarmed timer
		delete_timer(&unarmed_timer);

		// Delete an armed timer
		delete_timer(&armed_timer);

		// Delete a thread unarmed timer
		delete_thread_timer(&unarmed_timer);

		// Delete a thread armed timer
		delete_thread_timer(&armed_timer);

		// Let the timer run out, and delete
		timer_t timer_id = 0;
		delete_completed_timer(timer_id, clocks_to_test[clock]);

		// Retry the same operation with the same timer_id (should succeed)
		delete_completed_timer(timer_id, clocks_to_test[clock]);
	}

	return t_status;
}
