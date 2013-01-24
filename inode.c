#include <linux/buffer_head.h>
#include "inode.h"


/*
 * reads the inode from the disk by inode number
 */
//static struct testfs_inode *read_inode(struct super_block *sb, unsigned long ino, struct buffer_head **bh_);


struct inode *inode_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
//	struct testfs_inode *raw_inode;
//	struct buffer_head *bh;
//	long ret = -EIO;

	inode 		= iget_locked(sb, ino);

	/*
	raw_inode 	= read_inode(sb, ino, &bh); 

	if (raw_inode == NULL)
	{
		ret = -EIO; 
		goto err_read_inode;
	}

	inode->i_mode = raw_inode->i_mode;
	*/

	// this needs to be replaced with raw_inode->i_mode, but until read_inode
	// works this hardcode lets us at least mount the file system
	inode->i_mode = S_IFDIR;

	if (S_ISDIR(inode->i_mode))
	{
		inode->i_op     = &testfs_dir_iops; // set the inode ops	
		inode->i_fop    = &testfs_dir_fops;
	}
	else if (S_ISREG(inode->i_mode))
	{
		inode->i_op     = &testfs_file_iops; // set the inode ops
                inode->i_fop    = &testfs_file_fops;
	}

	return inode;

//err_read_inode:
//	brelse(bh);

//	return ERR_PTR(ret);
}


/* block and b_offset needs to be calculated, but I`m too tired to do this now :(
static struct testfs_inode *read_inode(struct super_block *sb, unsigned long ino, struct buffer_head **bh_)
{
	struct buffer_head *bh;
	unsigned long block;
	unsigned long b_offset;
	block 	= 0; 
	*bh_ 	= NULL;

	if (!(bh = sb_bread(sb, block)))
	{
		printk(KERN_INFO "testfs: error reading inode block from disk... inode number: %lu.\n", ino);
		return NULL;
	}

	*bh_ = bh;

	return (struct testfs_inode *) (bh->b_data + b_offset);
}
*/
