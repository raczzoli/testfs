#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/slab.h>

#include "testfs.h"
#include "super.h"
#include "inode.h"


// fill super
static int fill_super(struct super_block *sb, void *data, int silent);

// super operations
static void put_super(struct super_block *);


static struct super_operations testfs_super_ops = {
	.put_super 	= put_super
};


struct dentry *super_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, fill_super);
}


static int fill_super(struct super_block *sb, void *data, int silent)
{
	struct buffer_head *bh 	= NULL;
	struct inode *root  = NULL;
	struct testfs_superblock *testfs_sb = NULL;
	int ret = -1;
	
        if (!(bh = sb_bread(sb, TESTFS_SUPER_BLOCK_NUM)))
	{
        	printk(KERN_INFO "testfs: unable to read superblock.\n");
               	goto err;
       	}
	printk(KERN_INFO "testfs: superblock read successfuly.\n");

	testfs_sb = kmalloc(sizeof(*testfs_sb), GFP_KERNEL);
	if (!testfs_sb) {
		printk(KERN_INFO "testfs: unable to read superblock.\n");
		goto err;
	}
	memcpy(testfs_sb, bh->b_data, sizeof(*testfs_sb));

	printk(KERN_INFO "testfs: magic=%d block_size=%d "
		"block_count=%d itable=%d itable_size=%d block_bitmap=%d\n",
		testfs_sb->magic, testfs_sb->block_size, testfs_sb->block_count,
		testfs_sb->itable, testfs_sb->itable_size, testfs_sb->block_bitmap);

	/* Check whether valid testfs */
	if (testfs_sb->magic != TESTFS_MAGIC_NUM) {
		printk(KERN_ERR "testfs: not a testfs filesystem\n");
		goto err;
	}

	/* TODO: Use a in-memory structure */
	sb->s_fs_info		= testfs_sb;
	sb->s_blocksize 	= testfs_sb->block_size;
	sb->s_blocksize_bits 	= 10;
	sb->s_magic		= TESTFS_MAGIC_NUM;
	sb->s_op		= &testfs_super_ops;

	root = inode_iget(sb, TESTFS_ROOT_INODE_NUM);

	if (!root) 
	{
		printk(KERN_INFO "testfs: inode_iget failed in fill_super!\n");
		goto err;	
	}

	sb->s_root = d_make_root(root);	

	return 0;

err:
	if (bh)
	 	brelse(bh);
	if (root)
        	iput(root);
	if (testfs_sb)
		kfree(testfs_sb);

	return ret;
}


static void put_super (struct super_block *sb)
{
	if (sb->s_fs_info) {
		kfree(sb->s_fs_info);
	}
}

