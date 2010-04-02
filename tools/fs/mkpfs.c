/* mkpfs.c, make a PFS file system */

/*
this utility mimics some of the structures and
routines in the kernel for writing to a filesystems
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#include "../src/sys/h/list.h"
#include "buf.h"
#include "pfs.h"
#include "fs.h"

#define NUM_SECTORS 29 /* 32 - 2 (kernel) - 1 (floating) */
#define BLOCKS_PER_SECTOR 504
#define BLOCKS ((long)NUM_SECTORS * BLOCKS_PER_SECTOR)

char *argv0;
char *label = NULL;
int verbose = 0;

#if 0
struct list_head buflist;
#endif

static void usage(void)
{
	fprintf(stderr,
	        "Usage: %s [OPTIONS...] filename\n"
	        "Options:\n"
	        "    -h            show this help message and exit\n"
	        "    -L label      volume label\n"
	        "    -s blocks     size of filesystem in blocks (default: %ld)\n"
	        "    -v            be verbose\n",
	        argv0, blocks
	);
}

int main(int argc, char *argv[])
{
	FILE *filesystemfile = NULL;
	char *filename = NULL;
	int opt;
	unsigned long size;
	struct super_block sb;
	struct inode rootinode;
	argv0 = argv[0];
	
	bufinit();
	
	blockdev = NULL;
	blocks = BLOCKS;
	
	while ((opt = getopt(argc, argv, "hL:s:v")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			exit(0);
		case 'L':
			label = optarg;
			break;
		case 's':
			blocks = atol(optarg);
			if (blocks < 1) {
				fprintf(stderr, "%s: blocks must be greater than 0\n", argv0);
				exit(1);
			}
			break;
		case 'v':
			++verbose;
			break;
		default: /* '?' */
			usage();
			exit(1);
		}
	}
	
	if (optind + 1 != argc) {
		if (optind + 1 > argc)
			fprintf(stderr, "%s: missing filename\n", argv0);
		else
			fprintf(stderr, "%s: too many filenames given\n", argv0);
		usage();
		exit(1);
	}
	filename = argv[optind];
	
	if (!label) label = "";
	if (strlen(label) > 16) {
		fprintf(stderr, "%s: warning: truncating volume label to 16 characters\n", argv0);
		label[16] = '\0';
	}
	
	size = blocks * BLOCK_SIZE;
	blockdev = malloc(size);
	if (!blockdev) {
		fprintf(stderr, "%s: cannot allocate %ld bytes for filesystem\n", argv0, size);
		exit(1);
	}
	memset(blockdev, 0xff, size);
	
	filesystemfile = fopen(filename, "wb");
	if (!filesystemfile) {
		fprintf(stderr, "%s: cannot open \"%s\" for writing\n", argv0, filename);
		exit(1);
	}
	
	fprintf(stderr, "making filesystem...\n");
	
	sb.u.pfs_sb.s_version = 0;
	strncpy(sb.u.pfs_sb.s_label, label, 16);
	sb.u.pfs_sb.s_block_count = blocks;
	sb.u.pfs_sb.s_inode_count = 0; /* we'll add one later */
	sb.u.pfs_sb.s_rsvd_block_count = blocks * 5 / 100; /* 5% of total size */
	sb.u.pfs_sb.s_first_data_block = (blocks + (128 * 8)) / (128 * 8) + 1; /* block bitmap + sb */
	
	rootinode.i_sb = &sb;
	rootinode.i_ino = 1;
	rootinode.i_mode = 0050000 /* directory */ | 0755;  /* mode and type of file */
	rootinode.i_nlink = 0;     /* number of links to file */
	rootinode.i_uid = 0;       /* owner's user id */
	rootinode.i_gid = 0;       /* owner's group id */
	rootinode.i_size = 0;      /* number of bytes in file */
	rootinode.i_atime = 0; /* XXX */         /* time last accessed */
	rootinode.i_mtime = 0; /* XXX */         /* time last modified */
	rootinode.i_ctime = 0; /* XXX */         /* time last changed */
	
	pfs_write_super(&sb);
	pfs_write_inode(&rootinode);
	
	fwrite(blockdev, size, 1, filesystemfile);
	fclose(filesystemfile);

	return 0;
}
