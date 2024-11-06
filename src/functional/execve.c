/*
 * execve unit test
 *
 * Note: The following POSIX errors is not testing:
 * - The new process image file has appropriate privileges and has a recognized
 *   executable binary format, but the system does not support execution of a
 *   file with this format (set errno to EINVAL). This is not tested as it is
 *   system dependent.
 */

#include "test.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TESTE(c, ...) (!(c) || (t_error(#c " failed: " __VA_ARGS__), 0))
#define TESTFILE "testfile"
#define DIR_NAME "testdir/"
#define NON_EXEC_PERM 0666
#define EXEC_PERM 0777
#define NON_SEARCH_PERM 0444

static void file_creator_helper(int valid, char *filename, mode_t perm)
{
	FILE *file = fopen(filename, "w");
	TESTE(file == NULL, "fopen() failed with errno: %s\n", strerror(errno));

	// writes shebang to file to make it a valid process image
	if (valid) {
		TESTE(fprintf(file, "#!/bin/bash\n") < 0,
		      "fprintf() failed with errno: %s\n", strerror(errno));
	}

	TESTE(chmod(filename, perm) == -1, "chmod() failed with errno: %s\n",
	      strerror(errno));

	TESTE(fclose(file) == EOF, "fclose() failed with errno: %s\n",
	      strerror(errno));
}

static void test_non_existent_file(void)
{
	char *argv[] = {TESTFILE, NULL};
	char *envp[] = {NULL};
	int retval = execve(argv[0], argv, envp);

	TEST(
	    retval == -1 && errno == ENOENT,
	    "Failed to set errno to ENOENT when given non-existant path. Returned  "
	    "%d, errno: %s\n",
	    retval, strerror(errno));
}

static void test_non_executable_file(void)
{
	file_creator_helper(1, TESTFILE, NON_EXEC_PERM);

	char *argv[] = {TESTFILE, NULL};
	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);

	TEST(retval == -1 && errno == EACCES,
	     "Failed to set errno to EACCES when given non-executable file. "
	     "Returned  "
	     "%d, errno: %s\n",
	     retval, strerror(errno));

	remove(TESTFILE);
}

static void test_non_regular_file(void)
{
	// path to dir rather than file
	TESTE(mkdir(DIR_NAME, EXEC_PERM) == -1, "mkdir() failed with errno: %s\n",
	      strerror(errno));

	char *argv[] = {DIR_NAME, NULL};
	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);

	TEST(retval == -1 && errno == EACCES,
	     "Failed to set errno to EACCES when given path to non-regular file. "
	     "Returned  "
	     "%d, errno: %s\n",
	     retval, strerror(errno));

	rmdir(DIR_NAME);
}

static void test_oversized_arg_list(void)
{
	file_creator_helper(1, TESTFILE, EXEC_PERM);

	// place argv on heap to avoid stack space restrictions
	char **argv = NULL;
	argv = malloc((ARG_MAX + 1) * sizeof(char *));
	if (argv == NULL) {
		t_error("malloc() failed with errno %s\n", strerror(errno));
	}

	// Fill the allocated memory
	for (int i = 0; i < ARG_MAX; ++i) {
		argv[i] = TESTFILE;
	}

	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);
	TEST(retval == -1 && errno == E2BIG,
	     "Failed to set errno to E2BIG when given path to oversized argument "
	     "list. "
	     "Returned  "
	     "%d, errno: %s\n",
	     retval, strerror(errno));

	remove(TESTFILE);
	free(argv);
}

static void test_search_permission_denied(void)
{
	TESTE(mkdir(DIR_NAME, NON_SEARCH_PERM) == -1,
	      "mkdir() failed with errno: %s\n", strerror(errno));

	char *argv[] = {DIR_NAME, NULL};
	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);

	TEST(retval == -1 && errno == EACCES,
	     "Failed to set errno to EACCES when search permission is denied for a "
	     "directory in the new process image file's path. "
	     "Returned  "
	     "%d, errno: %s\n",
	     retval, strerror(errno));

	rmdir(DIR_NAME);
}

static void test_symlink_loop(void)
{
	TESTE(symlink("link2", "link1") == -1, "symlink() failed with errno: %s\n",
	      strerror(errno));
	TESTE(symlink("link1", "link2") == -1, "symlink() failed with errno: %s\n",
	      strerror(errno));

	char *argv[] = {"link1", NULL};
	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);
	TEST(retval == -1 && errno == ELOOP,
	     "Failed to set errno to ELOOP when symbolic link loop exists in path. "
	     "Returned %d, errno: %s\n",
	     retval, strerror(errno));

	unlink("link1");
	unlink("link2");
}

static void test_oversized_path_name(void)
{
	char *path = NULL;
	path = malloc((NAME_MAX + 2) * sizeof(char));
	if (path == NULL) {
		t_error("malloc() failed with errno %s\n", strerror(errno));
	}

	memset(path, 'a', NAME_MAX + 1);
	path[NAME_MAX + 1] = '\0';

	char *argv[] = {path, NULL};
	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);
	TEST(retval == -1 && errno == ENAMETOOLONG,
	     "Failed to set errno to ENAMETOOLONG when path string is greater than "
	     "NAME_MAX."
	     "Returned %d, errno: %s\n",
	     retval, strerror(errno));

	free(path);
}

static void test_not_a_directory(void)
{
	char *filename = "file";
	file_creator_helper(1, filename, EXEC_PERM);

	char *argv[] = {"file/", NULL};
	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);
	TEST(retval == -1 && errno == ENOTDIR,
	     "Failed to set errno to ENOTDIR when path prefix is an existing file "
	     "that "
	     "is not a directory or a symbolic link to a directory."
	     "Returned %d, errno: %s\n",
	     retval, strerror(errno));

	remove(filename);
}

static void test_unrecognised_format(void)
{
	file_creator_helper(0, TESTFILE, EXEC_PERM);

	char *argv[] = {TESTFILE, NULL};
	char *envp[] = {NULL};

	int retval = execve(argv[0], argv, envp);
	TEST(retval == -1 && errno == ENOEXEC,
	     "Failed to set errno to ENOEXEC when file permissions are appropriate "
	     "but "
	     "format of file is unrecognized."
	     "Returned %d, errno: %s\n",
	     retval, strerror(errno));

	remove(TESTFILE);
}

static void test_success(void)
{
	file_creator_helper(1, TESTFILE, EXEC_PERM);

	pid_t pid = fork();

	if (pid == -1) {
		t_error("fork() failed with errno %s\n", strerror(errno));
	} else if (pid == 0) {
		// child
		char *argv[] = {TESTFILE, NULL};
		char *envp[] = {NULL};
		int retval = execve(argv[0], argv, envp);

		// if this is reached, execve failed
		t_error("execve failed with status %d. Errno: %s\n", retval,
		        strerror(errno));

		exit(EXIT_FAILURE);
	} else {
		// parent
		int status = -1;
		waitpid((pid_t)-1, &status, 0);

		TEST(status == 0, "execve() failed. Child process return %d\n",
		     status && 0377);
	}
	remove(TESTFILE);
}

int main(void)
{
	test_non_existent_file();
	test_non_executable_file();
	test_non_regular_file();
	test_oversized_arg_list();
	test_search_permission_denied();
	test_symlink_loop();
	test_oversized_path_name();
	test_not_a_directory();
	test_unrecognised_format();
	test_success();
	return t_status;
}
