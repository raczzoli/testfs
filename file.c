#include <linux/fs.h>
#include <linux/quotaops.h>


int testfs_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	return generic_file_fsync(file, start, end, datasync);	
}


/* file operations */
const struct file_operations testfs_file_fops = {
	.llseek 	= generic_file_llseek,
	.aio_read 	= generic_file_aio_read,
	.aio_write 	= generic_file_aio_write,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
	.mmap		= generic_file_mmap,
	.open		= dquot_file_open,
	.fsync 		= testfs_fsync
};

/* file inode operations */
const struct inode_operations testfs_file_iops;
