#ifndef INODE_H
#define INODE_H

#include <linux/fs.h>


/* On disk inode structure */
struct testfs_inode {
	__le16 i_mode;		/* Mode */
	__le16 i_size;		/* Size */
	__le32 group;		/* Block group */
	__le32 block_ptr;	/* Pointer to data block */
};

struct testfs_group_desc {
        __le32 block_bitmap;
        __le32 inode_bitmap;
        __le32 inode_table;
};

/* Inode memory and on disk locations */
struct testfs_iloc {
	struct buffer_head *bh;
	u32 group;
	u32 block_num;
	u32 offset;
	u32 ino;
};

struct inode *inode_iget(struct super_block *sb, u32 ino);

struct inode *inode_get_new_inode(struct inode *dir, umode_t mode, int alloc_data_block);

int inode_alloc_data_block(struct super_block *sb, struct inode *inode);
int inode_write_inode(struct inode *inode, struct writeback_control *wbc);

int inode_get_data_block_num(struct inode *inode);
int inode_get_size(struct inode *inode);

#endif /* INODE_H */
