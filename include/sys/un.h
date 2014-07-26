#ifndef _SYS_UN_H_
#define _SYS_UN_H_

/* POSIX compliant */

#include <sys/socket.h>

struct sockaddr_un {
	sa_family_t	sun_family;	/* address family */
	char		sun_path[];	/* socket pathname */
};

#endif
