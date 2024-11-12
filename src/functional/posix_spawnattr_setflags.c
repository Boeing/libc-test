#include "test.h"
#include <spawn.h>

#define TEST(c, ...) ((c) || (t_error(#c " failed: " __VA_ARGS__), 0))

int main(void)
{
	posix_spawnattr_t attr = {0};
	int status = posix_spawnattr_init(&attr);
	if (status != 0) {
		t_error("Failed to initialise spawnattr struct, status: %d\n", status);
	}

	TEST(attr.__flags == 0,
	     "Expected default attribute value of __flags to be 0, got %d\n",
	     attr.__flags);

	static const short test_args =
	    POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETSCHEDPARAM;
	status = posix_spawnattr_setflags(&attr, test_args);

	TEST(status == 0,
	     "Expected posix_spawnattr_setflags to return 0, instead got %d\n",
	     status);

	TEST(attr.__flags == test_args,
	     "Expected attribute value of __flags to be %d, got %d\n", test_args,
	     attr.__flags);

	posix_spawnattr_destroy(&attr);

	posix_spawnattr_init(&attr);
	posix_spawnattr_setflags(&attr, -1);

	TEST(attr.__flags == 0, "Expected value of __flags to be 0, got %d\n",
	     attr.__flags);

	posix_spawnattr_destroy(&attr);

	return t_status;
}
