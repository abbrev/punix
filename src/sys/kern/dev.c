#include <string.h>
#include <errno.h>

#include "punix.h"
#include "proc.h"
#include "queue.h"
#include "buf.h"
#include "dev.h"
#include "inode.h"
#include "globals.h"

STARTUP(void nodev())
{
	P.p_error = ENODEV;
}

STARTUP(void nulldev())
{
}

/* FIXME: move these to their appropriate home */
extern void flopen(struct file *, int);
extern void flclose(struct file *, int);
extern void flstrategy(struct buf *);
extern const struct devtab fltab;

const struct bdevsw bdevsw[] = {
	{ flopen, flclose, flstrategy, &fltab },
	{ 0, 0, 0, 0 }
};

const int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]) - 1;

void vtopen(struct file *, int), vtclose(struct file *, int);
void vtread(dev_t), vtwrite(dev_t);
void vtioctl(dev_t, int, ...);

void devttyopen(struct file *fp, int), devttyclose(struct file *fp, int);
void devttyread(dev_t), devttywrite(dev_t);
void devttyioctl(dev_t, int, ...);

void linkopen(struct file *fp, int), linkclose(struct file *fp, int);
void linkread(dev_t), linkwrite(dev_t);
void linkioctl(dev_t, int, ...);

void audioopen(struct file *fp, int), audioclose(struct file *fp, int);
void audioread(dev_t), audiowrite(dev_t);
void audioioctl(dev_t, int, ...);

void miscopen(struct file *fp, int), miscclose(struct file *fp, int);
void miscread(dev_t), miscwrite(dev_t);
void miscioctl(dev_t, int, ...);

void devfdopen(struct file *fp, int rw);

/* eventually put the following devices into cdevsw:
 * USB port (for HW3)
 */
const struct cdevsw cdevsw[] = {
{ miscopen,   nulldev,     miscread,   miscwrite,   miscioctl   }, /* misc */
#if 0
{ vtopen,     vtclose,     vtread,     vtwrite,     vtioctl     }, /* vt */
#endif
{ devttyopen, devttyclose, devttyread, devttywrite, devttyioctl }, /* tty */
{ linkopen,   linkclose,   linkread,   linkwrite,   linkioctl   }, /* link */
{ audioopen,  audioclose,  audioread,  nulldev,     audioioctl  }, /* audio */
{ devfdopen,  nulldev,     nulldev,    nulldev,     nulldev     }, /* fd */
{ 0, 0, 0, 0, 0 }
};

const int nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]) - 1;
