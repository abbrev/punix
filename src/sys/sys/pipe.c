#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include "punix.h"
#include "proc.h"
#include "param.h"
#include "inode.h"
#include "queue.h"
#include "globals.h"

#define PIPSIZ 4096

extern void slp(void *, int);

STARTUP(void plock(struct inode *ip))
{
	while (ip->i_flag & ILOCKED) {
		ip->i_flag |= IWANT;
		slp(ip, 0);
	}
	
	ip->i_flag |= ILOCKED;
}

STARTUP(void prele(struct inode *ip))
{
	ip->i_flag &= ~ILOCKED;
	
	if (ip->i_flag & IWANT) {
		ip->i_flag &= ~IWANT;
		wakeup(ip);
	}
}

/* int pipe(int *fd); */
STARTUP(void sys_pipe())
{
	struct a {
		int *fd;
	} *ap = (struct a *)P.p_arg;
	
	struct inode *ip;
	int rfd, wfd;
	struct file *rfp, *wfp;
	
	if (!ap->fd) {
		P.p_error = EFAULT;
		return;
	}
	
	/* get our inode and two file structures/descriptors */
	if ((ip = ialloc(G.pipedev)) == NULL)
		goto error_inode;
	if ((rfd = falloc()) < 0)
		goto error_readfile;
	rfp = P.p_ofile[rfd];
	if ((wfd = falloc()) < 0)
		goto error_writefile;
	wfp = P.p_ofile[wfd];
	
	ap->fd[0] = rfd;
	ap->fd[1] = wfd;
	
	wfp->f_type = rfp->f_type = DTYPE_PIPE;
	wfp->f_inode = rfp->f_inode = ip;
	wfp->f_flag = FWRITE;
	rfp->f_flag = FREAD;
	
	ip->i_count = 2;
	ip->i_mode = IFREG;
	ip->i_flag = IACC | IUPD | ICHG;
	return;
	
error_writefile:
	P.p_ofile[rfd]->f_count = 0;
	P.p_ofile[rfd] = NULL;
error_readfile:
	iput(ip);
error_inode:
}

STARTUP(void readp(struct file *fp))
{
	struct inode *ip;
	
	ip = fp->f_inode;
	
loop:
	plock(ip);
	if (ip->i_size == 0) {
		/* If there are not both reader and writer active,
		 * return without satisfying read. */
		prele(ip);
		if (ip->i_count < 2)
			return;
		ip->i_mode |= IREAD;
		slp(ip+2, 1);
		goto loop;
	}
	
	P.p_offset = fp->f_offset;
	readi(ip);
	fp->f_offset = P.p_offset;
	/* If reader has caught up with writer, reset offset and size to 0. */
	if (fp->f_offset == ip->i_size) {
		fp->f_offset = 0;
		ip->i_size = 0;
		if (ip->i_mode & IWRITE) {
			ip->i_mode &= ~IWRITE;
			wakeup(ip+1);
		}
	}
	prele(ip);
}

STARTUP(void writep(struct file *fp))
{
	int c;
	struct inode *ip;
	
	ip = fp->f_inode;
	c = P.p_count;
	
loop:
	plock(ip);
	if (c == 0) {
		prele(ip);
		P.p_count = 0;
		return;
	}
	
	/* If there are not both read and write sides of the pipe active,
	 * return error and signal too. */
	
	if (ip->i_count < 2) {
		prele(ip);
		P.p_error = EPIPE;
		psignal(&P, SIGPIPE);
		return;
	}
	
	if (ip->i_size >= PIPSIZ) {
		ip->i_mode |= IWRITE;
		prele(ip);
		slp(ip+1, 1);
		goto loop;
	}
	
	P.p_offset = ip->i_size;
	P.p_count = min((unsigned)c, (unsigned)PIPSIZ);
	c -= P.p_count;
	writei(ip);
	prele(ip);
	if (ip->i_mode & IREAD) {
		ip->i_mode &= ~IREAD;
		wakeup(ip+2);
	}
	goto loop;
}

