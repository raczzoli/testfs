#include <linux/fs.h>
#include <linux/quotaops.h>

#include "file.h"

int testfs_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	return generic_file_fsync(file, start, end, datasync);	
}


loff_t testfs_llseek(struct file *file, loff_t offset, int whence)
{
	int ret = 0;
	ret = generic_file_llseek(file, offset, whence);

	printk(KERN_INFO "testfs: testfs_llseek called...\n");

	return ret;
}


ssize_t testfs_aio_read(struct kiocb *iocb, const struct iovec *iov, unsigned long nr_segs, loff_t pos)
{
	int ret = 0;
	printk(KERN_INFO "testfs: testfs_aio_read\n");
	ret = generic_file_aio_read(iocb, iov, nr_segs, pos);

	return ret;
}


ssize_t testfs_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	int ret = 0;
	printk(KERN_INFO "testfs: testfs_sync_read\n");
	ret = do_sync_read(filp, buf, len, ppos);

	return ret;
}


ssize_t testfs_splice_read(struct file *in, loff_t *ppos, struct pipe_inode_info *pipe, size_t len, unsigned int flags)
{
	int ret = 0;
	printk(KERN_INFO "testfs: testfs_splice_read\n");
	ret = generic_file_splice_read(in, ppos, pipe, len, flags);

	return ret;
}

/* file operations */

const struct file_operations testfs_file_fops = {
	.llseek 	= testfs_llseek,
	.aio_read 	= testfs_aio_read,
	.aio_write 	= generic_file_aio_write,
	.read		= testfs_sync_read,
	.write		= do_sync_write,
	.splice_read	= testfs_splice_read,
	.splice_write	= generic_file_splice_write,
	.mmap		= generic_file_mmap,
	.open		= dquot_file_open,
	.fsync 		= testfs_fsync
};

/* file inode operations */
const struct inode_operations testfs_file_iops;
