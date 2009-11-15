/*
 * Punix, Puny Unix kernel
 * Copyright 2009 Christopher Williams
 * 
 * $Id$
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _BSD_SOURCE

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sound.h>
#include <sys/utsname.h>


/*
 * XXX: The following includes should not be needed in a real user progam.
 * They are included only due to the lack of global variables and for the
 * reference to G.batt_level
 */
#include "punix.h"
#include "globals.h"

extern ssize_t write(int fd, const void *buf, size_t count);
extern void _exit(int status);

static void println(char *s)
{
	write(2, s, strlen(s));
	write(2, "\n", 1);
}

int printf(const char *format, ...);
int putchar(int);
//size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
//int fflush(FILE *stream);
int vfork(void);
int adjtime(const struct timeval *delta, struct timeval *olddelta);

/* simple implementations of some C standard library functions */
void *kmalloc(size_t *size);

void *malloc(size_t size)
{
	if (!size) return NULL;
	return kmalloc(&size);
}

void *calloc(size_t nelem, size_t elsize)
{
	void *ptr;
	size_t size;
	if (LONG_MAX / nelem < elsize)
		return 0;
	size = nelem * elsize;
	ptr = malloc(size);
	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}

void free(void *ptr)
{
	void kfree(void *ptr);
	kfree(ptr);
}

void *realloc(void *ptr, size_t size)
{
	return NULL; /* FIXME */
}

time_t time(time_t *tp)
{
	struct timeval tv;
	time_t t;
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	if (tp) *tp = t;
	return t;
}


#define EPOCHYEAR 1970
#define SECS_DAY 86400
#define YEAR0 1900
#define LEAPYEAR(year) ( (((year) % 4) == 0) && (((year) % 100) || ((year) % 400) == 0) )
#define YEARSIZE(year) (LEAPYEAR(year) ? 366 : 365)
static const int _ytab[2][12] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};
/* following implementation from Minix? */
struct tm *gmtime_r(const time_t *tp, struct tm *tmp)
{
	time_t t = *tp;
	unsigned long dayclock, dayno;
	int year = EPOCHYEAR;

	dayclock = (unsigned long)t % SECS_DAY;
	dayno = (unsigned long)t / SECS_DAY;

	tmp->tm_sec = dayclock % 60;
	tmp->tm_min = (dayclock % 3600) / 60;
	tmp->tm_hour = dayclock / 3600;
	tmp->tm_wday = (dayno + 4) % 7;       /* day 0 was a thursday */
	while (dayno >= YEARSIZE(year)) {
		dayno -= YEARSIZE(year);
		++year;
	}
	tmp->tm_year = year - YEAR0;
	tmp->tm_yday = dayno;
	tmp->tm_mon = 0;
	while (dayno >= _ytab[LEAPYEAR(year)][tmp->tm_mon]) {
		dayno -= _ytab[LEAPYEAR(year)][tmp->tm_mon];
		tmp->tm_mon++;
	}
	tmp->tm_mday = dayno + 1;
	tmp->tm_isdst = 0;

	return tmp;
}

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	time_t t = *timep;
	t -= 7 * 3600; /* XXX: works for me ;) */
	return gmtime_r(&t, result);
}

#if 0
       char *asctime(const struct tm *tm);
       char *asctime_r(const struct tm *tm, char *buf);

       char *ctime(const time_t *timep);
       char *ctime_r(const time_t *timep, char *buf);

       struct tm *gmtime(const time_t *timep);
       struct tm *gmtime_r(const time_t *timep, struct tm *result);

       struct tm *localtime(const time_t *timep);
       struct tm *localtime_r(const time_t *timep, struct tm *result);

       time_t mktime(struct tm *tm);
#endif

static const char _wtab[7][4] = {
	"Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
static const char _mtab[12][4] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
char *asctime_r(const struct tm *tm, char *buf)
{
	/* XXX: this should be sprintf(), but that isn't implemented yet */
	printf("%3.3s %3.3s %2d %02d:%02d:%02d %4.4d\n",
	        _wtab[tm->tm_wday], _mtab[tm->tm_mon], tm->tm_mday,
	        tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
	return NULL;
	return buf;
}

unsigned alarm(unsigned seconds)
{
	struct itimerval it, oldit;
	it.it_value.tv_sec = seconds;
	it.it_value.tv_usec =
	 it.it_interval.tv_sec =
	 it.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &it, &oldit);
#if 0
	/*
	 * We can't return 0 if there is any time remaining from a previous
	 * alarm() request. If there's at least a second remaining, just round
	 * the time to the nearest second.
	 */
	if ((!oldit.it_value.tv_sec && oldit.it_value.tv_usec) ||
	    oldit.it_value.tv_usec >= 500000L)
		++oldit.it_value.tv_sec;
#else
	/* round the remaining time up to a second */
	if (oldit.it_value.tv_usec)
		++oldit.it_value.tv_sec;
#endif
	return oldit.it_value.tv_sec;
}

#define ESC "\x1b"
/* erase from cursor to end of line (inclusive) */
static void cleareol()
{
	printf(ESC "[K");
}

static void updatetop()
{
	int getloadavg1(long loadavg[], int nelem);
	struct timeval tv;
	struct timeval difftime;
	long la[3];
	int day, hour, minute, second;
	long msec;
	long umsec, smsec;
	long t;
	int i;
	int ucpu = 42;
	int scpu = 31;
	struct rusage rusage;
	struct timeval ptime;
	struct timeval diffutime;
	struct timeval diffstime;
	
	getrusage(RUSAGE_SELF, &rusage);
	gettimeofday(&tv, NULL);
	
	if (G.lasttime.tv_sec != 0 || G.lasttime.tv_usec != 0) {
		timersub(&tv, &G.lasttime, &difftime);
		timersub(&rusage.ru_utime, &G.lastrusage.ru_utime, &diffutime);
		timersub(&rusage.ru_stime, &G.lastrusage.ru_stime, &diffstime);
		timeradd(&rusage.ru_utime, &rusage.ru_stime, &ptime);
		msec = difftime.tv_sec * 1000L + difftime.tv_usec / 1000;
		if (msec == 0) return;
		umsec = diffutime.tv_sec * 1000L + diffutime.tv_usec / 1000;
		smsec = diffstime.tv_sec * 1000L + diffstime.tv_usec / 1000;
		ucpu = (1000L * umsec + 500) / msec;
		scpu = (1000L * smsec + 500) / msec;
		getloadavg1(la, 3);
		
		/* line 1 */
		t = tv.tv_sec - 25200; /* -7 hours */
		second = t % 60; t /= 60;
		minute = t % 60; t /= 60;
		hour = t % 24;   t /= 24;
		day = t;
		printf(ESC "[H" "%02d:%02d:%02d up ",
		       hour, minute, second);
		t = uptime.tv_sec;
		second = t % 60; t /= 60;
		minute = t % 60; t /= 60;
		hour = t % 24;   t /= 24;
		day = t;
		if (day) {
			printf("%d+", day);
		}
		printf("%02d:%02d, 1 user, load:", hour, minute);
		for (i = 0; i < 3; ++i) {
			if (i > 0) putchar(',');
			printf(" %ld.%02ld", la[i] >> 16,
			       (100 * la[i] >> 16) % 100);
		}
		cleareol();
		/* line 2 */
		printf("\nTasks: 1 total, 1 run, 0 slp, 0 stop, 0 zomb");
		cleareol();
		/* line 3 */
		int totalcpu = ucpu+scpu;
		int idle;
		/* totalcpu is occasionally > 100% */
		if (totalcpu > 1000) totalcpu = 1000;
		idle = 1000 - totalcpu;
		printf("\nCpu(s): %3d.%01d%%us, %3d.%01d%%sy, %3d.%01d%%ni, %3d.%01d%%id", ucpu/10, ucpu%10, scpu/10, scpu%10, 0, 0, idle/10, idle%10);
		cleareol();
		/* line 4 */
		printf("\nMem: 0k total, 0k used, 0k free, 0k buffers");
		cleareol();
		/* line 5 */
		putchar('\n');
		cleareol();
		/* line 6 */
		printf("\n" ESC "[7m  PID USER      PR  NI  SHR  RES S  %%CPU   TIME COMMAND" ESC "[m");
		cleareol();
		/* line 7- */
		int pid = getpid();
		/* BOGUS! we can't really use 'current' in userspace */
		printf("\n%5d %-8s %3d %3d %3ldk %3ldk %c %3d.%01d %3d:%02d %.12s",
		       pid, "root", current->p_pri,
		       getpriority(PRIO_PROCESS, pid), (long)0, (long)0, 'R',
		       totalcpu / 10, totalcpu % 10,
		       (int)(ptime.tv_sec / 60), (int)(ptime.tv_sec % 60),
		       "top");
		cleareol();
		printf(ESC "[J" ESC "[5H");
	}
	G.lasttime = tv;
	G.lastrusage = rusage;
}

void userpause()
{
	ssize_t n;
	char buf[60];
	printf("Type ctrl-d to continue...\n");
	do
		n = read(0, buf, sizeof(buf));
	while (n != 0);
	putchar('\n');
}

static void testuname(int argc, char *argv[], char *envp[])
{
	struct utsname utsname;
	int err;
	
	err = uname(&utsname);
	if (!err) {
		printf("uname: %s %s %s %s %s\n",
		       utsname.sysname,
		       utsname.nodename,
		       utsname.release,
		       utsname.version,
		       utsname.machine);
	} else {
		printf("uname returned %d\n", err);
	}
}

static void sigalrm()
{
}

static void testtty(int argc, char *argv[], char *envp[])
{
	printf("Type ctrl-d to end input and go to the next test.\n");
	/* cat */
	char buf[60];
	ssize_t n;
	struct sigaction sa;
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
	for (;;) {
		alarm(30);
		n = read(0, buf, 60);
		if (n < 0) {
			printf("timeout\n");
			continue;
		}
		printf("read %ld bytes\n", n);
		if (n == 0) break;
		fwrite(buf, n, (size_t)1, NULL);
		fflush(NULL);
	}
	alarm(0);
}

#define BUFSIZE 512
static void testshell(int argc, char *argv[], char *envp[])
{
	struct utsname utsname;
	char *username = "root";
	char *hostname;
	char buf[BUFSIZE];
	ssize_t n;
	char *bp = buf;
	ssize_t len = 0;
	int neof = 0;
	int err;
	
	printf("stupid shell v -0.1\n");
	err = uname(&utsname);
	if (!err) {
		hostname = utsname.nodename;
	} else {
		hostname = "localhost";
	}
	
	for (;;) {
		if (len == 0)
			printf("%s@%s:%s%s ", username, hostname, "~", "$");
		n = read(0, bp, BUFSIZE - len);
		if (n < 0) {
			printf("read error\n");
		}
		len += n;
		bp += n;
		if (len == 0) {
			++neof;
			if (neof > 3) {
				printf ("exit\n");
				break;
			}
			printf("Use \"exit\" to leave the shell.\n");
			continue;
		}
		if (len == BUFSIZE || buf[len-1] == '\n') {
			char *cmd;
			size_t cmdlen;
			int i;
			buf[len-1] = '\0';
			len = 0;
			for (bp = buf; *bp == '\t' || *bp == ' ' || *bp == '\n'; ++bp)
				;
			cmd = bp;
			for (; *bp && *bp != '\t' && *bp != ' ' && *bp != '\n'; ++bp)
				;
			cmdlen = bp - cmd;
			bp = buf;
			if (cmdlen == 0)
				continue;
			
			if (!strncmp(cmd, "clear", cmdlen)) {
				printf("%s", ESC "[H" ESC "[J");
			} else if (!strncmp(cmd, "exit", cmdlen)) {
				break;
			} else if (!strncmp(cmd, "uname", cmdlen)) {
				int err;
				
				err = uname(&utsname);
				if (!err) {
					printf("%s %s %s %s %s\n",
					       utsname.sysname,
					       utsname.nodename,
					       utsname.release,
					       utsname.version,
					       utsname.machine);
				//} else {
					//printf("uname returned %d\n", err);
				}
			} else if (!strncmp(cmd, "env", cmdlen)) {
				char **ep = envp;
				for (; *ep; ++ep)
					printf("%s\n", *ep);
			} else if (!strncmp(cmd, "id", cmdlen)) {
				uid_t uid;
				gid_t gid;
				gid_t groups[15];
				int numgroups, i;
				
				uid = getuid();
				gid = getgid();
				numgroups = getgroups(15, groups);
				printf("uid=%d gid=%d groups=%d", uid, gid, gid);
				for (i = 0; i < numgroups; ++i) {
					printf(",%d", groups[i]);
				}
				putchar('\n');
			} else if (!strncmp(cmd, "wait", cmdlen)) {
				int status;
				pid_t pid = wait(&status);
			} else if (!strncmp(cmd, "vfork", cmdlen)) {
				pid_t pid;
				pid = vfork();
				if (pid == 0) {
					printf("I am the child\n");
					_exit(0);
				} else if (pid > 0) {
					printf("I am the parent\n");
				} else {
					printf("Sorry, there was an error vforking\n");
				}
			} else if (!strncmp(cmd, "pause", cmdlen)) {
				struct sigaction sa;
				sa.sa_handler = sigalrm;
				sa.sa_flags = SA_RESTART;
				printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
				alarm(3);
				printf("pause returned %d\n", pause());
			} else if (!strncmp(cmd, "batt", cmdlen)) {
				static const char *batt_level_strings[8] = { "dead", "almost dead", "starving", "very low", "low", "medium", "ok", "full" };
				printf("Battery level: %d (%s)\n", G.batt_level, batt_level_strings[G.batt_level]);
			} else if (!strncmp(cmd, "date", cmdlen)) {
				/*
				 * this prints out the equivalent of
				 * date --utc '+%F %T %Z'
				 */
				struct tm tm;
				struct timeval tv;
				char buf[40];
					
				gettimeofday(&tv, NULL);
				//printf("%5ld.%06ld\n", tv.tv_sec, tv.tv_usec);
				localtime_r(&tv.tv_sec, &tm);
#if 0
				printf("%4d-%02d-%02d %02d:%02d:%02d UTC\n",
				 tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
				 tm.tm_hour, tm.tm_min, tm.tm_sec);
#else
				printf("%s", asctime_r(&tm, buf));
#endif
			} else if (!strncmp(cmd, "adjtime", cmdlen)) {
				struct timeval tv = { 10, 0 };
				struct timeval oldtv;
				adjtime(NULL, &oldtv);
				if (oldtv.tv_sec || oldtv.tv_usec) {
					printf("adjtime(NULL, %ld.%06lu)\n",
					       oldtv.tv_sec, oldtv.tv_usec);
				} else {
					adjtime(&tv, &oldtv);
#define FIXTV(tv) do { \
if ((tv)->tv_sec < 0 && (tv)->tv_usec) { \
	++(tv)->tv_sec; \
	(tv)->tv_usec = 1000000L - (tv)->tv_usec; \
} \
} while (0)
					FIXTV(&tv);
					FIXTV(&oldtv);
					
					printf("adjtime(%ld.%06lu, %ld.%06lu)\n",
					       tv.tv_sec, tv.tv_usec,
					       oldtv.tv_sec, oldtv.tv_usec);
				}
			} else {
				printf("stupidsh: %.*s: command not found\n", (int)cmdlen, cmd);
			}
			bp = buf;
			len = 0;
			neof = 0;
		}
	}
}

static void testrandom(int argc, char *argv[], char *envp[])
{
	int i;
	int randomfd;
	int randombuf[100];
	
	randomfd = open("/dev/random", O_RDWR);
	printf("randomfd = %d\n", randomfd);
	
	if (randomfd >= 0) {
		read(randomfd, randombuf, sizeof(randombuf));
		for (i = 0; i < 100; ++i) {
			printf("%5u ", randombuf[i]);
		}
		printf("\n");
		close(randomfd);
	}
	
	userpause();
}

static void testaudio(int argc, char *argv[], char *envp[])
{
	int i;
	int audiofd;
	long audioleft[1024];
	long audioright[1024];
	long audiocenter[1024];
	
	audiofd = open("/dev/audio", O_RDWR);
	printf("audiofd = %d\n", audiofd);
	
	for (i = 0; i < 1024; ++i) {
		audioleft[i] = 0x00aaaa00;
		audioright[i] = 0x00555500;
		audiocenter[i] = 0x00ffff00;
	}
	
	printf("playing left...\n");
	write(audiofd, audioleft, sizeof(audioleft));
	ioctl(audiofd, SNDCTL_DSP_SYNC);
	
	printf("playing right...\n");
	write(audiofd, audioright, sizeof(audioright));
	ioctl(audiofd, SNDCTL_DSP_SYNC);
	
	printf("playing both...\n");
	write(audiofd, audiocenter, sizeof(audiocenter));
#if 0
	ioctl(audiofd, SNDCTL_DSP_SYNC); /* close() automatically sync's */
#endif
	
	printf("closing audio...\n");
	close(audiofd);
	
	userpause();
}

static const char varpkt1[] = {
	0x88, 0x56, 0x00, 0x00, /* ACK */
	0x88, 0x09, 0x00, 0x00, /* CTS */
};
static const char varpkt2[] = {
	0x88, 0x56, 0x00, 0x00, /* ACK */
};

static const char rdypkt[] = {
	0x88, 0x92, 0x00, 0x00, /* RDY */
};

struct pkthead {
	unsigned char machid;
	unsigned char commid;
	unsigned int length;
};

static const struct machinfo {
	int id;
	char desc[80];
} machinfo[] = {
	{ 0x08, "PC -> 89/92+/V200" },
	{ 0x88, "92+/V200 -> PC or 92+/V200 -> 92+/V200" },
	{ 0x98, "89 (Titanium) -> PC or 89 (Titanium) to 89 (Titanium)" },
	{ -1, "" },
};

static const struct comminfo {
	int id;
	char desc[5];
	int hasdata;
} comminfo[] = {
	{ 0x06, "VAR",  1 },
	{ 0x09, "CTS",  0 },
	{ 0x15, "DATA", 1 },
	{ 0x2d, "VER",  0 },
	{ 0x36, "MEM",  1 },
	{ 0x56, "ACK",  0 },
	{ 0x5a, "ERR",  0 },
	{ 0x68, "RDY",  0 },
	{ 0x6d, "SCR",  0 },
	{ 0x78, "CONT", 0 },
	{ 0x87, "CMD",  0 },
	{ 0x88, "DEL",  1 },
	{ 0x92, "EOT",  0 },
	{ 0xa2, "REQ",  1 },
	{ 0xc9, "RTS",  1 },
	{ -1, "", 0 }
};
#define PKT_COMM_VAR 0x06
#define PKT_COMM_CTS 0x09
#define PKT_COMM_DATA 0x15
#define PKT_COMM_XDP PKT_COMM_DATA
#define PKT_COMM_VER 0x2d
#define PKT_COMM_MEM 0x36
#define PKT_COMM_ACK 0x56
#define PKT_COMM_OK PKT_COMM_ACK
#define PKT_COMM_ERR 0x5a
#define PKT_COMM_RDY 0x68
#define PKT_COMM_SCR 0x6d
#define PKT_COMM_CONT 0x78
#define PKT_COMM_CMD 0x87
#define PKT_COMM_DEL 0x88
#define PKT_COMM_EOT 0x92
#define PKT_COMM_DONE PKT_COMM_EOT
#define PKT_COMM_REQ 0xa2
#define PKT_COMM_RTS 0xc9

static unsigned cksum(unsigned char *buf, ssize_t count)
{
	unsigned sum = 0;
	unsigned char *p = buf;
	for (; count; --count)
		sum += *p++;
	sum &= 0xffff;
	return sum;
}

static int recvpkthead(struct pkthead *pkt, int fd)
{
	unsigned char head[4];
	ssize_t n;
	int machid, commid;
	unsigned length;
	const struct machinfo *mip;
	const struct comminfo *cip;
	
	/* read packet header */
	n = read(fd, head, 4);
	if (n < 4) return -1;
	
	machid = head[0];
	commid = head[1];
	length = head[2] | (head[3] << 8);
	for (mip = &machinfo[0]; mip->id != -1; ++mip)
		if (mip->id == machid) break;
	for (cip = &comminfo[0]; cip->id != -1; ++cip)
		if (cip->id == commid) break;
	//printf("machine id: 0x%02x (%s)\n", machid, mip->id == -1 ? "unknown" : mip->desc);
	//printf("command id: 0x%02x (%s)\n", commid, cip->id == -1 ? "unknown" : cip->desc);
	//printf("    length: %u\n", length);
	pkt->machid = machid;
	pkt->commid = commid;
	pkt->length = length;
	return 0;
}

static long recvpkt(struct pkthead *pkt, char *buf, size_t count, int fd)
{
	ssize_t n;
	unsigned length;
	unsigned checksum;
	unsigned char *p;
	unsigned buf2[2];
	const struct comminfo *cip;
	
	if (recvpkthead(pkt, fd) < 0)
		return -1;
	
	for (cip = &comminfo[0]; cip->id != -1; ++cip)
		if (cip->id == pkt->commid) break;
	
	if (!cip->hasdata) return 0;
	/* continue reading "length" bytes plus the checksum */
	length = pkt->length;
	if (length > count) return -1;
	p = buf;
	while (length > 0) {
		n = read(fd, p, length);
		if (n <= 0) return -1;
		length -= n;
		p += n;
	}
	n = read(fd, buf2, 2); /* checksum */
	if (n < 0) return -1;
	checksum = buf2[0] | (buf2[1] << 8);
	printf("checksum: %u (%s)\n", checksum, checksum == cksum(buf, pkt->length) ? "correct" : "INCORRECT");
	return checksum;
}

static int sendpkthead(const struct pkthead *pkt, int fd)
{
	char buf[4];
	buf[0] = pkt->machid;
	buf[1] = pkt->commid;
	buf[2] = pkt->length % 256;
	buf[3] = pkt->length / 256;
	if (write(fd, buf, 4) < 4)
		return -1;
	return 0;
}

/* read and discard len bytes and return the checksum */
static long discard(int fd, unsigned len)
{
	unsigned char buf[128];
	unsigned sum = 0;
	ssize_t n;
	int i;
	while (len) {
		n = read(fd, buf, len < sizeof(buf) ? len : sizeof(buf));
		//printf("discard: read %ld bytes\n", n);
		if (n < 0) return -1;
		for (i = 0; i < n; ++i)
			sum += buf[i];
		len -= n;
	}
	return sum;
}

static unsigned short getchecksum(int fd)
{
	unsigned char buf[2];
	if (read(fd, buf, 2) < 2) return -1;
	return buf[0] | (buf[1] << 8);
}

/* simple variable receiver - doesn't actually save variables anywhere */
static void getcalc(int fd)
{
	struct pkthead pkt;
	static const struct pkthead ackpkt = { 0x88, PKT_COMM_ACK, 0 };
	static const struct pkthead ack92ppkt = { 0x88, PKT_COMM_ACK, 0x1001 };
	static const struct pkthead ctspkt = { 0x88, PKT_COMM_CTS, 0 };
	static const struct pkthead errpkt = { 0x88, PKT_COMM_ERR, 0 };
	long n;
	unsigned short sum, checksum;
	enum {
		RECV_START, RECV_WAITDATA, RECV_RXDATA
	} state = RECV_START;
	
	struct sigaction sa;
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
	
	for (;;) {
		alarm(10);
		n = recvpkthead(&pkt, fd);
		if (n < 0) goto timeout;
		switch (state) {
		case RECV_START:
			printf("START\n");
			if (pkt.commid == PKT_COMM_VAR || pkt.commid == PKT_COMM_RTS) {
				sum = discard(fd, pkt.length);
				checksum = getchecksum(fd);
				if (checksum == sum) {
					sendpkthead(&ackpkt, fd);
					sendpkthead(&ctspkt, fd);
					state = RECV_WAITDATA;
				} else {
					sendpkthead(&errpkt, fd);
				}
			} else if (pkt.commid == PKT_COMM_RDY) {
				if (pkt.machid == 0x00) {
					sendpkthead(&ack92ppkt, fd);
				} else {
					sendpkthead(&ackpkt, fd);
				}
			} else if (pkt.commid == PKT_COMM_EOT) {
				sendpkthead(&ackpkt, fd);
				return;
			} else {
				goto error;
			}
			break;
		case RECV_WAITDATA:
			printf("WAITDATA\n");
			if (pkt.commid != PKT_COMM_ACK)
				goto error;
			state = RECV_RXDATA;
			break;
		case RECV_RXDATA:
			printf("RXDATA\n");
			if (pkt.commid != PKT_COMM_DATA)
				goto error;
			//printf("reading and discarding variable data...\n");
			sum = discard(fd, pkt.length);
			//printf("reading variable data checksum...\n");
			checksum = getchecksum(fd);
			if (checksum == sum) {
				//printf("sending ACK\n");
				sendpkthead(&ackpkt, fd);
				state = RECV_START;
			} else {
				printf("sending ERR (checksum=0x%04x, expected=0x%04x\n", checksum, sum);
				sendpkthead(&errpkt, fd);
			}
			break;
		default:
			printf("getcalc internal error (%d)\n", state);
			return;
		}
		//userpause();
	}
error:
	printf("ERROR\n");
	//send something??
	return;
timeout:
	printf("TIMEOUT\n");
}

#define BPERLINE 8
static void printhex(char buf[], int length)
{
	int i, j;
	for (i = 0; i < (length + BPERLINE - 1) / BPERLINE * BPERLINE; ++i) {
		if (i < length)
			printf("%02x ", buf[i]);
		else
			printf("   ");
		if ((i + 1) % BPERLINE == 0) {
			for (j = i - BPERLINE + 1; j <= i; ++j) {
				int ch;
				ch = j < length ? buf[j] : ' ';
				if (ch < 31 || 126 < ch)
					ch = '.';
				putchar(ch);
			}
			putchar('\n');
		}
	}
	putchar('\n');
}

static void testlink(int argc, char *argv[], char *envp[])
{
	unsigned char *buf = NULL;
	size_t buflen;
	int linkfd;
	struct pkthead packet;
	
	linkfd = open("/dev/link", O_RDWR);
	printf("linkfd = %d\n", linkfd);
	if (linkfd < 0) goto out;
	
	printf("Receiving variable...\n");
	getcalc(linkfd);
	userpause();
	
	printf("sending RDY packet...\n");
	write(linkfd, rdypkt, sizeof(rdypkt));
	
	printf("reading packet...\n");
	buflen = 64*1024L;
	printf("buflen=%lu\n", buflen);
	buf = malloc(buflen);
	printf("buf=%08lx\n", buf);
	if (buf) {
		recvpkt(&packet, buf, buflen, linkfd); /* ACK */
		free(buf);
	} else {
		printf("testlink(): could not allocate buf\n");
	}
	userpause();
	
	printf("closing link...\n");
	close(linkfd);
	printf("closed.\n");
	
out:
	userpause();
}

static int banner(const char *s)
{
	static const char stars[] = "*****************************";
	int h, i;
	int l = strlen(s);
	h = l / 2;
	return printf("\n%s %s %s\n", &stars[h + (l%2)], s, &stars[h]);
}

struct test {
	char name[20];
	void (*func)();
};

static const struct test tests[] = {
	{ "uname() syscall", testuname },
	{ "/dev/tty", testtty },
	{ "shell", testshell },
	{ "/dev/random", testrandom },
	{ "/dev/link", testlink },
	{ "/dev/audio", testaudio },
	{ "", NULL }
};

int main_usertest(int argc, char *argv[], char *envp[])
{
	const struct test *testp;
#if 0
	println("This is a user program.");
	
	println("\narguments:");
	for (; *argv; ++argv)
		println(*argv);
	
	println("\nenvironment:");
	for (; *envp; ++envp)
		println(*envp);
#endif
	
	int i;
	struct timeval starttv;
	struct timeval endtv;
	struct timeval tv;
#if 0
	printf("waiting for the clock to settle...");
	gettimeofday(&starttv, 0);
	do {
		gettimeofday(&tv, 0);
	} while (tv.tv_sec < starttv.tv_sec + 3);
	printf("starting\n");
	gettimeofday(&starttv, 0);
	for (i = 0; i < 8000; ++i) {
		gettimeofday(&endtv, 0);
	}
	tv.tv_sec = endtv.tv_sec - starttv.tv_sec;
	tv.tv_usec = endtv.tv_usec - starttv.tv_usec;
	if (tv.tv_usec < 0) {
		tv.tv_usec += 1000000L;
		tv.tv_sec--;
	}
	long x = tv.tv_usec + 1000000L * tv.tv_sec;
	x /= 8;
	printf("%ld.%06ld = %ld ns per call = %ld calls per second\n", tv.tv_sec, tv.tv_usec, x, 1000000000L / x);
#endif
	
#if 0
	/* test invalid address */
	printf("testing syscall with an invalid pointer:\n");
	int error = gettimeofday((void *)0x40000 - 1, 0);
	printf("error = %d\n", error);
#endif
	
	for (testp = &tests[0]; testp->func; ++testp) {
		banner(testp->name);
		testp->func(argc, argv, envp);
	}
	
	argv[0] = "top";
	execve(argv[0], argv, envp);
	return 0;
}

int main_init(int argc, char *argv[], char *envp[])
{
	int fd;
	int x = 42;
	
	fd = open("/dev/vt", O_RDWR); /* 0 */
	if (fd < 0) return -1;
	dup(fd); /* 1 */
	dup(fd); /* 2 */
	
	printf("This is init. executing usertest now\n");
	argv[0] = "usertest";
	x = execve(argv[0], argv, envp);
	printf("still in init, execve returned %d\n", x);
	return 1;
}

int exec_command(int ch)
{
	switch (ch) {
	case 'q': return 1;
	case ' ': case '\n': break;
	default: printf("%s", "\rUnknown command"); break;
	}
	return 0;
}

#define TOPBUFSIZE 200
int main_top(int argc, char *argv[], char *envp[])
{
	/* we should eventually put the terminal in raw mode
	 * so we can read characters as soon as they're typed */
	
	char buf[TOPBUFSIZE];
	ssize_t n;
	int quit = 0;
#define TOPDELAY 5
	struct itimerval it = {
		{ TOPDELAY, 0 },
		{ TOPDELAY, 0 }
	};
	struct sigaction sa;
	
	printf("This is top.\n");
	
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
	setitimer(ITIMER_REAL, &it, NULL);
	
	//long lasttime = 0;
	
	printf(ESC "[H" ESC "[J");
		
	while (!quit) {
		updatetop();
		n = read(0, buf, TOPBUFSIZE);
		if (n >= 0) {
			size_t i;
			n = n > 0; /* discard all but the first character */
			for (i = 0; i < n; ++i)
				if (exec_command(buf[i]))
					quit = 1;
			setitimer(ITIMER_REAL, &it, NULL);
		}
	}
	
	return 0;
}
