#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

#include "super.h"


MODULE_LICENSE("Dual BSD/GPL");

static struct file_system_type testfs_type = {
  .owner 		= THIS_MODULE,
	.name		= "testfs",
	.mount		= super_mount,
	.kill_sb 	= kill_block_super
};


static int __init testfs_init(void)
{
	int ret = 0;

	printk(KERN_INFO "testfs: init...\n");

	ret = register_filesystem(&testfs_type);
	
	if (ret)
	{
		printk(KERN_INFO "testfs: file system registration failed!\n");
		return -1;
	}
	
	printk(KERN_INFO "testfs: file system registered successfuly.\n");
	return 0;
}


static void __exit testfs_exit(void)
{
	unregister_filesystem(&testfs_type);
	printk(KERN_INFO "testfs: exit...\n");
}

module_init(testfs_init);
module_exit(testfs_exit);


