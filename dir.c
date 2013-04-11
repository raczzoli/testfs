#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/quotaops.h>

#include "testfs.h"
#include "dir.h"
#include "super.h"
#include "inode.h"


static int add_link(struct inode *parent_inode, struct inode *child_inode, struct dentry *dentry, int type)
{
	struct testfs_dir_entry *raw_dentry 	= NULL;
	struct buffer_head *bh 			= NULL;
	int free_inode_found			= 0;
	int data_block_num			= TESTFS_GET_INODE(parent_inode)->block_ptr;
		
	d_instantiate(dentry, child_inode);
	
	if (!(bh = sb_bread(parent_inode->i_sb, data_block_num))) {
                printk(KERN_INFO "testfs: error reading data block number %d from disk\n", data_block_num);
                return -EIO;
        }
	
	raw_dentry = (struct testfs_dir_entry *)bh->b_data;
        for ( ; ((char*)raw_dentry) < ((char*)bh->b_data) + TESTFS_GET_BLOCK_SIZE(parent_inode->i_sb); raw_dentry++) {
		if (raw_dentry->inode_number == 0) {
			//we found an empty inode
			free_inode_found = 1;
			break;
		}
        }
		
	if (!free_inode_found) {
		brelse(bh);
		return -ENOSPC;
	}

	raw_dentry->inode_number = cpu_to_le32(child_inode->i_ino);
	raw_dentry->type	 = type;

	raw_dentry->name_len	= dentry->d_name.len;
	memcpy(raw_dentry->name, dentry->d_name.name, dentry->d_name.len);

	((struct testfs_inode *)parent_inode->i_private)->i_size 	+= sizeof(struct testfs_dir_entry);
		
	mark_buffer_dirty(bh);
	mark_inode_dirty(parent_inode);
	
	brelse(bh);
	return 0;
}


static int testfs_create(struct inode *parent_dir, struct dentry *dentry,
		umode_t mode, bool excl)
{
	struct inode *new_ino = NULL;
	int err = 0;

	dquot_initialize(parent_dir);
	
	new_ino = inode_get_new_inode(parent_dir, mode, 0);

	if (!new_ino)
		return -ENOSPC;

	err = add_link(parent_dir, new_ino, dentry, DT_REG);	
	
	if (err != 0) {
		iput(new_ino);
		return err;
	}
	
	((struct testfs_inode *)new_ino->i_private)->i_size = 0;
	new_ino->i_size = 0;

	mark_inode_dirty(new_ino);
	unlock_new_inode(new_ino);


	fsync_bdev(parent_dir->i_sb->s_bdev);

	return 0;
}

static struct dentry *testfs_lookup(struct inode *dir, struct dentry *dentry,
		unsigned int flags)
{
	struct testfs_dir_entry *raw_dentry 	= NULL;
	struct buffer_head *bh			= NULL;
	struct inode *found_inode		= NULL;
	int data_block_num			= TESTFS_GET_INODE(dir)->block_ptr;

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
	struct inode *new_dir 			= NULL;
	struct buffer_head *new_dir_bh 		= NULL;
	struct testfs_dir_entry *raw_dentry 	= NULL;
	int err			= 0;
	struct testfs_inode *new_testfs_ino 	= NULL;	

	// request new inode
	new_dir = inode_get_new_inode(parent_dir, S_IFDIR | mode, 1);

	if (!new_dir) 
		return -ENOSPC;

	new_testfs_ino = TESTFS_GET_INODE(new_dir);
	
	err = add_link(parent_dir, new_dir, dentry, DT_DIR);
		
	if (err != 0) {
		iput(new_dir);
		return err;
	}
	
	if (!(new_dir_bh = sb_bread(new_dir->i_sb, new_testfs_ino->block_ptr))) {
                printk(KERN_INFO "testfs: error reading data block number %d from disk\n", new_testfs_ino->block_ptr);
                return -EIO;
        }

	/*
	 * we add the two . and .. directory entries to the inode`s datablock.
	 */
	new_testfs_ino->i_size 		= sizeof(struct testfs_dir_entry) * 2;
	
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

	fsync_bdev(parent_dir->i_sb->s_bdev);

	brelse(new_dir_bh);
	return 0;
}

static int testfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct testfs_dir_entry *raw_dentry 	= NULL;
	struct buffer_head *bh			= NULL;
	struct inode *child_dir			= dentry->d_inode;
	int data_block_num                      = TESTFS_GET_INODE(dir)->block_ptr;
	int ret					= 0;
	
	if (!(bh = sb_bread(dir->i_sb, data_block_num))) {
		printk(KERN_INFO "testfs: error reading data block number %d from disk\n", data_block_num);
		return -EIO;
	}

	raw_dentry = (struct testfs_dir_entry *)bh->b_data;
	for ( ; ((char*)raw_dentry) < ((char*)bh->b_data) + TESTFS_GET_BLOCK_SIZE(dir->i_sb); raw_dentry++) {
		if (strcmp(dentry->d_name.name, raw_dentry->name) == 0) {
			d_delete(dentry);
			
			//bitmap_free_inode_num(dir->i_sb, raw_dentry->inode_number);
			ret = inode_delete_inode(child_dir);			

			if (ret) {
				brelse(bh);
				return ret;
			}

			raw_dentry->inode_number = 0;
			memset(raw_dentry->name,0x00,raw_dentry->name_len);
			raw_dentry->name_len = 0;

			((struct testfs_inode *)dir->i_private)->i_size -= sizeof(struct testfs_dir_entry);             
			dir->i_size = ((struct testfs_inode *)dir->i_private)->i_size;
			
			((struct testfs_inode *)child_dir->i_private)->i_size = 0;
                        child_dir->i_size = 0;

	                mark_inode_dirty(dir);
			mark_inode_dirty(child_dir);

        	        mark_buffer_dirty(bh);
			iput(child_dir);
        	        fsync_bdev(dir->i_sb->s_bdev);
			break;
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
	int isize				= 0;
	struct testfs_dir_entry *raw_dentry 	= NULL;
	int data_block_num                      = TESTFS_GET_INODE(dir)->block_ptr;

	/*
	 * data block number containing entries for the current directory 
	 */
	isize = inode_get_size(dir);

	if (fp->f_pos >= isize) {
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
	.rename		= testfs_rename
};

