/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Christopher Williams
 * 
 * $Id: file.c 354 2011-03-16 08:13:32Z fredfoobar $
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

/* these are generic file operation routines (in struct fileops) */

#include <limits.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "proc.h"
#include "file.h"
#include "buf.h"
#include "dev.h"
#include "punix.h"
#include "process.h"
/*
#include "uio.h"
*/
#include "inode.h"
#include "queue.h"
#include "globals.h"

off_t generic_file_lseek(struct file *fp, off_t offset, int whence)
{
	off_t base, newoffset;
	
	switch (whence) {
		case SEEK_SET:
			base = 0;
			break;
		case SEEK_CUR:
			base = fp->f_offset;
			break;
		case SEEK_END:
			base = fp->f_inode->i_size;
			break;
		default:
			P.p_error = EINVAL;
			return -1;
	}
	newoffset = base + offset;
	if (newoffset < 0) {
		/* if base and offset are both positive,
		 * newoffset is negative due to overflow */
		if (base > 0 && offset > 0)
			P.p_error = EOVERFLOW;
		else
			P.p_error = EINVAL;

		return -1;
	}

	fp->f_offset = newoffset;
	
	return newoffset;
}
