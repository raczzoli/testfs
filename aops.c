#include <linux/fs.h>
#include <linux/buffer_head.h>

#include "testfs.h"
#include "inode.h"
#include "aops.h"


int testfs_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create)
{
	int err = 0;
	struct testfs_inode *testfs_inode = TESTFS_GET_INODE(inode); 

	if (create) {
		err = inode_alloc_data_block(inode->i_sb, inode);	
		if (err) {
			printk(KERN_INFO "testfs: entered here\n");
			return err;
		}

		bh_result->b_state      |= (1UL << BH_New);
	}
	else {
		bh_result->b_state      |= (1UL << BH_Mapped);
	}

	printk(KERN_INFO "testfs: testfs_get_block: op: %d, new data block for inode: %d\n", create, testfs_inode->block_ptr);

	bh_result->b_bdev 	= inode->i_sb->s_bdev;
        bh_result->b_blocknr 	= testfs_inode->block_ptr;

	return 0;
}

static int testfs_writepage(struct page *page, struct writeback_control *wbc)
{
	return block_write_full_page(page, testfs_get_block, wbc);
}

static int testfs_writepages(struct address_space *mapping, struct writeback_control *wbc)
{
	return mpage_writepages(mapping, wbc, testfs_get_block);
}

static int testfs_readpage(struct file *file, struct page *page)
{
	return mpage_readpage(page, testfs_get_block);
}

static int testfs_readpages(struct file *file, struct address_space *mapping,
		struct list_head *pages, unsigned nr_pages)
{
	return mpage_readpages(mapping, pages, nr_pages, testfs_get_block);
}


const struct address_space_operations testfs_aops = {
        .writepage	= testfs_writepage,
	.writepages	= testfs_writepages,
	.readpage	= testfs_readpage,
	.readpages	= testfs_readpages
};
