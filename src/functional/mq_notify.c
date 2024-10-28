/*
 * mq_notify unit test function
 *
 * Note: Due to the non-deterministic nature of the scheduler, it is difficult
 * to test that a thread calling mq_receive will receive the notification first
 * instead of a thread that has registered to be notified. It is possible with
 * a sleep call, however, this will yield unexpected failures in rare cases.
 */
#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define MSGQ_NAME "/mq_notify_queue"
#define PERM 0644

static sem_t sem_notify;
static int notify_flag = 0;

static void notify_handler(union sigval sig)
{
	notify_flag = 1;
	sem_post(&sem_notify);
}

static mqd_t create_messagequeue(int flags)
{
	mqd_t mqdes = mq_open(MSGQ_NAME, flags, PERM, 0);

	if (mqdes == -1) {
		t_error("Failed to create message queue with flags: %x. Error: %s\n",
		        flags, strerror(errno));
	}

	return mqdes;
}

static mqd_t create_mq_notify(const struct sigevent *sev)
{
	mqd_t mqdes = create_messagequeue(O_CREAT | O_RDWR);

	int status = mq_notify(mqdes, sev);
	TEST(status == 0, "Expected success, got %d, error: %s\n", status,
	     strerror(errno));

	return mqdes;
}

static void test_multiple_registrations(void)
{
	const struct sigevent sev = {.sigev_notify = SIGEV_NONE};
	mqd_t mqdes = create_mq_notify(&sev);

	int status = mq_notify(mqdes, &sev);
	TEST(status == -1, "Expected failure, got %d\n", status);
	TEST(errno == EBUSY, "Expected EBUSY errno, got %s\n", strerror(errno));

	mq_close(mqdes);
	mq_unlink(MSGQ_NAME);
}

static void test_notification_removal(void)
{
	const struct sigevent sev = {.sigev_notify = SIGEV_NONE};
	mqd_t mqdes = create_mq_notify(&sev);

	int status = mq_notify(mqdes, NULL);
	TEST(status == 0,
	     "Expected success on notification removal, got %d, error: %s\n",
	     status, strerror(errno));

	// Should be able to re-register
	status = mq_notify(mqdes, &sev);
	TEST(status == 0, "Expected success, got %d, error: %s\n", status,
	     strerror(errno));

	mq_close(mqdes);
	mq_unlink(MSGQ_NAME);
}

static void test_notification_received(void)
{
	const struct sigevent sev = {.sigev_notify = SIGEV_THREAD,
	                             .sigev_notify_function = notify_handler};
	mqd_t mqdes = create_mq_notify(&sev);

	sem_init(&sem_notify, 0, 0);
	const char message[] = "test";
	int status = mq_send(mqdes, message, sizeof(message), 0);
	if (status != 0) {
		t_error("Failed sending message to queue: %d, error: %s\n", status,
		        strerror(errno));
	}
	sem_wait(&sem_notify);
	TEST(notify_flag == 1, "Expected SIGEV_THREAD function to run\n");

	// Test that the process can re-register after receiving a mq notification
	const struct sigevent sev_none = {.sigev_notify = SIGEV_NONE};
	status = mq_notify(mqdes, &sev_none);
	TEST(status == 0,
	     "Expected mq_notify to allow registration, instead got status: %d\n",
	     status);

	sem_destroy(&sem_notify);
	mq_close(mqdes);
	mq_unlink(MSGQ_NAME);
}

static void test_bad_mqdes(void)
{
	const struct sigevent sev = {.sigev_signo = 0};
	int status = mq_notify(-1, &sev);

	TEST(status == -1,
	     "Expected return value of -1 for invalid mqdes, got %d\n", status);

	TEST(errno == EBADF, "Expected EBADF for invalid mqdes, got %s\n",
	     strerror(errno));
}

int main(void)
{
	errno = 0;
	test_bad_mqdes();
	test_multiple_registrations();
	test_notification_removal();
	test_notification_received();

	return t_status;
}
