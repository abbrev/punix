#include <sys/time.h>
#include <errno.h>

#include "punix.h"
#include "queue.h"
#include "buf.h"
#include "dev.h"
#include "proc.h"
#include "inode.h"
#include "globals.h"

/*enum { INO_SOUGHT, INO_CREATE, INO_DELETE };*/

struct inode *namei(char *path, int flag)
{
	struct inode *dp;
	char c;
	char *cp;
	int eo;
	struct buf *bp;
	
	/*
	 * If name starts with '/', start from root.
	 * Otherwise start from current dir.
	 */
	
	dp = P.p_cdir;
	if ((c = *path++) == '/')
		dp = P.p_rdir;
	iget(dp->i_dev, dp->i_number);
	while (c == '/')
		c = *path++;
	if (c == '\0' && flag != INO_SOUGHT) {
		P.p_error = ENOENT;
		goto out;
	}
	
cloop:
	/*
	 * Here dp contains pointer to last component matched.
	 */
	
	if (P.p_error)
		goto out;
	if (c == '\0')
		return dp;
	
	/*
	 * If there is another component, dp must be a directory and
	 * must have x permission.
	 */
	if ((dp->i_mode&IFMT) != IFDIR) {
		P.p_error = ENOTDIR;
		goto out;
	}
	if (access(dp, IEXEC))
		goto out;
	
	/*
	 * Gather up name into users' dir buffer.
	 */
	
	cp = &P.p_dbuf[0];
	while (c != '/' && c != '\0' && P.p_error == 0) {
		if (cp < &P.p_dbuf[DIRSIZ])
			*cp++ = c;
		c = *path++;
	}
	while (cp < &P.p_dbuf[DIRSIZ])
		*cp++ = '\0';
	while (c == '/')
		c = *path++;
	if (P.p_error)
		goto out;
	
	/*
	 * Set up to search a directory.
	 */
	
	P.p_offset = 0;
	eo = 0;
	P.p_count = dp->i_size / (DIRSIZ+2);
	bp = NULL;
	
eloop:
	
	/*
	 * If at the end of the directory, the search failed. Report what
	 * is appropriate as per flag.
	 */
	
	if (P.p_count == 0) {
		if (bp)
			brelse(bp);
		if (flag == INO_CREATE && c == '\0') {
			if (access(dp, IWRITE))
				goto out;
			P.p_pdir = dp;
			if (eo)
				P.p_offset = eo - DIRSIZ - 2;
			else
				dp->i_flag |= IUPD;
			
			return NULL;
		}
		P.p_error = ENOENT;
		goto out;
	}
	
	/*
	 * If offset is on a block boundary,
	 * read the next directory block.
	 * Release previous if it exists.
	 */
	
	if ((P.p_offset & 0777) == 0) {
		if (bp)
			brelse(bp);
		bp = bread(dp->i_dev, bmap(dp, P.p_offset / 512));
	}
	
	/*
	 * Note first empty directory slot in eo for possible creat.
	 * String compare the directory entry and the current component.
	 * If they do not match, go back to eloop.
	 */
	
	bcopy(bp->b_addr+(P.p_offset&0777), &P.p_dent, (DIRSIZ+2)/2);
	P.p_offset += DIRSIZ+2;
	--P.p_count;
	if (P.p_dent.d_ino == 0) {
		if (eo == 0)
			eo = P.p_offset;
		goto eloop;
	}
	for (cp = &P.p_dbuf[0]; cp < &P.p_dbuf[DIRSIZ]; ++cp)
		if(*cp != cp[P.p_dent.d_name - P.p_dbuf])
			goto eloop;
	
	/*
	 * Here a component matched in a directory.
	 * If there is more pathname, go back to
	 * cloop, otherwise return.
	 */
	
	if (bp)
		brelse(bp);
	if (flag == INO_DELETE && c == '\0') {
		if (access(dp, IWRITE))
			goto out;
		return dp;
	}
	bp = dp->i_dev;
	iput(dp);
	dp = iget(bp, P.p_dent.d_ino);
	if (dp == NULL)
		return NULL;
	goto cloop;
	
out:
	iput(dp);
	return NULL;
}
