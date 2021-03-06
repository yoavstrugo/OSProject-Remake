
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>

#include <bits/ensure.h>
#include <mlibc/debug.hpp>
#include <internal-config.h>
#include <mlibc/posix-sysdeps.hpp>

int uname(struct utsname *p) {
	if (p == NULL) {
		errno = EFAULT;
		return -1;
	}

	if(!mlibc::sys_uname) {
		MLIBC_MISSING_SYSDEP();
		errno = ENOSYS;
		return -1;
	}
	if(int e = mlibc::sys_uname(p); e) {
		errno = e;
		return -1;
	}
	return 0;
}

