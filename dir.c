#include <linux/fs.h>

static int testfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static struct dentry *testfs_lookup(struct inode *dir, struct dentry *dentry,
		unsigned int flags)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return NULL;
}

static int testfs_link(struct dentry *old_dentry, struct inode *dir,
		struct dentry *dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_unlink(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_symlink(struct inode *dir, struct dentry *dentry,
		const char *symname)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
		dev_t rdev)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_rename(struct inode *old_dir, struct dentry *old_dentry,
		struct inode *new_dir, struct dentry *new_dentry)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

static int testfs_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "testfs: %s\n", __FUNCTION__);
	return 0;
}

/* dir file opertions */
const struct file_operations testfs_dir_fops = {
	.read		= generic_read_dir,
	.readdir	= testfs_readdir,
	.release	= testfs_release,
};

/* dir inode operations */
const struct inode_operations testfs_dir_iops = {
	.create		= testfs_create,
	.lookup		= testfs_lookup,
	.link		= testfs_link,
	.unlink		= testfs_unlink,
	.symlink	= testfs_symlink,
	.mkdir		= testfs_mkdir,
	.rmdir		= testfs_rmdir,
	.mknod		= testfs_mknod,
	.rename		= testfs_rename,
};

