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
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return mount_bdev(fs_type, flags, dev_name, data, fill_super);
}


static int fill_super(struct super_block *sb, void *data, int silent)
{
	struct buffer_head *bh 	= NULL;
	struct inode *root  = NULL;
	struct testfs_superblock *testfs_sb = NULL;
	struct testfs_info *testfs_i = NULL;
	int ret = -1;
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);

    if (!(bh = sb_bread(sb, TESTFS_SUPER_BLOCK_NUM)))
	{
		printk(KERN_ERR "testfs: unable to read superblock.\n");
		goto err;
	}
	printk(KERN_INFO "testfs: superblock read successfuly.\n");

	testfs_sb = kmalloc(sizeof(*testfs_sb), GFP_KERNEL);
	if (!testfs_sb) {
		printk(KERN_ERR "testfs: failed to allocate sb memory\n");
		goto err;
	}
	memcpy(testfs_sb, bh->b_data, sizeof(*testfs_sb));

	testfs_i = kmalloc(sizeof(*testfs_i), GFP_KERNEL);
	if (!testfs_i) {
		printk(KERN_ERR "testfs: failed to allocate info memory\n");
		goto err;
	}
	memset(testfs_i, 0x00, sizeof(*testfs_i));
	testfs_i->block_bitmap = kmalloc(testfs_sb->block_size, GFP_KERNEL);

	if (!testfs_i->block_bitmap) {
		printk(KERN_ERR "testfs: failed to allocate memory for data block bitmap\n");
		goto err;
	}
	
	if (!(bh = sb_bread(sb, testfs_sb->block_bitmap)))
	{
		printk(KERN_ERR "testfs: unable to read data block bitmap from disk.\n");
		goto err;
	}
	memcpy(testfs_i->block_bitmap, bh->b_data, testfs_sb->block_size);
	
	printk(KERN_INFO "testfs: magic=%d block_size=%d "
		"block_count=%d itable=%d itable_size=%d block_bitmap=%d\n",
		testfs_sb->magic, testfs_sb->block_size, testfs_sb->block_count,
		testfs_sb->itable, testfs_sb->itable_size,
		testfs_sb->block_bitmap);

	/* Check whether valid testfs */
	if (testfs_sb->magic != TESTFS_MAGIC_NUM) {
		printk(KERN_ERR "testfs: not a testfs filesystem\n");
		goto err;
	}

	testfs_i->sb		= testfs_sb;
	testfs_i->bh		= bh;

	sb->s_fs_info		= testfs_i;
	sb->s_blocksize 	= testfs_sb->block_size;
	sb->s_blocksize_bits 	= 10;
	sb->s_magic		= TESTFS_MAGIC_NUM;
	sb->s_op		= &testfs_super_ops;

	root = inode_iget(sb, TESTFS_ROOT_INODE_NUM);
	if (!root) 
	{
		printk(KERN_ERR "testfs: inode_iget failed in fill_super!\n");
		goto err;
	}
	testfs_i->root = root;

	sb->s_root = d_make_root(root);
	if (!sb->s_root) {
		printk(KERN_ERR "testfs: get root dentry failed\n");
		goto err;
	}

	printk(KERN_INFO "testfs: mounted testfs file system\n");
	
	return 0;

err:
	if (root)
        	iput(root);
	if (testfs_i) {
		if (testfs_i->block_bitmap)
			kfree(testfs_i->block_bitmap);
		kfree(testfs_i);
	}
	if (testfs_sb)
		kfree(testfs_sb);
	if (bh)
		brelse(bh);

	return ret;
}


static void put_super (struct super_block *sb)
{
	struct testfs_info *testfs_i = NULL;
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);

	if (sb->s_fs_info) {
		testfs_i = sb->s_fs_info;

		if (testfs_i->sb) {
			kfree(testfs_i->sb);
		}

		if (testfs_i->bh) {
			brelse(testfs_i->bh);
		}

		kfree(sb->s_fs_info);
	}

	invalidate_bdev(sb->s_bdev);
}


inline int super_get_free_data_block_num(struct super_block *sb, int *block_num)
{
	struct testfs_info *testfs_info		= (struct testfs_info *)sb->s_fs_info;
	struct testfs_superblock *testfs_sb	= testfs_info->sb;

	int i,bit,byte				= 0;
	int mask					= 0x80;
	int block_size 				= testfs_sb->block_size;
	int first_data_block_num 	= testfs_sb->itable + testfs_sb->itable_size;

	for (i=0;i<block_size;i++) {
		byte = testfs_info->block_bitmap[i];
		for (bit=0;bit<7;bit++) {
			byte = byte << (bit == 0 ? 0 : 1);
			if ((byte & mask) < 1) {
				/* we found the first free data block, so we mark it as used */
				testfs_info->block_bitmap[i] = testfs_info->block_bitmap[i] ^ (mask >> bit);
				goto block_found;
			}
		}
	}

	return -1;
	
block_found:
	*block_num = ((i+1) * bit) + first_data_block_num;
	return 0;
}

