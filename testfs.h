#ifndef TESTFS_H
#define TESTFS_H

#define TESTFS_MAGIC_NUM  	0x1012F4DD
#define TESTFS_SUPER_BLOCK_NUM	0
#define TESTFS_ROOT_INODE_NUM   1

#define TESTFS_GET_BLOCK_SIZE(sb)	(sb->s_blocksize)


#endif /* TESTFS_H */
