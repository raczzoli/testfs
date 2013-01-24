#ifndef SUPER_H
#define SUPER_H

#include <linux/fs.h>

/* Testfs superblock */
struct testfs_superblock {
	__le32 magic;		/* Magic number */
	__le32 block_size;	/* Block size */
	__le32 block_count;	/* Number of blocks. Max is 32768 blocks */
	__le32 itable;		/* Block number of inode table */
	__le32 itable_size;	/* Size in blocks of inode table */
	__le32 block_bitmap;	/* Location of block usage bitmap */
	__le32 rootdir_inode;	/* Inode number of the root directory */
};

/**
 * Entry for each file in the fs, the entire table resides in the itable which
 * is at block number 2 and 3
 */
struct testfs_dir_entry {
	__le32 inode_number;	/* Inode number */
	__le32 block_number;	/* Block number */
	__le32 size;		/* File size */
	char name[20];		/* File name */
};

struct dentry *super_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data);

#endif /* SUPER_H */
