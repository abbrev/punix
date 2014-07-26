#ifndef _DIRENT_H_
#define _DIRENT_H_

/* $Id$ */

/* somewhat POSIX compliant */

#include <sys/types.h>

/* FIXME */
/* Directory stream type.  */
typedef struct {
	int		dd_fd;		/* file descriptor */
	int		dd_loc;		/* offset in buffer */
	int		dd_size;	/* # of valid entries in buffer */
	struct dirent	*dd_buf;	/* -> directory buffer */
} DIR;				/* stream data from opendir() */

/*
typedef int (*__dir_select_fn_t)(struct dirent *);

typedef int (*__dir_compar_fn_t)(struct dirent **, struct dirent **);
*/

struct dirent {
	ino_t		d_ino;	/* [XSI] */
	off_t		d_off;
	unsigned short	d_reclen;
	char		d_name[NAME_MAX+1];
};

extern int	closedir(DIR *__dirp);
extern DIR	*opendir(const char *__dirname);
extern struct dirent	*readdir(DIR *__dirp);
extern void	rewinddir(DIR *__dirp);

/* [XSI] */
/* XSI specifies `long' in place of `off_t' */
extern void	seekdir(DIR *__dirp, long __loc);
extern long	telldir(DIR *__dirp);

#if 0 /* not in POSIX */
/* Scan the directory DIR, calling SELECT on each directory entry.
   Entries for which SELECT returns nonzero are individually malloc'd,
   sorted using qsort with CMP, and collected in a malloc'd array in
   *NAMELIST.  Returns the number of entries selected, or -1 on error.
 */
extern int scandir(char *__dir,
			 struct dirent ***__namelist,
			 __dir_select_fn_t __select,
			 __dir_compar_fn_t __compar);
#endif

#endif /* _DIRENT_H_ */
