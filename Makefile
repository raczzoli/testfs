obj-m := testfs.o
testfs-objs := bitmap.o dir.o file.o inode.o super.o testfs_main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

