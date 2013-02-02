#include <linux/fs.h>
#include <linux/buffer_head.h>

#include "testfs.h"
#include "dir.h"
#include "super.h"
#include "inode.h"

static int testfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static struct dentry *testfs_lookup(struct inode *dir, struct dentry *dentry,
		unsigned int flags)
{
	int data_block_num 			= inode_get_data_block_num(dir);
	struct testfs_dir_entry *raw_dentry 	= NULL;
	struct buffer_head *bh			= NULL;
	struct inode *found_inode		= NULL;


	if (data_block_num < 4) {
			printk(KERN_INFO "testfs: invalid data block number for directory entry.\n");
			return ERR_PTR(-EIO);
	}

	if (!(bh = sb_bread(dir->i_sb, data_block_num))) {
			printk(KERN_INFO "testfs: error reading data block number %d from disk\n", data_block_num);
			return ERR_PTR(-EIO);
	}	

	raw_dentry = (struct testfs_dir_entry *)bh->b_data;	
	for ( ; ((char*)raw_dentry) < ((char*)bh->b_data) + TESTFS_GET_BLOCK_SIZE(dir->i_sb); raw_dentry++) 
	{
		/* if the two lengths are not equal, it means the current dentry
		 * is not the one we are looking for, we can go to the next one
		 */
		if (raw_dentry->name_len != dentry->d_name.len) 
			continue;
		
		if (memcmp(raw_dentry->name, dentry->d_name.name, dentry->d_name.len) != 0)
			continue;
				
		brelse(bh);
		
		found_inode = inode_iget(dir->i_sb, le32_to_cpu(raw_dentry->inode_number));
		d_add(dentry, found_inode);		
		
		return 0;
	}
	
	brelse(bh);
	return ERR_PTR(-EIO);
}

static int testfs_link(struct dentry *old_dentry, struct inode *dir,
		struct dentry *dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_unlink(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_symlink(struct inode *dir, struct dentry *dentry,
		const char *symname)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
		dev_t rdev)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_rename(struct inode *old_dir, struct dentry *old_dentry,
		struct inode *new_dir, struct dentry *new_dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

/*
 * for the moment it only lists entries within one block. 
 * if a directory contains so many entries, that multiple data blocks are needed
 * only the contents of the first block will be read
 */
static int testfs_readdir(struct file * fp, void * dirent, filldir_t filldir)
{
	struct buffer_head *bh  	= NULL;
	struct inode *dir 			= fp->f_dentry->d_inode;
	struct super_block* sb 		= dir->i_sb;
	int data_block_num			= 0;
	int isize					= 0;
	struct testfs_dir_entry *raw_dentry 	= NULL;
	
	/*
	 * data block number containing entries for the current directory 
	 */
	data_block_num = inode_get_data_block_num(dir);
	isize = inode_get_size(dir);

	if (fp->f_pos >= isize) {
		return 0;
	}

	/* something like this should never happen. if the inodes are properly written to disk
	 * block_ptr will point to a valid data block
	 */
	if (data_block_num < 4) {
		printk(KERN_INFO "testfs: invalid data block number for directory entry.\n");
		return 0;
	}

        if (!(bh = sb_bread(sb, data_block_num))) {
                printk(KERN_INFO "testfs: error reading data block number %d from disk\n", data_block_num);
                return -EIO;
        }

	/*
	 * because the first two entries (. and ..) are not kept on the disk
	 * we have to decrement fp->f_pos by 2. After we start listing entries from 
	 * the disk, fp->f_pos will be incremented by sizeof(struct testfs_dir_entry);
	 */
	raw_dentry = (struct testfs_dir_entry *)bh->b_data;

	while (fp->f_pos < isize) {
		filldir(dirent, raw_dentry->name, raw_dentry->name_len, fp->f_pos, raw_dentry->inode_number, raw_dentry->type);
		fp->f_pos += sizeof(struct testfs_dir_entry);
		raw_dentry++;
	}

	printk(KERN_INFO "testfs: fpos2=%lu\n", (unsigned long)fp->f_pos);

	return 0;
}

static int testfs_release(struct inode *inode, struct file *filp)
{
//	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

/* dir file opertions */
const struct file_operations testfs_dir_fops = {
	.read		= generic_read_dir,
	.readdir	= testfs_readdir,
	.release	= testfs_release,
};

/* dir inode operations */
const struct inode_operations testfs_dir_iops = {
	.create		= testfs_create,
	.lookup		= testfs_lookup,
	.link		= testfs_link,
	.unlink		= testfs_unlink,
	.symlink	= testfs_symlink,
	.mkdir		= testfs_mkdir,
	.rmdir		= testfs_rmdir,
	.mknod		= testfs_mknod,
	.rename		= testfs_rename,
};


