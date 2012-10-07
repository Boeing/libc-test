#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"

#define TEST(c) do { \
	errno = 0; \
	if (!(c)) \
		error("%s failed (errno = %d)\n", #c, errno); \
} while(0)

int main(void)
{
	char tmp[] = "/tmp/testsuite-XXXXXX";
	char foo[6];
	int fd;
	FILE *f;

	TEST((fd = mkstemp(tmp)) > 2);
	TEST(write(fd, "hello", 6)==6);
	TEST(f = fdopen(fd, "rb"));
	if (f) {
		TEST(ftello(f)==6);
		TEST(fseeko(f, 0, SEEK_SET)==0);
		TEST(fgets(foo, sizeof foo, f));
		if (strcmp(foo,"hello") != 0)
			error("fgets read back wrong message: \"%s\" wanted: \"hello\"\n", foo);
		fclose(f);
	}
	if (fd > 2)
		TEST(unlink(tmp) != -1);
	return test_status;
}
