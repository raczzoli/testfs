#include <linux/buffer_head.h>

#include "inode.h"
#include "super.h"

/*
 * reads the inode from the disk by inode number
 */
static int read_inode(struct super_block *sb, struct testfs_iloc *iloc,
		struct testfs_inode *raw_inode);

struct inode *inode_iget(struct super_block *sb, u32 ino)
{
	struct inode *inode;
	struct testfs_iloc iloc;
	struct testfs_inode raw_inode;
	struct testfs_info *testfs_i;
	int ret = -EIO;

	inode = iget_locked(sb, ino);
	if (!inode) {
		return ERR_PTR(-ENOMEM);
	}
	if (!(inode->i_state & I_NEW)) {
		/* if inode found return it */
		return inode;
	}

	/* Calculate inode block number and offset */
	testfs_i = (struct testfs_info *)sb->s_fs_info;
	iloc.block_num = le32_to_cpu(testfs_i->sb->itable) +
		((ino * sizeof(struct testfs_inode)) / sb->s_blocksize);
	iloc.offset = (ino * sizeof(struct testfs_inode)) % sb->s_blocksize;
	iloc.ino = ino;
	printk(KERN_INFO "testfs: inode block_num=%d offset=%d ino=%d\n",
		iloc.block_num, iloc.offset, iloc.ino);

	/* Read raw inode block from disk */
	ret = read_inode(sb, &iloc, &raw_inode);
	if (ret) {
		goto err_read_inode;
	}

	/* Initialize inode */
	inode->i_mode = le16_to_cpu(raw_inode.i_mode);
	inode->i_size = le16_to_cpu(raw_inode.i_size);
	i_uid_write(inode, 0);
	i_gid_write(inode, 0);
	inode->i_atime.tv_sec = (signed)le32_to_cpu(0);
	inode->i_ctime.tv_sec = (signed)le32_to_cpu(0);
	inode->i_mtime.tv_sec = (signed)le32_to_cpu(0);
	inode->i_atime.tv_nsec = inode->i_ctime.tv_nsec = inode->i_mtime.tv_nsec = 0;
	printk(KERN_INFO "testfs: inode i_mode=%d i_size=%d\n",
			inode->i_mode, (unsigned int)inode->i_size);

	if (S_ISDIR(inode->i_mode))		/* 16384 */
	{
		printk(KERN_INFO "testfs: inode=dir\n");
		inode->i_op     = &testfs_dir_iops; // set the inode ops	
		inode->i_fop    = &testfs_dir_fops;
	}
	else if (S_ISREG(inode->i_mode))	/* 32768 */
	{
		printk(KERN_INFO "testfs: inode=file\n");
		inode->i_op     = &testfs_file_iops; // set the inode ops
                inode->i_fop    = &testfs_file_fops;
	}

	unlock_new_inode(inode);

	return inode;

err_read_inode:
	brelse(iloc.bh);
	return NULL;
}


static int read_inode(struct super_block *sb, struct testfs_iloc *iloc,
		struct testfs_inode *raw_inode)
{
	/* Read block from disk */
	if (!(iloc->bh = sb_bread(sb, iloc->block_num))) {
		printk(KERN_INFO "testfs: error reading inode number %d from disk\n", iloc->ino);
		return -EIO;
	}

	/* Copy inode structure from disk */
	memcpy(raw_inode, iloc->bh->b_data + iloc->offset, sizeof(*raw_inode));
	return 0;
}

