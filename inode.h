#ifndef INODE_H
#define INODE_H

#include <linux/fs.h>


/* dir.c */
extern const struct file_operations testfs_dir_fops;
extern const struct inode_operations testfs_dir_iops;


/* file.c */
extern const struct file_operations testfs_file_fops;
extern const struct inode_operations testfs_file_iops;

/* On disk inode structure */
struct testfs_inode {
	__le16 i_mode;		/* Mode */
	__le16 i_size;		/* Size */
	__le32 block_ptr;	/* Pointer to data block */
};

/* Inode memory and on disk locations */
struct testfs_iloc {
	struct buffer_head *bh;
	u32 block_num;
	u32 offset;
	u32 ino;
};

struct inode *inode_iget(struct super_block *sb, u32 ino);

#endif /* INODE_H */
