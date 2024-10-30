/*
 * mq_close unit test
 */

#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TESTE(c, ...) (!(c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define MQ_NAME "/testqueue"
#define PERMISSIONS (mode_t)0644

static void test_notification_request_removed(void)
{
	sem_t *sem1 = sem_open("/sem1", O_CREAT, PERMISSIONS, 0);
	TESTE(sem1 == SEM_FAILED, "sem_open() failed with errno %s\n",
	      strerror(errno));

	sem_t *sem2 = sem_open("/sem2", O_CREAT, PERMISSIONS, 0);
	TESTE(sem2 == SEM_FAILED, "sem_open() failed with errno %s\n",
	      strerror(errno));

	if (sem1 == NULL || sem2 == NULL) {
		return;
	}

	const struct sigevent sev = {.sigev_notify = SIGEV_NONE};

	pid_t pid = fork();

	if (pid == -1) {
		t_error("fork() failed with errno %s\n", strerror(errno));
	} else if (pid == 0) {
		// child process
		// create mq
		mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, PERMISSIONS, 0);
		TESTE(mqdes == (mqd_t)-1, "mq_open() failed with errno %s\n",
		      strerror(errno));

		// register child process for notification
		TESTE(mq_notify(mqdes, &sev) == -1,
		      "mq_notify() failed with errno %s\n", strerror(errno));

		TESTE(sem_post(sem1) == -1, "sem_post() failed with errno %s\n",
		      strerror(errno));

		TESTE(sem_wait(sem2) == -1, "sem_wait() failed with errno %s\n",
		      strerror(errno));

		// close the mq in the child process. This should remove the registered
		// notification and allow for another process to register.
		TESTE(mq_close(mqdes) == -1, "mq_close() failed with errno %s\n",
		      strerror(errno));

		TESTE(sem_post(sem1) == -1, "sem_post() failed with errno %s\n",
		      strerror(errno));

		// wait here for parent to finish testing
		TESTE(sem_wait(sem2) == -1, "sem_wait() failed with errno %s\n",
		      strerror(errno));
	} else {
		// parent process
		TESTE(sem_wait(sem1) == -1, "sem_wait() failed with errno %s\n",
		      strerror(errno));

		// open mq created by child process
		mqd_t mqdes = mq_open(MQ_NAME, O_RDWR);
		TESTE(mqdes == -1, "mq_open() failed with errno\n", strerror(errno));

		// attempt to register for notification (mq can only have one process
		// registered for notification at one time)
		TEST(mq_notify(mqdes, &sev) == -1 && errno == EBUSY,
		     "Expected mq_notify() to fail with errno %s, got %s\n",
		     strerror(EBUSY), strerror(errno));

		TESTE(sem_post(sem2) == -1, "sem_post() failed with errno %s\n",
		      strerror(errno));

		TESTE(sem_wait(sem1) == -1, "sem_wait() failed with errno %s\n",
		      strerror(errno));

		// This should now succeed
		TEST(mq_notify(mqdes, &sev) == 0, "mq_notify() failed with errno %s\n",
		     strerror(errno));

		TESTE(sem_post(sem2) == -1, "sem_post() failed with errno %s\n",
		      strerror(errno));

		waitpid(pid, NULL, 0);
		mq_close(mqdes);
		mq_unlink(MQ_NAME);
		sem_close(sem1);
		sem_unlink("/sem1");
		sem_close(sem2);
		sem_unlink("/sem2");
	}
}

int main(void)
{
	mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, PERMISSIONS, 0);

	TEST(mq_close(mqdes) == 0, "mq_close() failed with errno %s\n",
	     strerror(errno));

	TEST(mq_close(mqdes) == -1 && errno == EBADF,
	     "Invalid message queue descriptor test failed. Expected %s, got %s\n",
	     strerror(EBADF), strerror(errno));

	mq_unlink(MQ_NAME);

	test_notification_request_removed();

	return t_status;
}
