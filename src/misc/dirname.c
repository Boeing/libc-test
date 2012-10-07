#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "test.h"

#define T(path, want) \
{ \
	char tmp[1000]; \
	char *got = dirname(strcpy(tmp, path)); \
	if (strcmp(want, got) != 0) \
		error("dirname(\"%s\") got \"%s\" want \"%s\"\n", path, got, want); \
}

int main()
{
	if (strcmp(dirname(0), ".") != 0)
		error("dirname(0) returned \"%s\"; expected \".\"\n", dirname(0));
	T("", ".");
	T("/usr/lib", "/usr");
	T("/usr/", "/");
	T("usr", ".");
	T("usr/", ".");
	T("/", "/");
	T("///", "/");
	T(".", ".");
	T("..", ".");
	return test_status;
}
