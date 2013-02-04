#ifndef BITMAP_H
#define BITMAP_H

#include <linux/fs.h>

/*
 * searches for the next free data block, stores the found block in block_num,
 * marks it as used, and returns
 */
inline int bitmap_get_free_data_block_num(struct super_block *sb, int *block_num);

/*
 * searches for the next free inode, stores the found number in inode_num,
 * marks it as used, and returns
 */
inline int bitmap_get_free_inode_num(struct super_block *sb, int *inode_num);


/*
 * marks the passed block_num as free in the data block bitmap
 */
inline int bitmap_free_data_block_num(struct super_block *sb, int block_num);


/*
 * marks the passed inode_num as free in the inode block bitmap
 */
inline int bitmap_free_inode_num(struct super_block *sb, int inode_num);


#endif /* BITMAP_H */
