#include <sys/time.h>
#include <time.h>

int gettimeofday(struct timeval *tvp, struct timezone *tz)
{
	struct timespec ts;
	int ret = clock_gettime(CLOCK_REALTIME, &ts);
	if (ret == 0) {
		struct timeval tv = { ts.tv_sec, ts.tv_nsec / 1000 };
		*tvp = tv;
	}
	return ret;
}
