#include <sys/time.h>
#include <time.h>

int settimeofday(const struct timeval *tvp, struct timezone *tz)
{
	struct timespec ts = { tvp->tv_sec, tvp->tv_usec * 1000 };
	int ret = clock_settime(CLOCK_REALTIME, &ts);
	return ret;
}
