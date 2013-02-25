#include <linux/fs.h>
#include <linux/buffer_head.h>


#include "aops.h"

int testfs_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create)
{
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