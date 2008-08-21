/* from v7 */
/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
struct mount
{
	dev_t   m_dev;          /* device mounted */
	struct filsys *m_filsys;     /* pointer to superblock */
	struct inode *m_inodp;  /* pointer to mounted on inode */
} mount[NMOUNT];

#define EACHMOUNT(mp) (mp = &mount[0]; mp < &mount[NMOUNT]; ++mp)
