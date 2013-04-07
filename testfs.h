#ifndef TESTFS_H
#define TESTFS_H

#define TESTFS_MAGIC_NUM  	0x1012F4DD
#define TESTFS_SUPER_BLOCK_NUM	0
#define TESTFS_ROOT_INODE_NUM   1

#define TESTFS_GET_BLOCK_SIZE(sb)	(sb->s_blocksize)
#define TESTFS_GET_INODE(inode)		((struct testfs_inode *)inode->i_private)
#define TESTFS_GET_SB_INFO(sb)		((struct testfs_info *)sb->s_fs_info)
#define TESTFS_GET_SB(s)		((struct testfs_superblock *)TESTFS_GET_SB_INFO(s)->sb)

#define TESTFS_INODES_PER_GROUP(sb)	(TESTFS_GET_SB(sb)->inodes_per_group)

#endif /* TESTFS_H */
