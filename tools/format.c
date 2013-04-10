#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define BLK_SIZE 		4096
#define NUM_INODES		(BLK_SIZE * 8)
#define ITABLE_SIZE		(NUM_INODES * sizeof(struct testfs_inode))
#define ITABLE_NUM_BLKS 	(ITABLE_SIZE / BLK_SIZE)
#define DATA_BLKS_SIZE		(BLK_SIZE * 8 * BLK_SIZE)
#define BLK_GRP_SIZE 		((BLK_SIZE * 3) + ITABLE_SIZE + DATA_BLKS_SIZE)
#define BLK_GRP_NUM_BLKS 	(BLK_GRP_SIZE / BLK_SIZE)



/* Commands :
 * $dd if=/dev/zero of=loopback.img bs=1024 count=204800
 * $losetup /dev/loop0 loopback.img
 * $format /dev/loop0
 */
int fd;
uint64_t disk_size = 0;

struct testfs_superblock {
	uint32_t magic;		/* Magic number */
	uint32_t block_size;	/* Block size */
	uint32_t block_count;	/* Number of blocks. Max is 32768 blocks */
	uint32_t group_count;	/* Number of block groups  */
	uint32_t blocks_per_group; /* Number of blocks in group */
	uint32_t inodes_per_group; /* Number of inodes in group */
	//uint32_t itable;	/* Block number of inode table */
	//uint32_t itable_size;	/* Size in blocks of inode table */
	//uint32_t block_bitmap;	/* Location of block usage bitmap */
	//uint32_t inode_bitmap;	/* Location of inode bitmap */
	uint32_t rootdir_inode;	/* Inode number of the root directory */
};

struct testfs_group_desc {
	uint32_t block_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint32_t first_data_block;
};

struct testfs_inode {
	uint16_t i_mode;
	uint16_t i_size;
	uint32_t group;
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

	int num_groups, i = 0;
	uint64_t write_pos = 0;

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

	printf("Size of the disk in MB:");
        scanf("%llu", &disk_size);

        disk_size *= 1024 * 1024;
	
	num_groups = (int)(disk_size / (uint64_t)BLK_GRP_SIZE);

	for (i=0;i<num_groups;i++) {
		write_pos = (uint64_t)i * (uint64_t)BLK_GRP_SIZE;
		if (write_super(num_groups, write_pos) < 0) {
			goto err;
		}

		if (write_group_descriptors(write_pos, i) < 0) {
			goto err;
		}

		if (write_block_bitmap(write_pos, i) < 0) {
			goto err;
		}

		if (write_inode_bitmap(write_pos, i) < 0) {
			goto err;
		}

		if (write_itable(write_pos, i) < 0) {
			goto err;
		}

		/*
 		 * we write the . and .. entries to disk (only for the root inode)
 		 */
		if (i==0 && write_root_dir() < 0) {
			goto err;
		}
	}

	close(fd);
	exit(EXIT_SUCCESS);
err:
	close(fd);
	exit(EXIT_FAILURE);
}

int write_super(uint32_t num_groups, uint64_t write_pos)
{
	struct testfs_superblock sb;
	
	sb.magic		= 0x1012F4DD;
	sb.block_size		= BLK_SIZE;
	sb.block_count		= 0; // not used
	sb.group_count		= num_groups;
	sb.blocks_per_group 	= (uint32_t)BLK_GRP_NUM_BLKS;
	sb.inodes_per_group	= (uint32_t)NUM_INODES;
	//sb.itable	= 4;
	//sb.itable_size	= ITABLE_NUM_BLKS;	
	//sb.block_bitmap = 2;
	//sb.inode_bitmap = 3;
	sb.rootdir_inode = 1;

	/* Seek to block 0 */
	if (lseek64(fd, write_pos + (uint64_t)(0 * BLK_SIZE), 0) < (uint64_t)0) {
		perror("Failed to seek to superblock");
		return -1;
	}

	/* Write superblock */
	write(fd, &sb, sizeof(sb));
	printf("Wrote superblock : %lu bytes\n", sizeof(sb));

	return 0;
}

int write_group_descriptors(uint64_t write_pos, int group)
{
	struct testfs_group_desc desc;
	uint32_t start_pos = group * BLK_GRP_NUM_BLKS;

	desc.block_bitmap 	= start_pos + 2;
	desc.inode_bitmap 	= start_pos + 3;
	desc.inode_table  	= start_pos + 4;
	desc.first_data_block 	= start_pos + 4 + ITABLE_NUM_BLKS;

	if (lseek64(fd, write_pos + (uint64_t)(1 * BLK_SIZE), 0) < (uint64_t)0) {
                perror("Failed to seek to group descriptor table: ");
                return -1;
        }

        write(fd, &desc, sizeof(desc));
	printf("Wrote block group descriptor : %lu bytes\n", sizeof(desc));	

	return 0;
}

int write_block_bitmap(uint64_t write_pos, int group)
{
	unsigned char bitmap[BLK_SIZE] = {0};
	int c;

	/* Resetting bitmap to dummy data */
	for (c = 0; c < sizeof(bitmap); c++) {
		bitmap[c] = 0x00;
	}

	
	if (group == 0)
		bitmap[0] = 0x1;

	/* Seek to block 1 */
	if (lseek64(fd, write_pos + (uint64_t)(2 * BLK_SIZE), 0) < (uint64_t)0) {
		perror("Failed to seek to block bitmap: ");
		return -1;
	}

	/* Write block bitmap */
	write(fd, bitmap, sizeof(bitmap));
	printf("Wrote block bitmap : %lu bytes\n", sizeof(bitmap));
}

int write_inode_bitmap(uint64_t write_pos, int group)
{
	unsigned char bitmap[BLK_SIZE] = {0};
	int c;

	/* Resetting bitmap to dummy data */
	for (c = 0; c < sizeof(bitmap); c++) {
		bitmap[c] = 0x00;
	}

	
	if (group == 0)
		bitmap[0] = 0x3;
	else
		bitmap[0] = 0x1;


	/* Seek to block 2 */
	if (lseek64(fd, write_pos + (uint64_t)(3 * BLK_SIZE), 0) < (uint64_t)0) {
		perror("Failed to seek to inode bitmap");
		return -1;
	}

	/* Write inode bitmap */
	write(fd, bitmap, sizeof(bitmap));
	printf("Wrote inode bitmap : %lu bytes\n", sizeof(bitmap));
}

int write_itable(uint64_t write_pos, int group)
{
	struct testfs_inode itable[NUM_INODES] = {0};
	int c;

	/* Resetting itable with dummy data */
	for (c = 0; c < NUM_INODES; c++) {
		if (c == 1 && group == 0) {
			itable[c].i_mode 	= 0x41FF;
			itable[c].i_size 	= 2 * sizeof(struct testfs_dir_entry);
			itable[c].group		= 0;
			itable[c].block_ptr 	= ITABLE_NUM_BLKS + 4;
			continue;
		}
		itable[c].i_mode 	= 0x41FF;	/* Mode = Dir */
		itable[c].i_size 	= 0;		/* Size */
		itable[c].group		= group;	/* Block group */
		itable[c].block_ptr 	= 0;		/* Block Pointer (relative to group) */
	}

	/* Seek to block 3 */
	if (lseek64(fd, write_pos + (uint64_t)(4 * BLK_SIZE), 0) < (uint64_t)0) {
		perror("Failed to seek to itable");
		return -1;
	}

	/* Write inode table */
	write(fd, itable, sizeof(itable));
	printf("Wrote itable : %lu bytes\n", sizeof(itable));
}

/*
 * only called once, when writing data in the first group
 * */
int write_root_dir(void)
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

	/* Seek to block 6 (1 + 6) */
	if (lseek(fd, (4 + ITABLE_NUM_BLKS) * BLK_SIZE, 0) < 0) {
		perror("Failed to seek to root directory data block");
		return -1;
	}

	/* Write root directory data block */
	write(fd, root, sizeof(root));
	printf("Wrote root directory data : %lu bytes\n", sizeof(root));
}
