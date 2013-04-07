#!/bin/sh

umount /dev/loop0
dd if=/dev/zero of=loopback.img bs=100M count=3
losetup /dev/loop0 loopback.img

/usr/bin/format /dev/loop0
