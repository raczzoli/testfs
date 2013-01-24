#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

/* Commands :
 * $dd if=/dev/zero of=loopback.img bs=1024 count=204800
 * $losetup /dev/loop0 loopback.img
 * $format /dev/loop0
 */
int fd;

struct testfs_superblock {
	uint32_t magic;		/* Magic number */
	uint32_t block_size;	/* Block size */
	uint32_t block_count;	/* Number of blocks. Max is 32768 blocks */
	uint32_t itable;	/* Block number of inode table */
	uint32_t itable_size;	/* Size in blocks of inode table */
	uint32_t block_bitmap;	/* Location of block usage bitmap */
	uint32_t rootdir_inode;	/* Inode number of the root directory */
};

struct tesfs_dir_entry {
	uint32_t inode_number;	/* Inode number */
	uint32_t block_number;	/* Block number */
	uint32_t size;		/* File size */
	char name[20];		/* File name */
};

int main(int argc, char *argv[])
{
	/* Read file system name from parameters */
	if (argc != 2) {
		printf("\nUsage : format [/dev/sda1]\n\n");
		exit(EXIT_FAILURE);
	}

	/* Open file system */
	fd = open(argv[1], O_RDWR);
	if (fd <= 0) {
		perror("error opening filesystem");
		exit(EXIT_FAILURE);
	}

	if (write_super() < 0) {
		goto err;
	}

	if (write_bitmap() < 0) {
		goto err;
	}

	if (write_itable() < 0) {
		goto err;
	}

	close(fd);
	exit(EXIT_SUCCESS);
err:
	close(fd);
	exit(EXIT_FAILURE);
}

int write_super(void)
{
	struct testfs_superblock sb;

	sb.magic	= 0x1012F4DD;
	sb.block_size	= 4096;
	sb.block_count	= 32768;
	sb.itable	= 2;
	sb.itable_size	= 2;	
	sb.block_bitmap = 1;
	sb.rootdir_inode = 1;

	/* Seek to block 0 */
	if (lseek(fd, 0 * 4096, 0) < 0) {
		perror("Failed to seek to superblock");
		return -1;
	}

	/* Write superblock */
	write(fd, &sb, sizeof(sb));
	printf("Wrote superblock : %lu bytes\n", sizeof(sb));

	return 0;
}

int write_bitmap(void)
{
	unsigned char bitmap[4096] = {0};
	int c;

	/* Resetting bitmap to dummy data */
	for (c = 0; c < sizeof(bitmap); c++) {
		bitmap[c] = 0x00;
	}

	/* Marking 1st 4 blocks in use by superblock, bitmap, itable */
	bitmap[0] = 0x0F;

	/* Seek to block 1 */
	if (lseek(fd, 1 * 4096, 0) < 0) {
		perror("Failed to seek to block bitmap");
		return -1;
	}

	/* Write block bitmap */
	write(fd, bitmap, sizeof(bitmap));
	printf("Wrote block bitmap : %lu bytes\n", sizeof(bitmap));
}

int write_itable(void)
{
	struct tesfs_dir_entry itable[256] = {0};
	int c;

	/* Resetting itable with dummy data */
	for (c = 0; c < 256; c++) {
		itable[c].inode_number = c;	/* Inode number */
		itable[c].block_number = c + 4;	/* Block number */
		itable[c].size = c;		/* File size */
		itable[c].name[0] = 'F';	/* File name */
		itable[c].name[1] = 'S';	/* File name */
		itable[c].name[2] = '\0';	/* File name */
	}

	/* Seek to block 2 */
	if (lseek(fd, 2 * 4096, 0) < 0) {
		perror("Failed to seek to itable");
		return -1;
	}

	/* Write block bitmap */
	write(fd, itable, sizeof(itable));
	printf("Wrote itable : %lu bytes\n", sizeof(itable));
}

