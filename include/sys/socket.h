#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

/* $Id: socket.h,v 1.1 2007/05/03 08:59:59 fredfoobar Exp $ */

/* this should be more or less POSIX compliant */

#include <sys/uio.h>

typedef long		socklen_t;
typedef unsigned	sa_family_t;

struct sockaddr {
	sa_family_t	sa_family;	/* address family */
	char		sa_data[];	/* socket address (variable-length) */
};

struct sockaddr_storage {
	sa_family_t	ss_family;
};

struct msghdr {
	void		*msg_name;	/* optional address */
	socklen_t	msg_namelen;	/* size of address */
	struct iovec	*msg_iov;	/* scatter/gather array */
	int		msg_iovlen;	/* members in msg_iov */
	void		*msg_control;	/* ancillary data */
	socklen_t	msg_controllen;	/* ancillary data buffer length */
	int		msg_flags;	/* flags on received message */
};

struct cmsghdr {
	socklen_t	cmsg_len;	/* data byte count, including the cmsghdr */
	int		cmsg_level;	/* originating protocol */
	int		cmsg_type;	/* protocol-specific type */
};

/* value for cmsg_type when cmsg_level is SOL_SOCKET */
#define SCM_RIGHTS	0x01	/* data array contains the access rights to be sent or received */

#define CMSG_DATA(cmsg)		(char *)((struct cmsghdr *)(cmsg) + 1)
#define CMSG_NXTHDR(mhdr, cmsg)	
#define CMSG_FIRSTHDR(mhdr)	\
	((size_t)(mhdr)->msg_controllen >= sizeof(struct cmsghdr) \
	? (struct cmsghdr *)(mhdr)->msg_control \
	: (struct cmsghdr *)NULL)

struct linger {
	int	l_onoff;	/* indicates whether linger option is enabled */
	int	l_linger;	/* linger time, in seconds */
};

#define SOCK_STREAM	1	/* byte-stream socket */
#define SOCK_DGRAM	2	/* datagram socket */
#define SOCK_RAW	3	/* raw protocol interface */
#define SOCK_SEQPACKET	5	/* sequence-packet socket */

/* value for 'level' argument of setsockopt() and getsockopt() */
#define SOL_SOCKET	1

/* values for 'option_name' argument of getsockopt() and setsockopt() */
#define SO_ACCEPTCONN	1
#define SO_BROADCAST	2
#define SO_DEBUG	3
#define SO_DONTROUTE	4
#define SO_ERROR	5
#define SO_KEEPALIVE	6
#define SO_LINGER	7
#define SO_OOBINLINE	8
#define SO_RCVBUF	9
#define SO_RCVLOWAT	10
#define SO_RCVTIMEO	11
#define SO_REUSEADDR	12
#define SO_SNDBUF	13
#define SO_SNDLOWAT	14
#define SO_TYPE		15

/* maximum backlog queue length as specified by the 'backlog' field of listen() */
#define SOMAXCONN	128	/* is this too high? */

#define MSG_OOB		0x001
#define MSG_PEEK	0x002
#define MSG_DONTROUTE	0x004
#define MSG_CTRUNC	0x008
#define MSG_TRUNC	0x020
#define MSG_EOR		0x080
#define MSG_WAITALL	0x100

/* address families */
#define AF_UNSPEC	0	/* unspecified */
#define AF_UNIX		1	/* UNIX domain sockets */
#define AF_INET		2	/* Internet domain sockets for use with IPv4 addresses */
#define AF_INET6	3	/* Internet domain sockets for use with IPv6 addresses */

#define SHUT_RD		0	/* disable further receive operations */
#define SHUT_WR		1	/* disable further send operations */
#define SHUT_RDWR	2	/* disable further send and receive operations */

int	accept(int __s, struct sockaddr *__addr, socklen_t *__addrlen);
int	bind(int __s, const struct sockaddr *__addr, socklen_t __addrlen);
int	connect(int __s, const struct sockaddr *__addr, socklen_t __addrlen);
int	getpeername(int __s, struct sockaddr *__name, socklen_t *__namelen);
int	getsockname(int __s, struct sockaddr *__name, socklen_t *__namelen);
int	getsockopt(int __s, int __level, int __optname, void *__optval,
		socklen_t *__optlen);
int	listen(int __s, int __backlog);
ssize_t	recv(int __s, void *__buf, size_t __len, int __flags);
ssize_t	recvfrom(int __s, void *__buf, size_t __len, int __flags,
		struct sockaddr *__addr, socklen_t *__addrlen);
ssize_t	recvmsg(int __s, struct msghdr *__msg, int __flags);
ssize_t	send(int __s, const void *__buf, size_t __len, int __flags);
ssize_t	sendmsg(int __s, const struct msghdr *__msg, int __flags);
ssize_t	sendto(int __s, const void *__msg, size_t __len, int __flags,
		const struct sockaddr *__addr, socklen_t __addrlen);
int	setsockopt(int __s, int __level, int __optname, const void *__optval,
		socklen_t __optlen);
int	shutdown(int __s, int __how);
int	socket(int __s, int __type, int __protocol);
int	sockatmark(int __s);
int	socketpair(int __s, int __type, int __protocol, int __sv[2]);

#endif
