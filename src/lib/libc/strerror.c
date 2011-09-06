#include <string.h>
#include <errno.h>

const int sys_nerr = 41;
const char *sys_errlist[] = {
	[0] = "Success",                     /* 0 */
	[EPERM] = "Operation not permitted",     /* 1 */
	[ENOENT] = "No such file or directory",
	[ESRCH] = "No such process",
	[EINTR] = "Interrupted system call",
	[EIO] = "I/O error",
	[ENXIO] = "No such device or address",
	[E2BIG] = "Argument list too long",
	[ENOEXEC] = "Exec format error",
	[EBADF] = "Bad file number",
	[ECHILD] = "No child processes",          /* 10 */
	[EAGAIN] = "Try again",
	[ENOMEM] = "Out of memory",
	[EACCES] = "Permission denied",
	[EFAULT] = "Bad address",
	[ENOTBLK] = "Block device required",
	[EBUSY] = "Device or resource busy",
	[EEXIST] = "File exists",
	[EXDEV] = "Cross-device link",
	[ENODEV] = "No such device",
	[ENOTDIR] = "Not a directory",             /* 20 */
	[EISDIR] = "Is a directory",
	[EINVAL] = "Invalid argument",
	[ENFILE] = "File table overflow",
	[EMFILE] = "Too many open files",
	[ENOTTY] = "Not a typewriter",
	[ETXTBSY] = "Text file busy",
	[EFBIG] = "File too large",
	[ENOSPC] = "No space left on device",
	[ESPIPE] = "Illegal seek",
	[EROFS] = "Read-only file system",       /* 30 */
	[EMLINK] = "Too many links",
	[EPIPE] = "Broken pipe",
	[EDOM] = "Math argument out of domain",
	[ERANGE] = "Math result not representable",
	[EDEADLK] = "Resource deadlock would occur",
	[ENAMETOOLONG] = "File name too long",
	[ENOLCK] = "No record locks available",
	[ENOSYS] = "Syscall not implemented",
	[ENOTEMPTY] = "Directory not empty",
	[ELOOP] = "Too many symbolic links"      /* 40 */
};

#define ERRSTRLEN 20
char *strerror(int e)
{
	//static char errstr[ERRSTRLEN+1];
	
	if (e < 0 || e >= sizeof(sys_errlist) / sizeof(sys_errlist[0])) {
		//sprintf("Unknown error %d", e);
		//return errstr;
		return "Unknown error";
	}
	return (char *)sys_errlist[e];
}
