#!/bin/sh

umount /dev/loop0
rmmod testfs
insmod testfs.ko
mount -t testfs /dev/loop0 /mnt/
