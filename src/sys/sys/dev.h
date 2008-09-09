#ifndef _DEV_H_
#define _DEV_H_

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct	bdevsw
{
	void	(*d_open)(struct file *fp, int rw);
	void	(*d_close)(struct file *fp, int rw);
	void	(*d_strategy)(struct buf *);
	const struct devtab *d_tab;
};

extern const struct bdevsw bdevsw[];

/*
 * Nblkdev is the number of entries
 * (rows) in the block switch. It is
 * set in binit/bio.c by making
 * a pass over the switch.
 * Used in bounds checking on major
 * device numbers.
 */
extern const int nblkdev;

/*
 * Character device switch.
 */
struct	cdevsw
{
	void	(*d_open)(struct file *fp, int rw);
	void	(*d_close)(struct file *fp, int rw);
	void	(*d_read)(dev_t dev);
	void	(*d_write)(dev_t dev);
	void	(*d_ioctl)(dev_t, int cmd, ...);
};

extern const struct cdevsw cdevsw[];

/*
 * Number of character switch entries.
 * Set by cinit/tty.c
 */
extern const int nchrdev;

#endif
