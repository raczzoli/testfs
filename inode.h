#ifndef INODE_H
#define INODE_H

#include <linux/fs.h>

struct inode *inode_iget(struct super_block *sb, unsigned long ino);

#endif /* INODE_H */
