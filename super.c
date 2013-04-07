#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/slab.h>

#include "testfs.h"
#include "super.h"
#include "inode.h"


// fill super
static int fill_super(struct super_block *sb, void *data, int silent);

// super operations
static void put_super(struct super_block *sb);

static struct super_operations testfs_super_ops = {
	.put_super 	= put_super,
	.write_inode	= inode_write_inode
};


struct dentry *super_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return mount_bdev(fs_type, flags, dev_name, data, fill_super);
}


static int fill_super(struct super_block *sb, void *data, int silent)
{
	struct buffer_head *bh 		= NULL;
	struct inode *root  		= NULL;
	struct testfs_superblock *testfs_sb = NULL;
	struct testfs_info *testfs_i 	= NULL;
	int ret 			= -1;
	int i, j 			= 0;	
	unsigned long desc_block 	= 0;

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
	/*
	if (!(testfs_i->block_bmp_bh = sb_bread(sb, testfs_sb->block_bitmap)))
	{
		printk(KERN_ERR "testfs: unable to read data block bitmap from disk.\n");
		goto err;
	}

	if (!(testfs_i->inode_bmp_bh = sb_bread(sb, testfs_sb->inode_bitmap)))
	{
		printk(KERN_ERR "testfs: unable to read inode bitmap from disk.\n");
		goto err;
	}
	*/
	printk(KERN_INFO "testfs: magic=%d, block_size=%d, groups count=%d, blocks_count=%d\n",
		testfs_sb->magic, testfs_sb->block_size, testfs_sb->group_count, testfs_sb->block_count);

	/* Check whether valid testfs */
	if (testfs_sb->magic != TESTFS_MAGIC_NUM) {
		printk(KERN_ERR "testfs: not a testfs filesystem\n");
		goto err;
	}

	testfs_i->sb		= testfs_sb;
	testfs_i->bh		= bh;

	sb->s_fs_info		= testfs_i;
	sb->s_blocksize 	= testfs_sb->block_size;
	sb->s_blocksize_bits 	= 12;  // hardcode... block size = 1 << blkbits, 4096 = 1 << blkbits, 4096 = 1 << 12
	sb->s_magic		= TESTFS_MAGIC_NUM;
	sb->s_op		= &testfs_super_ops;

	testfs_i->group_desc_bh = kmalloc(testfs_sb->group_count * sizeof(struct buffer_head *), GFP_KERNEL);
        if (!testfs_i->group_desc_bh) {
                printk(KERN_ERR "testfs: error allocating memory for group descriptor table!\n");
                goto err;
        }

	for (i=0; i<testfs_sb->group_count; i++)
        {
                desc_block = (testfs_sb->blocks_per_group * i) + 1;
                testfs_i->group_desc_bh[i] = sb_bread(sb, desc_block);
                if (!testfs_i->group_desc_bh[i]) {
                        for (j=0; j<i; j++)
                                brelse(testfs_i->group_desc_bh[j]);
                        printk(KERN_ERR "testfs: error reading group descriptor!\n");
                        kfree(testfs_i->group_desc_bh);
                        goto err;
                }
        }


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

	return 0;

err:
	if (root)
        	iput(root);
	if (testfs_i) {
		//if (testfs_i->block_bmp_bh)
		//	brelse(testfs_i->block_bmp_bh);
		//if (testfs_i->inode_bmp_bh)
		//	brelse(testfs_i->inode_bmp_bh);
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


