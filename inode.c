#include "inode.h"


/* file operations */
static struct file_operations dir_fops;

/* inode operations */
static struct inode_operations dir_iops;


struct inode *inode_iget(struct super_block *sb, unsigned long ino)
{
  struct inode *inode;
	inode = iget_locked(sb, ino);

	if (S_ISDIR(inode->i_mode))
	{
		inode->i_op 	= &dir_iops;
		inode->i_fop 	= &dir_fops;
	}

	return inode;
}

