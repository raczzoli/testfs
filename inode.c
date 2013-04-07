#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/quotaops.h>
#include <linux/security.h>

#include "testfs.h"
#include "inode.h"
#include "dir.h"
#include "file.h"
#include "super.h"
#include "aops.h"


/*
 * reads the inode from the disk by inode number
 */
static struct testfs_inode *read_inode(struct super_block *sb, struct testfs_iloc *iloc);

/*
 * fills the inode with data from raw_inode. raw_inode can be read from the disk,
 * or instantiated programatically if the inode is new
 */
static int fill_inode(struct super_block *sb, struct inode *inode, struct testfs_inode *raw_inode);
static int fill_iloc_by_inode_num(struct super_block *sb, u32 ino, struct testfs_iloc *iloc);


struct inode *inode_iget(struct super_block *sb, u32 ino)
{
	struct inode *inode;
	struct testfs_iloc iloc;
	struct testfs_inode *raw_inode;

	inode = iget_locked(sb, ino);
	if (!inode) {
		return ERR_PTR(-ENOMEM);
	}
	if (!(inode->i_state & I_NEW)) {
		/* if inode found return it */
		return inode;
	}

	fill_iloc_by_inode_num(sb, ino, &iloc);

	/* Read raw inode block from disk */
	raw_inode = read_inode(sb, &iloc);
	if (!raw_inode) {
		printk(KERN_INFO "testfs: error reading inode number: %d\n", ino);
		goto err_read_inode;
	}
	inode->i_ino = ino;
	fill_inode(sb, inode, raw_inode);

	unlock_new_inode(inode);

	return inode;

err_read_inode:
	brelse(iloc.bh);
	return NULL;
}


static int get_inode_group(struct super_block *sb, struct inode *inode)
{
	struct testfs_inode *testfs_inode = TESTFS_GET_INODE(inode);
	return testfs_inode->group;
}


struct inode *inode_get_new_inode(struct inode *dir, umode_t mode, int alloc_data_block)
{
	int err					= 0;
	struct inode *new_ino	 		= NULL;
	int new_inode_num 			= 0;
	int i, group				= 0;
	struct buffer_head *bitmap_bh		= NULL;
	struct testfs_inode *testfs_inode	= NULL;
	struct testfs_group_desc *desc          = NULL;
	struct testfs_superblock *testfs_sb     = NULL;
	struct super_block *sb			= dir->i_sb;
	struct testfs_info *testfs_i		= TESTFS_GET_SB_INFO(sb);


	testfs_sb = testfs_i->sb;

	/*
	if (bitmap_get_free_inode_num(sb, &new_inode_num) != 0){
		printk(KERN_INFO "testfs: No more free inodes left!\n");
		return NULL;
	}
	*/

	group = get_inode_group(sb, dir);

	if (group < 0) {
		printk(KERN_INFO "testfs: invalid group for inode number: %lu\n", dir->i_ino);
		return ERR_PTR(-ENOSPC);
	}

	for (i=0;i<testfs_sb->group_count;i++)
	{
//		desc = get_group_desc();	
	}

	new_ino = new_inode(sb);
        if (!new_ino) {
		printk(KERN_INFO "testfs: inode_get_new_inode: new_ino = NULL\n");
                return ERR_PTR(-ENOMEM);
        }


	if (insert_inode_locked(new_ino) < 0) {
		printk(KERN_INFO "testfs: inode allocated twice!\n");
		iput(new_ino);
		return ERR_PTR(-EIO);
	}

	dquot_initialize(new_ino);
	err = dquot_alloc_inode(new_ino);
	if (err) {
		dquot_drop(new_ino);
		unlock_new_inode(new_ino);
		iput(new_ino);
		return ERR_PTR(err);
	}

	testfs_inode = kmalloc(sizeof(*testfs_inode), GFP_KERNEL);
        if (!testfs_inode) {
                printk(KERN_INFO "testfs: Error allocating memory testfs_inode object!\n");
                return NULL;
        }

	testfs_inode->block_ptr = 0;
	testfs_inode->i_mode 	= mode;	
	new_ino->i_ino 		= new_inode_num;

	fill_inode(sb, new_ino, testfs_inode);

	new_ino->i_atime 	= new_ino->i_ctime = new_ino->i_mtime = CURRENT_TIME_SEC;
	new_ino->i_size		= 234;
        new_ino->i_blkbits     	= 12; // hardcode... block size = 1 << blkbits, 4096 = 1 << blkbits, 4096 = 1 << 12
	new_ino->i_blocks	= 0;
	
		
	security_inode_init_security(new_ino, dir, NULL, NULL, NULL);
	inode_init_owner(new_ino, dir, mode);

	insert_inode_hash(new_ino);
	mark_inode_dirty(new_ino);

        if (alloc_data_block) {
                err = inode_alloc_data_block(sb, new_ino);
                if (err) {
                        unlock_new_inode(new_ino);
                        iput(new_ino);
                        return ERR_PTR(err);
                }
        }

	unlock_new_inode(new_ino);

	return new_ino;
}



static int fill_inode(struct super_block *sb, struct inode *inode, struct testfs_inode *raw_inode)
{
        /* Initialize inode */
        inode->i_mode = le16_to_cpu(raw_inode->i_mode);
        inode->i_size = le16_to_cpu(raw_inode->i_size);
        inode->i_private = raw_inode;

        i_uid_write(inode, 0);
        i_gid_write(inode, 0);

        inode->i_atime.tv_sec = (signed)le32_to_cpu(0);
        inode->i_ctime.tv_sec = (signed)le32_to_cpu(0);
        inode->i_mtime.tv_sec = (signed)le32_to_cpu(0);
        inode->i_atime.tv_nsec = inode->i_ctime.tv_nsec = inode->i_mtime.tv_nsec = 0;

        if (S_ISDIR(inode->i_mode)) {            /* 16384 */
                inode->i_op     	= &testfs_dir_iops; // set the inode ops
                inode->i_fop    	= &testfs_dir_fops;
		inode->i_mapping->a_ops = &testfs_aops;
        }
        else if (S_ISREG(inode->i_mode)) {        /* 32768 */
                inode->i_op     	= &testfs_file_iops; // set the inode ops
                inode->i_fop    	= &testfs_file_fops;
		inode->i_mapping->a_ops = &testfs_aops;
        }

	return 0;
}


static int fill_iloc_by_inode_num(struct super_block *sb, u32 ino, struct testfs_iloc *iloc)
{
	unsigned long block_group 	= 0;
	struct testfs_group_desc *desc	= NULL;  
	struct testfs_info *testfs_i	= TESTFS_GET_SB_INFO(sb);

	block_group = ino / TESTFS_INODES_PER_GROUP(sb);        
	desc = (struct testfs_group_desc *)testfs_i->group_desc_bh[block_group]->b_data;

	printk(KERN_INFO "testfs: inodes per group: %d\n", TESTFS_INODES_PER_GROUP(sb));

	// TODO: remove itable pointer hardcode
        iloc->block_num = le32_to_cpu(desc->inode_table) +
                ((ino * sizeof(struct testfs_inode)) / sb->s_blocksize);
        iloc->offset = (ino * sizeof(struct testfs_inode)) % sb->s_blocksize;
        iloc->ino = ino;

	return 0;
}


static struct testfs_inode *read_inode(struct super_block *sb, struct testfs_iloc *iloc)
{
	/* Read block from disk */
	if (!(iloc->bh = sb_bread(sb, iloc->block_num))) {
		printk(KERN_INFO "testfs: error reading inode number %d from disk\n", iloc->ino);
		return ERR_PTR(-EIO);
	}

	/* Copy inode structure from disk */
	return (struct testfs_inode *)(iloc->bh->b_data + iloc->offset);
}


int inode_write_inode(struct inode *inode, struct writeback_control *wbc)
{
	struct testfs_iloc iloc;
        struct testfs_inode *raw_inode = NULL;

	fill_iloc_by_inode_num(inode->i_sb, inode->i_ino, &iloc);
	raw_inode = read_inode(inode->i_sb, &iloc);

	if (!raw_inode) {
		return -EIO;
	}

	raw_inode->i_size 	= inode_get_size(inode);
	raw_inode->i_mode 	= inode->i_mode;
	raw_inode->block_ptr 	= ((struct testfs_inode *)inode->i_private)->block_ptr;

	printk(KERN_INFO "testfs: writing inode: %lu, mode: %d, type: %s\n", inode->i_ino, raw_inode->i_mode, S_ISREG(inode->i_mode) ? "file" : "dir");

	mark_buffer_dirty(iloc.bh);

        return 0;
}



int inode_alloc_data_block(struct super_block *sb, struct inode *inode)
{
	/* TODO
	struct testfs_inode *testfs_inode = TESTFS_GET_INODE(inode);

	if (bitmap_get_free_data_block_num(sb, &testfs_inode->block_ptr) != 0) {
                printk(KERN_INFO "testfs: Error allocating data block for new inode!\n");
                return -ENOSPC;
        }
	*/
	mark_inode_dirty(inode);

	return 0;
}


int inode_get_data_block_num(struct inode *inode)
{
	struct testfs_inode *raw_inode 		= (struct testfs_inode *)inode->i_private;
	//struct testfs_info *testfs_info	= (struct testfs_info *)inode->i_sb->s_fs_info;
	//struct testfs_superblock *sb		= testfs_info->sb;

	/*
	 * we are checking if raw_inode is pointing to a valid data block number
	 */
	/* TODO
	if (raw_inode->block_ptr < (sb->itable + sb->itable_size)) {
		printk(KERN_INFO "testfs: invalid data block number %d\n ", raw_inode->block_ptr);
		return -1;
	}
	*/
	return raw_inode->block_ptr;
}

int inode_get_size(struct inode *inode)
{
	struct testfs_inode *raw_inode 		= (struct testfs_inode *)inode->i_private;

	return le16_to_cpu(raw_inode->i_size);
}




