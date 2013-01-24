#ifndef INODE_H
#define INODE_H

#include <linux/fs.h>


/* dir.c */
extern const struct file_operations testfs_dir_fops;
extern const struct inode_operations testfs_dir_iops;


/* file.c */
extern const struct file_operations testfs_file_fops;
extern const struct inode_operations testfs_file_iops;



struct testfs_inode {
	__le16  i_mode;
};


struct inode *inode_iget(struct super_block *sb, unsigned long ino);

#endif /* INODE_H */
