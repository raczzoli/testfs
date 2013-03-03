#include <linux/fs.h>



int testfs_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	printk(KERN_INFO "testfs: testfs_fsync called\n");
	return 0;
}


/* file operations */
const struct file_operations testfs_file_fops = {
	.llseek 	= generic_file_llseek,
	.aio_read 	= generic_file_aio_read,
	.aio_write 	= generic_file_aio_write,
//	.read		= generic_file_read,
//	.write		= generic_file_write,
	.mmap		= generic_file_mmap,
	.open		= generic_file_open,
	.fsync 		= testfs_fsync
};

/* file inode operations */
const struct inode_operations testfs_file_iops;
