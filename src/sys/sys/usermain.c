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
#include <ctype.h>
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
//int vfork(void);
int adjtime(const struct timeval *delta, struct timeval *olddelta);

/* simple implementations of some C standard library functions */
void *kmalloc(size_t *size);
void kfree(void *ptr);

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

static const char _wtab[7][3] = {
	"Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
static const char _mtab[12][3] = {
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

static void clear()
{
	printf(ESC "[H" ESC "[J");
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

static void sigalrm()
{
	printf("sigalrm\n");
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

#define GETCALC_READ_TIMEOUT 10

ssize_t getcalcread(int fd, void *buf, size_t count)
{
	alarm(GETCALC_READ_TIMEOUT);
	return read(fd, buf, count);
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
		n = getcalcread(fd, p, length);
		if (n <= 0) return -1;
		length -= n;
		p += n;
	}
	n = getcalcread(fd, buf2, 2); /* checksum */
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
		n = getcalcread(fd, buf, len < sizeof(buf) ? len : sizeof(buf));
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
	if (getcalcread(fd, buf, 2) < 2) return -1;
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
		n = recvpkthead(&pkt, fd);
		if (n < 0) goto timeout;
		switch (state) {
		case RECV_START:
			printf("START\n");
			if (pkt.commid == PKT_COMM_VAR || pkt.commid == PKT_COMM_RTS) {
				n = discard(fd, pkt.length);
				if (n < 0) goto timeout;
				sum = n;
				n = getchecksum(fd);
				if (n < 0) goto timeout;
				checksum = n;
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
			n = discard(fd, pkt.length);
			if (n < 0) goto timeout;
			sum = n;
			//printf("reading variable data checksum...\n");
			n = getchecksum(fd);
			if (n < 0) goto timeout;
			checksum = n;
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
	printf("buf=%08lx\n", (unsigned long)buf);
	if (buf) {
		recvpkt(&packet, buf, buflen, linkfd); /* ACK */
		free(buf);
	} else {
		printf("testlink(): could not allocate buf\n");
	}
	
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
	{ "/dev/random", testrandom },
	{ "/dev/link", testlink },
	{ "/dev/audio", testaudio },
	{ "", NULL }
};

typedef unsigned long softfloat;
softfloat fadd(softfloat, softfloat);

unsigned long long sl64(unsigned long long, unsigned);
unsigned long long sr64(unsigned long long, unsigned);

#define BUFSIZE 512

static int tests_main(int argc, char **argv, char **envp)
{
	const struct test *testp;
	for (testp = &tests[0]; testp->func; ++testp) {
		banner(testp->name);
		testp->func(argc, argv, envp);
	}
	printf("done!\n");
	return 0;
}

static int docat(int fd)
{
	char buf[BUFSIZE];
	ssize_t n;
	for (;;) {
		n = read(fd, buf, BUFSIZE);
		if (n <= 0) break;
		write(1, buf, n);
	}
	return n;
}

static int cat_main(int argc, char **argv, char **envp)
{
	int err, n;
	int fd;
	int i;
	
	if (argc <= 1) {
		n = docat(0);
		if (!n) err = 1;
		return err;
	}
	
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-")) {
			fd = 0; /* stdin */
		} else {
			fd = open(argv[i], O_RDONLY);
		}
		if (fd < 0) {
			printf("cat: %s: cannot open file for reading\n",
			       argv[i]);
			continue;
		}
		n = docat(fd);
		if (!n) err = 1;
		if (fd != 0) close(fd);
	}
	return err;
}

static int echo_main(int argc, char **argv, char **envp)
{
	int i;
	for (i = 1; i < argc; ++i) {
		if (i > 1) putchar(' ');
		printf("%s", argv[i]);
	}
	putchar('\n');
	return 0;
}

static int false_main(int argc, char **argv, char **envp)
{
	return 1;
}

static int true_main(int argc, char **argv, char **envp)
{
	return 0;
}

static int clear_main(int argc, char **argv, char **envp)
{
	clear();
	return 0;
}

static int uname_main(int argc, char **argv, char **envp)
{
	struct utsname utsname;
	int err;
	
	err = uname(&utsname);
	if (!err) {
		printf("%s %s %s %s %s\n",
		       utsname.sysname,
		       utsname.nodename,
		       utsname.release,
		       utsname.version,
		       utsname.machine);
	} else {
		printf("uname returned %d\n", err);
	}
	return 0;
}

static int env_main(int argc, char **argv, char **envp)
{
	char **ep = envp;
	for (; *ep; ++ep)
		printf("%s\n", *ep);
	return 0;
}

static int id_main(int argc, char **argv, char **envp)
{
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
	return 0;
}

static int pause_main(int argc, char **argv, char **envp)
{
	struct sigaction sa;
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
	alarm(3);
	printf("pause returned %d\n", pause());
	return 0;
}

static int batt_main(int argc, char **argv, char **envp)
{
	static const char
	  *batt_level_strings[8] = {
	    "dead",
	    "mostly dead",
	    "starving",
	    "very low",
	    "low",
	    "medium",
	    "ok",
	    "full"
	  };
	printf("Battery level: %d (%s)\n", G.batt_level,
	       batt_level_strings[G.batt_level]);
	return 0;
}

static int date_main(int argc, char **argv, char **envp)
{
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
	return 0;
}

static int adjtime_main(int argc, char **argv, char **envp)
{
	struct timeval tv = { 10, 0 };
	struct timeval oldtv;
	adjtime(&tv, &oldtv);

#define FIXTV(tv) do { \
if ((tv)->tv_sec < 0 && (tv)->tv_usec) { \
++(tv)->tv_sec; \
(tv)->tv_usec = 1000000L - (tv)->tv_usec; \
} \
} while (0)

	FIXTV(&tv);
	FIXTV(&oldtv);
	printf("previous time adjustment: %ld.%06lu\n",
	       oldtv.tv_sec, oldtv.tv_usec);
	printf("new time adjustment: %ld.%06lu\n",
	       tv.tv_sec, tv.tv_usec);
	return 0;
}

static int malloc_main(int argc, char **argv, char **envp)
{
	void *x;
	size_t s = 256*1024UL;
	while (s && !(x = malloc(s)))
		s -= 1024;
	free(x);
	printf("allocated up to %lu contiguous bytes\n", s);
	return 0;
}

static int exec_command(int ch)
{
	switch (ch) {
	case 'q': return 1;
	case ' ': case '\n': break;
	default: printf("\rUnknown command '%c'", ch); alarm(2); pause(); break;
	}
	return 0;
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
	int totalcpu;
	int idle;
	int pid;
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
		totalcpu = ucpu+scpu;
		/* totalcpu is occasionally > 100% */
		if (totalcpu > 1000) totalcpu = 1000;
		idle = 1000 - totalcpu;
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
		pid = getpid();
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

#define TOPBUFSIZE 200
static int top_main(int argc, char *argv[], char **envp)
{
	/* we should eventually put the terminal in raw mode
	 * so we can read characters as soon as they're typed */
	
	char buf[TOPBUFSIZE];
	ssize_t n;
	int quit = 0;
#define INITDELAY 1
#define TOPDELAY 5
	struct itimerval initit = {
		{ TOPDELAY, 0 },
		{ INITDELAY, 0 }
	};
	struct itimerval it = {
		{ TOPDELAY, 0 },
		{ TOPDELAY, 0 }
	};
	struct sigaction sa;
	
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sa, NULL);
	setitimer(ITIMER_REAL, &initit, NULL);
	
	G.lasttime.tv_sec = G.lasttime.tv_usec = 0;
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
	
	clear();
	
	return 0;
}

static int run(const char *, int, char **, char **);
struct applet {
	const char *name;
	int (*main)(int argc, char **argv, char **envp);
};
static struct applet applets[];

static void showhelp()
{
	struct applet *ap;
	printf("available applets:\n");
	for (ap = &applets[0]; ap->name; ++ap)
		printf(" %-9s", ap->name);
	printf("\n");
}

enum {
	TOKEN_ERR_SUCCESS = 0,
	TOKEN_ERR_EOL,
	TOKEN_ERR_UNMATCHED_DQUOTE,
	TOKEN_ERR_UNMATCHED_SQUOTE
};

const unsigned char _ctype[256] = {
	_C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C,
	_C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C,
	_S, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P,
	_N, _N, _N, _N, _N, _N, _N, _N, _N, _N, _P, _P, _P, _P, _P, _P,
	_P, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U,
	_U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _P, _P, _P, _P, _P,
	_P, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L,
	_L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _P, _P, _P, _P, _C,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

/*
          2 3 4 5 6 7       30 40 50 60 70 80 90 100 110 120
        -------------      ---------------------------------
       0:   0 @ P ` p     0:    (  2  <  F  P  Z  d   n   x
       1: ! 1 A Q a q     1:    )  3  =  G  Q  [  e   o   y
       2: " 2 B R b r     2:    *  4  >  H  R  \  f   p   z
       3: # 3 C S c s     3: !  +  5  ?  I  S  ]  g   q   {
       4: $ 4 D T d t     4: "  ,  6  @  J  T  ^  h   r   |
       5: % 5 E U e u     5: #  -  7  A  K  U  _  i   s   }
       6: & 6 F V f v     6: $  .  8  B  L  V  `  j   t   ~
       7: ´ 7 G W g w     7: %  /  9  C  M  W  a  k   u  DEL
       8: ( 8 H X h x     8: &  0  :  D  N  X  b  l   v
       9: ) 9 I Y i y     9: ´  1  ;  E  O  Y  c  m   w
       A: * : J Z j z
       B: + ; K [ k {
       C: , < L \ l |
       D: - = M ] m }
       E: . > N ^ n ~
       F: / ? O _ o DEL
*/

/*
 * get a token from the string *s, null terminate the token in *s, update *s
 * and *err, and return the token
 * also handle escaping (\) and quoting (" and ') and return an error
 * if there are unmatched quotes
 */
static char *gettoken(char **s, int *err)
{
	int quote = '\0';
	int escape = 0;
	char *c, *token;
	char *out;
	c = *s;
	if (*c == '\0') {
		/* no more tokens */
eol:
		*err = TOKEN_ERR_EOL;
		return NULL;
	}
	while (*c && isspace(*c))
		++c;
	if (*c == '\0') {
		/* no token found */
		goto eol;
	}
	token = c;
	out = c;
	while (*c && (escape || quote || !isspace(*c))) {
		if (escape) {
			/* copy this character literally */
			*out++ = *c;
			escape = 0;
		} else if (*c == '\'' || *c == '"') {
			if (!quote) {
				quote = *c;
			} else if (quote == *c) {
				quote = '\0';
			} else {
				*out++ = *c;
			}
		} else {
			if (*c == '\\' && !quote) escape = 1;
			else *out++ = *c;
		}
		++c;
	}
	if (quote) {
		if (quote == '\'')
			*err = TOKEN_ERR_UNMATCHED_SQUOTE;
		else if (quote == '"')
			*err = TOKEN_ERR_UNMATCHED_DQUOTE;
		return NULL;
	}
	if (*c) {
		*c++ = '\0';
	}
	*out = '\0';
	*s = c;
	return token;
}

#define MAXARGC (BUFSIZE/2+1)
static int sh_main(int argc, char **argv, char **envp)
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
	int uid;
	struct itimerval it = {
		{ 0, 0 },
		{ 0, 0 }
	};
	int aargc;
	char *aargv[BUFSIZE/2+1];
	uid = getuid();
	
	printf("stupid shell v -0.1\n");
	err = uname(&utsname);
	if (!err) {
		hostname = utsname.nodename;
	} else {
		hostname = "localhost";
	}
	
	for (;;) {
		setitimer(ITIMER_REAL, &it, NULL);
		if (len == 0)
			printf("%s@%s:%s%c ", username, hostname, "~",
			       uid ? '$' : '#');
		n = read(0, bp, BUFSIZE - len);
		if (n < 0) {
			printf("read error\n");
			return 1;
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
			char *s;
			char *token;
			int i;
			int err = 0;
			
			buf[len-1] = '\0';
			s = buf;
			aargc = 0;
			while ((token = gettoken(&s, &err)) != NULL) {
				if (aargc >= MAXARGC) {
					err = -1; /* XXX */
					break;
				}
				aargv[aargc++] = token;
			}
			if (err && err != TOKEN_ERR_EOL) {
				const char *errstr = "too many arguments";
				switch (err) {
				case TOKEN_ERR_UNMATCHED_DQUOTE:
					errstr = "unmatched double quote";
					break;
				case TOKEN_ERR_UNMATCHED_SQUOTE:
					errstr = "unmatched single quote";
					break;
				}
				printf("stupidsh: error: %s\n", errstr);
				goto nextline;
			}
			aargv[aargc] = NULL;
			
			cmd = aargv[0];
			if (!strcmp(cmd, "exit")) {
				break;
			} else if (!strcmp(cmd, "help")) {
				showhelp();
			} else {
				int n = run(cmd, aargc, aargv, envp);
				if (n < 0) {
					printf(
					  "stupidsh: %s: command not found\n",
					   cmd);
				}
			}
nextline:
			bp = buf;
			len = 0;
			neof = 0;
		}
	}
	return 0;
}

static struct applet applets[] = {
	{ "sh", sh_main },
	{ "tests", tests_main },
	{ "top", top_main },
	{ "cat", cat_main },
	{ "echo", echo_main },
	{ "true", true_main },
	{ "false", false_main },
	{ "clear", clear_main },
	{ "uname", uname_main },
	{ "env", env_main },
	{ "id", id_main },
	{ "pause", pause_main },
	{ "batt", batt_main },
	{ "date", date_main },
	{ "adjtime", adjtime_main },
	{ "malloc", malloc_main },

	{ NULL, NULL }
};

static int run_applet(const char *cmd, int argc, char **argv, char **envp)
{
	struct applet *ap;
	for (ap = &applets[0]; ap->name; ++ap) {
		if (!strcmp(cmd, ap->name)) {
			return ap->main(argc, argv, envp);
		}
	}
	return -1;
}

static int run(const char *cmd, int argc, char **argv, char **envp)
{
	int err;
	int pid;
	err = run_applet(cmd, argc, argv, envp);
	if (err >= 0) return err;
	
	/*
	 * here we would vfork and try to execve(2) the command
	 * in the child (in a real shell, that is)
	 */
	pid = vfork();
	if (pid == 0) {
		err = execve(cmd, argv, envp);
		printf("error: couldn't execute %s\n", cmd);
		_exit(err);
	} else if (pid > 0) {
		/* parent process. we chill here until the child dies */
		wait(&err);
	} else {
		printf("error: couldn't vfork\n");
	}
	
	return err;
}

int main_bittybox(int argc, char **argv, char **envp)
{
	int n = run_applet(argv[0], argc, argv, envp);
	if (n < 0) {
		printf("bittybox: unknown applet \"%s\"\n", argv[0]);
		showhelp();
	}
	return n;
}

int main_init(int argc, char *argv[], char *envp[])
{
	int fd;
	int err;
	int pid;
	
	fd = open("/dev/vt", O_RDWR); /* 0 */
	if (fd < 0) return -1;
	dup(fd); /* 1 */
	dup(fd); /* 2 */
	
	pid = vfork();
	if (pid <= 0) {
		if (pid < 0) {
			printf(
			  "Hey, this is init. There was a problem vforking.\n"
			  "Dropping to a single-process shell now.\n");
		}
		argv[0] = "sh";
		err = execve(argv[0], argv, envp);
		printf("still in init, execve failed with error %d\n", err);
	} 
	
	/* sit here and reap zombie processes */
	/* a real init process would also spawn tty's and stuff */
	for (;;) {
		wait(NULL);
	}
}
