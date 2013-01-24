#ifndef SUPER_H
#define SUPER_H

#include <linux/fs.h>


struct dentry *super_mount(struct file_system_type *fs_type,
  int flags, const char *dev_name, void *data);

#endif /* SUPER_H */
