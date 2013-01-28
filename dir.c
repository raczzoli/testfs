#include <linux/fs.h>
#include <linux/buffer_head.h>

#include "super.h"


static int fop_readdir(struct file * fp, void * dirent, filldir_t filldir);

static struct dentry *iop_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags);


/* dir opertions */
const struct file_operations testfs_dir_fops = {
	.readdir = fop_readdir
};

/* dir inode operations */
const struct inode_operations testfs_dir_iops = {
	.lookup = iop_lookup
};



static struct dentry *iop_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	printk(KERN_INFO "testfs: lookup %s\n", dentry->d_name.name);
	return ERR_PTR(-EIO);
}


static int fop_readdir(struct file * fp, void * dirent, filldir_t filldir)
{
	struct inode *dir 	= fp->f_dentry->d_inode;


	printk(KERN_INFO "testfs: pos: %d\n", (int)fp->f_pos);

	if (fp->f_pos == 0) {
		filldir(dirent, ".", 1, fp->f_pos, dir->i_ino, DT_DIR);
		fp->f_pos++;
		return 0;
	}

        if (fp->f_pos == 1) {
                filldir(dirent, "..", 2, fp->f_pos, dir->i_ino, DT_DIR);
		fp->f_pos++;
                return 0;
        }

	return 0;
}
