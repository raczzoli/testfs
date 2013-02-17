#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

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
	uint32_t inode_bitmap;	/* Location of inode bitmap */
	uint32_t rootdir_inode;	/* Inode number of the root directory */
};

struct testfs_inode {
	uint16_t i_mode;
	uint16_t i_size;
	uint32_t block_ptr;
};

struct testfs_dir_entry {
	uint32_t inode_number;	/* Inode number */
	uint32_t name_len;	/* File name length */
	char name[20];		/* File name */
	uint8_t type;		/* DT_DIR or DT_REG */
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

	if (write_block_bitmap() < 0) {
		goto err;
	}

	if (write_inode_bitmap() < 0) {
		goto err;
	}

	if (write_itable() < 0) {
		goto err;
	}

	if (write_rootdir() < 0) {
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

	sb.magic		= 0x1012F4DD;
	sb.block_size	= 4096;
	sb.block_count	= 32768;
	sb.itable	= 3;
	sb.itable_size	= 2;	
	sb.block_bitmap = 1;
	sb.inode_bitmap = 2;
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

int write_block_bitmap(void)
{
	unsigned char bitmap[4096] = {0};
	int c;

	/* Resetting bitmap to dummy data */
	for (c = 0; c < sizeof(bitmap); c++) {
		bitmap[c] = 0x00;
	}

	/* Marking 1st 5 blocks in use by superblock, block bitmap, inode bitmap, itable */
	
	/* zoli: If we want to mark the first X bytes as used, becuase of the big/little endian thing
	 * we need to invert the order of 1s and 0s. So if we want to mark the first 5 blocks as used
	 * we need to write it as 0xF8 = 11111000 not 0x1F = 00011111
	 */
	bitmap[0] = 0xF8;

	/* Seek to block 1 */
	if (lseek(fd, 1 * 4096, 0) < 0) {
		perror("Failed to seek to block bitmap");
		return -1;
	}

	/* Write block bitmap */
	write(fd, bitmap, sizeof(bitmap));
	printf("Wrote block bitmap : %lu bytes\n", sizeof(bitmap));
}

int write_inode_bitmap(void)
{
	unsigned char bitmap[4096] = {0};
	int c;

	/* Resetting bitmap to dummy data */
	for (c = 0; c < sizeof(bitmap); c++) {
		bitmap[c] = 0x00;
	}

	/* Marking 1st 4 inodes in use by ".","..", and the 3 test dirs */
	bitmap[0] = 0xF0;

	/* Seek to block 2 */
	if (lseek(fd, 2 * 4096, 0) < 0) {
		perror("Failed to seek to inode bitmap");
		return -1;
	}

	/* Write inode bitmap */
	write(fd, bitmap, sizeof(bitmap));
	printf("Wrote inode bitmap : %lu bytes\n", sizeof(bitmap));
}

int write_itable(void)
{
	struct testfs_inode itable[1024] = {0};
	int c;

	/* Resetting itable with dummy data */
	for (c = 1; c <= 1024; c++) {
		if (c == 1) {
			itable[c].i_mode 	= 0x41FF;
			itable[c].i_size 	= 2 * sizeof(struct testfs_dir_entry);
			itable[c].block_ptr 	= c + 4;
		}
		else {		
			itable[c].i_mode = 0x41FF;	/* Mode = Dir */
			itable[c].i_size = 0;		/* Size */
			itable[c].block_ptr = 0;	/* Block Pointer */
		}
	}

	/* Seek to block 3 */
	if (lseek(fd, 3 * 4096, 0) < 0) {
		perror("Failed to seek to itable");
		return -1;
	}

	/* Write inode table */
	write(fd, itable, sizeof(itable));
	printf("Wrote itable : %lu bytes\n", sizeof(itable));
}

int write_rootdir(void)
{
	struct testfs_dir_entry root[2];
	int c;

	root[0].inode_number = 1;	/* Inode number */
	root[0].name_len = 1;		/* Name length */
	root[0].name[0] = '.';
	root[0].name[1] = '\0';
	root[0].type = 1;		/* DT_DIR or DT_REG */

	root[1].inode_number = 1;	/* Inode number */
	root[1].name_len = 2;		/* Name length */
	root[1].name[0] = '.';
	root[1].name[1] = '.';
	root[1].name[2] = '\0';
	root[1].type = 1;		/* DT_DIR or DT_REG */

//	for (c = 2; c < 5; c++) {
//		root[c].inode_number = c;				/* Inode number */
//		snprintf(root[c].name, sizeof(root[c].name), "testdir%d", c);
//		root[c].name_len = (uint32_t)strlen(root[c].name);      /* Name length */
//		root[c].type = 1;		/* DT_DIR or DT_REG */
//	}

	/* Seek to block 6 (1 + 6) */
	if (lseek(fd, 5 * 4096, 0) < 0) {
		perror("Failed to seek to root directory data block");
		return -1;
	}

	/* Write root directory data block */
	write(fd, root, sizeof(root));
	printf("Wrote root directory data : %lu bytes\n", sizeof(root));
}

