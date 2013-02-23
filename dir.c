#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>


#include "testfs.h"
#include "dir.h"
#include "super.h"
#include "inode.h"
#include "bitmap.h"

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
			printk(KERN_INFO "testfs: lookup: invalid data block number for directory entry %s.\n", dentry->d_name.name);
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
	return 0;
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

static int testfs_mkdir(struct inode *parent_dir, struct dentry *dentry, umode_t mode)
{
	int data_block_num	= 0;
	struct inode *new_dir 	= NULL;
	struct buffer_head *bh 	= NULL;
	struct buffer_head *new_dir_bh = NULL;
	int free_inode_found	= 0;
	struct testfs_dir_entry *raw_dentry = NULL;

	// request new inode
	new_dir = inode_get_new_inode(parent_dir->i_sb, S_IFDIR | mode);

	if (!new_dir) 
		return -ENOSPC;

	d_instantiate(dentry, parent_dir);	

	data_block_num = inode_get_data_block_num(parent_dir);	
	if (data_block_num < 4) {
                printk(KERN_INFO "testfs: mkdir(parent dir): invalid data block number for directory entry %s.\n", dentry->d_name.name);
                return -EIO;
        }

	if (!(bh = sb_bread(parent_dir->i_sb, data_block_num))) {
                printk(KERN_INFO "testfs: error reading data block number %d from disk\n", data_block_num);
                return -EIO;
        }
	
	data_block_num = inode_get_data_block_num(new_dir);
	if (data_block_num < 4) {
                printk(KERN_INFO "testfs: mkdir(new dir): invalid data block number for directory entry %s.\n", dentry->d_name.name);
		brelse(bh);
                return -EIO;
        }

	if (!(new_dir_bh = sb_bread(new_dir->i_sb, data_block_num))) {
                printk(KERN_INFO "testfs: error reading data block number %d from disk\n", data_block_num);
		brelse(bh);
                return -EIO;
        }
	
        raw_dentry = (struct testfs_dir_entry *)bh->b_data;
        for ( ; ((char*)raw_dentry) < ((char*)bh->b_data) + TESTFS_GET_BLOCK_SIZE(parent_dir->i_sb); raw_dentry++) {
		if (raw_dentry->inode_number == 0) {
			//we found an empty inode
			free_inode_found = 1;
			break;
		}
        }
		
	if (!free_inode_found) {
		brelse(new_dir_bh);
		brelse(bh);
		return -ENOSPC;
	}

	raw_dentry->inode_number = cpu_to_le32(new_dir->i_ino);
	raw_dentry->type	 = 1;

	raw_dentry->name_len	= dentry->d_name.len;
	memcpy(raw_dentry->name, dentry->d_name.name, dentry->d_name.len);

	printk(KERN_INFO "testfs: raw_dentry name: %s, length: %d\n", raw_dentry->name, raw_dentry->name_len);

	((struct testfs_inode *)parent_dir->i_private)->i_size 	+= sizeof(struct testfs_dir_entry);
	((struct testfs_inode *)new_dir->i_private)->i_size 	= sizeof(struct testfs_dir_entry) * 2;
	((struct testfs_inode *)new_dir->i_private)->block_ptr	= data_block_num;

	raw_dentry = (struct testfs_dir_entry *)new_dir_bh->b_data;
	memcpy(raw_dentry->name, ".", 1);
	raw_dentry->name_len 	= 1;
	raw_dentry->type	= 1;
	raw_dentry->inode_number = new_dir->i_ino;
	raw_dentry++;
	
	memcpy(raw_dentry->name, "..", 2);
	raw_dentry->name_len 		= 2;
	raw_dentry->type		= 1;
	raw_dentry->inode_number 	= parent_dir->i_ino;

	mark_buffer_dirty(new_dir_bh);
	mark_inode_dirty(new_dir);

	mark_buffer_dirty(bh);
	mark_inode_dirty(parent_dir);

	fsync_bdev(parent_dir->i_sb->s_bdev);

	brelse(bh);
	brelse(new_dir_bh);
	return 0;
}

static int testfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	int data_block_num 			= inode_get_data_block_num(dir);
	struct testfs_dir_entry *raw_dentry 	= NULL;
	struct buffer_head *bh			= NULL;
	unsigned char *buffer;
	int bufptr = 0;		/* Buffer to hold the dir data block minus the directory name */

	buffer = (unsigned char *)kzalloc(TESTFS_GET_BLOCK_SIZE(dir->i_sb), GFP_KERNEL);
	if (!buffer) {
		printk(KERN_INFO "testfs: failed to allocate temporary buffer\n");
		return -EIO;
	}

	if (data_block_num < 4) {
		printk(KERN_INFO "testfs: rmdir: invalid data block number for directory entry %s.\n", dentry->d_name.name);
		return -EIO;
	}

	if (!(bh = sb_bread(dir->i_sb, data_block_num))) {
		printk(KERN_INFO "testfs: error reading data block number %d from disk\n", data_block_num);
		return -EIO;
	}

	bufptr = 0;
	raw_dentry = (struct testfs_dir_entry *)bh->b_data;
	for ( ; ((char*)raw_dentry) < ((char*)bh->b_data) + TESTFS_GET_BLOCK_SIZE(dir->i_sb); raw_dentry++) {
		if (strcmp(dentry->d_name.name, raw_dentry->name) == 0) {
			bitmap_free_inode_num(dir->i_sb, raw_dentry->inode_number);
			raw_dentry->inode_number = 0;
			memset(raw_dentry->name,0x00,raw_dentry->name_len);
			raw_dentry->name_len = 0;

			((struct testfs_inode *)dir->i_private)->i_size -= sizeof(struct testfs_dir_entry);             

	                mark_inode_dirty(dir);
        	        mark_buffer_dirty(bh);
        	        fsync_bdev(dir->i_sb->s_bdev);
		}
	}

	brelse(bh);
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
	


	return 0;
}

/*
 * for the moment it only lists entries within one block. 
 * if a directory contains so many entries, that multiple data blocks are needed
 * only the contents of the first block will be read
 */
static int testfs_readdir(struct file * fp, void * dirent, filldir_t filldir)
{
	struct buffer_head *bh  		= NULL;
	struct inode *dir 			= fp->f_dentry->d_inode;
	struct super_block* sb 			= dir->i_sb;
	int data_block_num			= 0;
	int isize				= 0;
	struct testfs_dir_entry *raw_dentry 	= NULL;

	/*
	 * data block number containing entries for the current directory 
	 */
	isize = inode_get_size(dir);

	if (fp->f_pos >= isize) {
		return 0;
	}

	data_block_num = inode_get_data_block_num(dir);

	/* something like this should never happen. if the inodes are properly written to disk
	 * block_ptr will point to a valid data block
	 */
	if (data_block_num < 4) {
		printk(KERN_INFO "testfs: readdir: invalid data block number for directory entry %s.\n", fp->f_dentry->d_name.name);
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
		printk(KERN_INFO "testfs: listing: %s\n", raw_dentry->name);
		if (raw_dentry->inode_number > 0)
		{
			filldir(dirent, raw_dentry->name, raw_dentry->name_len, fp->f_pos, raw_dentry->inode_number, raw_dentry->type);
			fp->f_pos += sizeof(struct testfs_dir_entry);
		}
		raw_dentry++;
	}

	return 0;
}

static int testfs_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
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

