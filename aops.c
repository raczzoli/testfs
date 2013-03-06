#include <linux/fs.h>
#include <linux/buffer_head.h>

#include "testfs.h"
#include "inode.h"
#include "aops.h"


int testfs_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create)
{

	int err = 0;
	struct testfs_inode *testfs_inode = TESTFS_GET_INODE(inode); 

	printk(KERN_INFO "testfs: testfs_get_block: inode flags: %d\n", inode->i_flags);

	if (testfs_inode->block_ptr == 0) {
		if (create == 0) 
			return 0;

		err = inode_alloc_data_block(inode->i_sb, inode);
                if (err) {
                        printk(KERN_INFO "testfs: entered here\n");
                        return err;
                }
		bh_result->b_state |= (1UL << BH_New) | (1UL << BH_Mapped);
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
	printk(KERN_INFO "testfs: testfs_writepage called\n");
	return block_write_full_page(page, testfs_get_block, wbc);
}

static int testfs_writepages(struct address_space *mapping, struct writeback_control *wbc)
{
	return mpage_writepages(mapping, wbc, testfs_get_block);
}

static int testfs_readpage(struct file *file, struct page *page)
{
	printk(KERN_INFO "testfs: testfs_readpage called\n");
	return mpage_readpage(page, testfs_get_block);
}

static int testfs_readpages(struct file *file, struct address_space *mapping,
		struct list_head *pages, unsigned nr_pages)
{
	printk(KERN_INFO "testfs: testfs_readpages called\n");
	return mpage_readpages(mapping, pages, nr_pages, testfs_get_block);
}


static int testfs_write_begin(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned flags,
		struct page **pagep, void **fsdata)
{
	return block_write_begin(mapping, pos, len, flags, pagep, testfs_get_block);
}


static int testfs_write_end(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata)
{
	return generic_write_end(file, mapping, pos, len, copied, page, fsdata);
}


static sector_t testfs_bmap(struct address_space *mapping, sector_t block)
{
	return generic_block_bmap(mapping,block,testfs_get_block);
}


const struct address_space_operations testfs_aops = {
        .writepage	= testfs_writepage,
	.writepages	= testfs_writepages,
	.readpage	= testfs_readpage,
	.readpages	= testfs_readpages,
	.write_begin	= testfs_write_begin,
	.write_end	= testfs_write_end,
	.bmap		= testfs_bmap
};
