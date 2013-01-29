#ifndef DIR_H
#define DIR_H

/**
 * Entry for each file in the fs, the entire table resides in the itable which
 * is at block number 2 and 3
 */
struct testfs_dir_entry {
	__le32 inode_number;	/* Inode number */
	__le32 name_len;	/* File name length */
	char name[20];		/* File name */
	__u8 type;		/* DT_DIR or DT_REG */
};

extern const struct file_operations testfs_dir_fops;
extern const struct inode_operations testfs_dir_iops;

#endif

