/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Christopher Williams
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

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sound.h>
#include <sys/utsname.h>

#include "setjmp.h"
#include "proc.h"
#include "punix.h"
#include "process.h"
#include "queue.h"
#include "inode.h"
#include "wait.h"
#include "globals.h"

/* process manipulation system calls */

void sys_sigreturn()
{
	/* FIXME: write this! */
}

/* XXX: user functions for testing */
extern ssize_t write(int fd, const void *buf, size_t count);
extern void _exit(int status);

extern void userstart();

static void println(char *s)
{
	write(2, s, strlen(s));
	write(2, "\n", 1);
}

int printf(const char *format, ...);

/* simple implementations of some C standard library functions */
void *kmalloc(size_t *size);

void *malloc(size_t size)
{
	if (!size) return NULL;
	return kmalloc(&size);
}

void *calloc(size_t size)
{
	void *ptr = malloc(size);
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

struct tm *gmtime_r(const time_t *tp, struct tm *tmp)
{
	int sec, min, hour, day;
	time_t t = *tp;
	sec = t % 60;  t /= 60;
	min = t % 60;  t /= 60;
	hour = t % 24; t /= 24;
	day = t;
	tmp->tm_sec = sec;
	tmp->tm_min = min;
	tmp->tm_hour = hour;
	/* FIXME: do the rest! */
	return tmp;
}

#define ESC "\x1b"
/* erase from cursor to end of line (inclusive) */
static void cleareol()
{
	printf(ESC "[K");
}

static void updatetop()
{
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
	struct itimerval it = {
		{ 0, 0 },
		{ 30, 0 }
	};
	struct sigaction sa;
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
	for (;;) {
		setitimer(ITIMER_REAL, &it, NULL);
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
	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);
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
				struct itimerval it = {
					{ 0, 0 },
					{ 3, 0 }
				};
				struct sigaction sa;
				sa.sa_handler = sigalrm;
				sa.sa_flags = SA_RESTART;
				printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
				setitimer(ITIMER_REAL, &it, NULL);
				printf("pause returned %d\n", pause());
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
	
	struct itimerval it = {
		{ 0, 0 },
		{ 10, 0 }
	};
	struct sigaction sa;
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
	
	for (;;) {
		setitimer(ITIMER_REAL, &it, NULL);
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

int usermain(int argc, char *argv[], char *envp[])
{
	const struct test *testp;
	int fd;
	
	fd = open("/dev/vt", O_RDWR); /* 0 */
	if (fd < 0) _exit(-1);
	dup(fd); /* 1 */
	dup(fd); /* 2 */
	
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
	
	char buf[1];
	ssize_t n;
	struct itimerval it = {
		{ 5, 0 },
		{ 5, 0 }
	};
	struct sigaction sa;
	sa.sa_handler = sigalrm;
	sa.sa_flags = SA_RESTART;
	printf("sigaction returned %d\n", sigaction(SIGALRM, &sa, NULL));
	setitimer(ITIMER_REAL, &it, NULL);
	
	long lasttime = 0;
	printf(ESC "[H" ESC "[J");
	for (;;) {
		updatetop();
		n = read(0, buf, 1);
		if (n >= 0)
			setitimer(ITIMER_REAL, &it, NULL);
	}
	
	return 0;
}
/* XXX */

static void copyenv(char ***vp, char **sp, char **sv)
{
	char *cp;
	char **v = *vp;
	char *s = *sp;
	
	for (; (cp = *sv); ++sv) {
		*v++ = s; /* set this vector for this string */
		while ((*s++ = *cp++) != '\0') /* copy the string */
			;
	}
	*v++ = NULL;
	*vp = v;
	*sp = s;
}

static void endvfork()
{
	struct proc *pp;
	
	pp = P.p_pptr;
	P.p_flag &= ~P_VFORK;
	wakeup(pp);
	while (!(P.p_flag&P_VFDONE))
		slp(pp, 0);
#if 0
	P.p_dsize = pp->p_dsize = 0; /* are these necessary? */
	P.p_ssize = pp->p_ssize = 0; /* " */
#endif
}

/* the following are inherited by the child from the parent (this list comes from execve(2) man page in FreeBSD 6.2):
	process ID           see getpid(2)
	parent process ID    see getppid(2)
	process group ID     see getpgrp(2)
	access groups        see getgroups(2)
	working directory    see chdir(2)
	root directory       see chroot(2)
	control terminal     see termios(4)
	resource usages      see getrusage(2)
	interval timers      see getitimer(2)
	resource limits      see getrlimit(2)
	file mode mask       see umask(2)
	signal mask          see sigvec(2), sigsetmask(2)
Essentially, don't touch those (except for things like signals set to be caught
are set to default in the new image).
*/
void sys_execve()
{
	struct a {
		const char *pathname;
		char **argp;
		char **envp;
	} *uap = (struct a *)P.p_arg;
	
	const char *pathname = uap->pathname;
	char **argp = uap->argp;
	char **envp = uap->envp;
	
	/*
	 * XXX: this version does not load the binary from a file, allocate
	 * memory, or any of that fun stuff (as these capabilities have not
	 * been implemented in Punix yet).
	 */
	
	
	void *text = userstart; /* XXX */
	char *data = NULL;
	char *stack = NULL;
	char *ustack = NULL;
	size_t textsize;
	size_t datasize;
	size_t stacksize;
	size_t ustacksize;
	
	long size = 0;
	char **ap;
	char *s, **v;
	int argc, envc;
	
	/* set up the user's stack (argc, argv, and env) */
	
	/* calculate the size of the arguments and environment + the size of
	 * their vectors */
	
	for (ap = argp; *ap; ++ap)
		size += strlen(*ap) + 1;
	argc = ap - argp; /* number of arg vectors */
	
	for (ap = envp; *ap; ++ap)
		size += strlen(*ap) + 1;
	envc = ap - envp; /* number of env vectors */
	
#define STACKSIZE 1024
#define USTACKSIZE 1024
#define UDATASIZE 1024
	/* allocate the system stack */
	stacksize = STACKSIZE;
	stack = memalloc(&stacksize, 0);
	/* if (!stack) goto ... */
	/* allocate the user stack */
	ustacksize = USTACKSIZE;
	ustack = memalloc(&ustacksize, 0);
	/* if (!ustack) goto ... */
	ustack += ustacksize;
	
	/* allocate the data section */
	datasize = UDATASIZE;
	data = memalloc(&datasize, 0);
	/* if (!data) goto ... */
	
	size = (size + 1) & ~1; /* size must be even */
	s = ustack - size; /* start of strings */
	v = (char **)s - (argc + envc + 2); /* start of vectors */
	
	ustack = (char *)v;
	*--(int *)ustack = argc;
	
	/* copy arguments */
	copyenv(&v, &s, argp);
	copyenv(&v, &s, envp);
	
	if (P.p_flag & P_VFORK)
		endvfork();
	else {
		/* free our resources */
#if 1
		memfree(NULL, P.p_pid);
		if (P.p_stack)
			memfree(P.p_stack, 0);
		if (P.p_ustack)
			memfree(P.p_ustack, 0);
		if (P.p_text)
			memfree(P.p_text, 0);
		if (P.p_data)
			memfree(P.p_data, 0);
#endif
	}
	P.p_stack = stack;
	P.p_ustack = ustack;
	P.p_text = text;
	P.p_data = data;
	
	/* finally, set up the context to return to the new process image */
	if (setjmp(P.p_ssav))
		return;
	
	/* go to the new user context */
	P.p_ssav->usp = ustack;
	P.p_sfp->pc = text;
	longjmp(P.p_ssav, 1);
	
	return;
	
	/*
	 * open binary file (filename is given in the file if it's a script)
	 * allocate memory for user stack
	 * allocate memory for user data
	 * allocate memory for user text
	 * load text and data from binary file
	 * copy environment from old image to new image (special case for argv
	 *   if file is a script)
	 * close files set close-on-exec
	 * reset signal handlers
	 * set PC and SP in proc structure
	 * free resources used by the old image if it is not a vfork child
	 * wake up the parent if it is vforking
	 * return as normal
	 * 
	 * on error (error code is set prior to jumping to one of these points):
	 * close binary file
	 * free memory for user text
	 * free memory for user data
	 * free memory for user stack
	 * return
	 */
}

void doexit(int status)
{
	int i;
	struct proc *q;
	
	P.p_flag &= ~P_TRACED;
	P.p_sigignore = ~0;
	P.p_sig = 0;
	
#if 0
	for (i = 0; i < NOFILE; ++i) {
		struct file *f;
		f = P.p_ofile[i];
		P.p_ofile[i] = NULL;
		P.p_oflag[i] = 0;
		closef(f); /* crash in closef() */
	}
	ilock(P.p_cdir);
	i_unref(P.p_cdir);
	if (P.p_rdir) {
		ilock(P.p_rdir);
		i_unref(P.p_rdir);
	}
	P.p_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	if (P.p_flag & P_VFORK)
		endvfork();
	else {
		/* xfree(); */
		/* mfree(...); */
	}
	/* mfree(...); */
#endif
	
	if (P.p_pid == 1) {
#if 1
		if (WIFEXITED(status))
			kprintf("init exited normally with status %d\n", WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			kprintf("init terminated with signal %d\n", WTERMSIG(status));
		else if (WIFSTOPPED(status))
			kprintf("init stopped with signal %d\n", WSTOPSIG(status));
		else if (WIFCONTINUED(status))
			kprintf("init continued\n");
		else
			kprintf("init unknown status 0x%04x!\n", status);
#endif

		panic("init died");
	}
	
	P.p_waitstat = status;
	/* ruadd(&P.p_rusage, &P.p_crusage); */
	list_for_each_entry(q, &G.proc_list, p_list) {
		if (q->p_pptr == current) {
			q->p_pptr = G.initproc;
			wakeup(G.proclist);
			if (q->p_flag & P_TRACED) {
				q->p_flag &= ~P_TRACED;
				psignal(q, SIGKILL);
			} else if (q->p_status == P_STOPPED) {
				psignal(q, SIGHUP);
				psignal(q, SIGCONT);
			}
		}
	}
	psignal(P.p_pptr, SIGCHLD);
	wakeup(P.p_pptr);
	swtch();
}

void sys_pause()
{
	slp(sys_pause, 1);
}

/* XXX: fix this to conform to the new vfork() method */
void sys_exit()
{
	struct a {
		int status;
	} *ap = (struct a *)P.p_arg;
	
	doexit(W_EXITCODE(ap->status));
}

void sys_fork()
{
	P.p_error = ENOSYS;
}

void sys_vfork()
{
#if 1
	struct proc *cp = NULL;
	pid_t pid;
	void *sp = NULL;
	void setup_env(jmp_buf env, struct syscallframe *sfp, long *sp);
	
	goto nomem; /* XXX: remove this once vfork is completely written */
	
	/* spl7(); */
	
	cp = palloc();
	if (!cp)
		goto nomem;
	
	*cp = P; /* copy our own process structure to the child */
	
	pid = pidalloc();
	P.p_retval = pid;
	cp->p_pid = pid;
	
	/* allocate a kernel stack for the child */
	/* sp = kstackalloc(); */
	if (!sp)
		goto stackp;
	
	/* At this point there's no turning back! */
	
	cp->p_stack = sp;
	cp->p_stacksize = 0; /* XXX */
	cp->p_status = P_RUNNING; /* FIXME: use setrun() instead */
	cp->p_flag |= P_VFORK;
	
	/* use the new kernel stack but the same user stack */
	setup_env(cp->p_ssav, P.p_sfp, sp);
	
	/* wait for child to end its vfork */
	while (cp->p_flag & P_VFORK)
		slp(cp, 0);
	
	/* reclaim our stuff */
	cp->p_flag |= P_VFDONE;
	wakeup(current);
	return;
stackp:
	pfree(cp);
nomem:
#endif
	P.p_error = ENOMEM;
}

static void dowait4(pid_t pid, int *status, int options, struct rusage *rusage)
{
	struct proc *p;
	int nfound = 0;
	int s;
	struct rusage r;
	int error;
	
loop:
	list_for_each_entry(p, &G.proc_list, p_list)
	if (p->p_pptr == current) {
		if (pid > 0) {
			if (p->p_pid != pid) continue;
		} else if (pid == 0) {
			if (p->p_pgrp != P.p_pgrp) continue;
		} else if (pid < (pid_t)-1) {
			if (p->p_pgrp != -pid) continue;
		}
		kprintf("dowait4: found pid=%d (ppid=%d)\n", p->p_pid, p->p_pptr->p_pid);
		++nfound;
		if (P.p_flag & P_NOCLDWAIT)
			continue;
		if ((options & WCONTINUED) && p->p_status == P_RUNNING && (p->p_flag & P_WAITED)) {
			/* return with this proc */
			p->p_status &= ~P_WAITED;
			goto found;
		}
		if ((options & WUNTRACED) && p->p_status == P_STOPPED && !(p->p_flag & P_WAITED)) {
			/* return with this proc */
			p->p_status |= P_WAITED;
			goto found;
		}
		if (p->p_status == P_ZOMBIE) {
			/* FIXME: free this proc for use by new processes */
			goto found;
		}
	}
	
	if (!nfound) {
		P.p_error = ECHILD;
		return;
	}
	if (options & WNOHANG) {
		return;
	}
	error = tsleep(current, PWAIT|PCATCH, 0);
	if (!error) goto loop;
	return;
	
found:
	P.p_retval = p->p_pid;
	if (status) {
		if (copyout(status, &p->p_waitstat, sizeof(s)))
			P.p_error = EFAULT;
	}
	
	if (rusage) {
		/* fill in rusage */
		if (copyout(rusage, &p->p_rusage, sizeof(r)))
			P.p_error = EFAULT;
	}
	
	if (p->p_status == P_ZOMBIE) {
		pfree(p);
	}
}

/* first arg is the same as wait */
struct wait3a {
	int *status;
	int options;
	struct rusage *rusage;
};

/* first 3 args are the same as waitpid */
struct wait4a {
	pid_t pid;
	int *status;
	int options;
	struct rusage *rusage;
};

void sys_wait()
{
	struct wait3a *ap = (struct wait3a *)P.p_arg;
	dowait4(-1, ap->status, 0, NULL);
}

void sys_waitpid()
{
	struct wait4a *ap = (struct wait4a *)P.p_arg;
	dowait4(ap->pid, ap->status, ap->options, NULL);
}

/*
 * The following two system calls are XSI extensions and are not necessary for
 * POSIX conformance.
 */

#if 0
STARTUP(void sys_wait3())
{
	struct wait3a *ap = (struct wait3a *)P.p_arg;
	dowait4(-1, ap->status, ap->options, ap->rusage);
}

STARTUP(void sys_wait4())
{
	struct wait4a *ap = (struct wait4a *)P.p_arg;
	dowait4(ap->pid, ap->status, ap->options, ap->rusage);
}
#endif

void sys_getpid()
{
	P.p_retval = P.p_pid;
}

void sys_getppid()
{
	P.p_retval = P.p_pptr->p_pid;
}

static void donice(struct proc *p, int n)
{
	if (P.p_euid != 0 && P.p_ruid != 0 &&
	    P.p_euid != p->p_euid && P.p_ruid != p->p_euid) {
		P.p_error = EPERM;
		return;
	}
	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	if (n < p->p_nice && P.p_euid != 0) {
		P.p_error = EACCES;
		return;
	}
	p->p_nice = n;
}

void sys_nice()
{
	struct a {
		int inc;
	} *ap = (struct a *)P.p_arg;
	
	donice(current, P.p_nice + ap->inc);
	P.p_retval = P.p_nice - NZERO;
	
	/*
	 * setpriority() returns EACCES if process
	 * attempts to lower the nice value if the
	 * user does not have the appropriate
	 * privileges, but nice() returns EPERM
	 * instead. Curious.
	 */
	if (P.p_error == EACCES)
		P.p_error = EPERM;
}

void sys_getpriority()
{
	struct a {
		int which;
		id_t who;
	} *ap = (struct a *)P.p_arg;
	struct proc *p;
	pid_t pgrp;
	uid_t uid;
	int nice = PRIO_MAX + 1;
	
	switch (ap->which) {
	case PRIO_PROCESS:
		if (ap->who == 0) /* current process */
			p = current;
		else
			p = pfind(ap->who);
		
		if (p)
			nice = p->p_nice;
		break;
	case PRIO_PGRP:
		pgrp = ap->who;
		if (pgrp == 0) /* current process group */
			pgrp = P.p_pgrp;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_pgrp == pgrp && p->p_nice < nice)
				nice = p->p_nice;
		}
		break;
	case PRIO_USER:
		uid = ap->who;
		if (uid == 0) /* current user */
			uid = P.p_euid;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_euid == uid && p->p_nice < nice)
				nice = p->p_nice;
		}
		break;
	default:
		P.p_error = EINVAL;
	}
	
	if (nice <= PRIO_MAX)
		P.p_retval = nice - NZERO;
	else
		P.p_error = ESRCH;
}

void sys_setpriority()
{
	struct a {
		int which;
		id_t who;
		int value;
	} *ap = (struct a *)P.p_arg;
	struct proc *p;
	pid_t pgrp;
	uid_t uid;
	int found = 0;
	
	switch (ap->which) {
	case PRIO_PROCESS:
		if (ap->who == 0) /* current process */
			p = current;
		else
			p = pfind(ap->who);
		
		if (p) {
			++found;
			donice(p, ap->value);
		}
		break;
	case PRIO_PGRP:
		pgrp = ap->who;
		if (pgrp == 0) /* current process group */
			pgrp = P.p_pgrp;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_pgrp == pgrp) {
				++found;
				donice(p, ap->value);
			}
		}
		break;
	case PRIO_USER:
		uid = ap->who;
		if (uid == 0) /* current user */
			uid = P.p_euid;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_euid == uid) {
				++found;
				donice(p, ap->value);
			}
		}
		break;
	default:
		P.p_error = EINVAL;
	}
	
	if (!found)
		P.p_error = ESRCH;
}

void sys_getrlimit()
{
	struct a {
		int resource;
		struct rlimit *rlp;
	} *ap = (struct a *)P.p_arg;
	
	switch (ap->resource) {
	case RLIMIT_CPU:
	case RLIMIT_DATA:
	case RLIMIT_NOFILE:
	case RLIMIT_CORE:
	case RLIMIT_FSIZE:
	case RLIMIT_STACK:
	case RLIMIT_AS:
		if (copyout(ap->rlp, &P.p_rlimit[ap->resource], sizeof(struct rlimit)))
			P.p_error = EFAULT;
		break;
	default:
		P.p_error = EINVAL;
	}
}

void sys_setrlimit()
{
	struct a {
		int resource;
		const struct rlimit *rlp;
	} *ap = (struct a *)P.p_arg;
	
	/* FIXME: write this! */
	switch (ap->resource) {
	case RLIMIT_CPU:
		break;
	case RLIMIT_DATA:
		break;
	case RLIMIT_NOFILE:
		break;
	case RLIMIT_CORE:
	case RLIMIT_FSIZE:
	case RLIMIT_STACK:
	case RLIMIT_AS:
	default:
		P.p_error = EINVAL;
	}
}

void sys_getrusage()
{
	struct a {
		int who;
		struct rusage *r_usage;
	} *ap = (struct a *)P.p_arg;
	
/*
	P.p_error = ENOSYS;
	return;
*/
	
	switch (ap->who) {
	case RUSAGE_SELF:
		if (copyout(ap->r_usage, &P.p_rusage, sizeof(struct rusage)))
			P.p_error = EFAULT;
		break;
	case RUSAGE_CHILDREN:
		P.p_error = EINVAL; /* FIXME */
		break;
	default:
		P.p_error = EINVAL;
	}
}

void sys_uname()
{
	struct a {
		struct utsname *name;
	} *ap = (struct a *)P.p_arg;
	
	struct utsname me = {
		"Punix",    /* sysname */
		"timmy",    /* nodename */
		OS_VERSION, /* release */
		BUILD,      /* version */
		"m68k",     /* machine */
	};
	P.p_error = copyout(ap->name, &me, sizeof(me));
}

void sys_chdir()
{
	/*  int chdir(const char *path); */
	struct a {
		const char *path;
	} *ap = (struct a *)P.p_arg;
	struct inode *ip = NULL;
	
#ifdef NOTYET
	ip = namei(ap->path);
#endif
	if (!ip)
		return;
	if (!(ip->i_mode & IFDIR)) {
		P.p_error = ENOTDIR;
		return;
	}
	if (ip == P.p_cdir) return;
	i_unref(P.p_cdir);
	++ip->i_count; /* unless namei() does this ?? */
	P.p_cdir = ip;
}
