#!/bin/sh

dd if=/dev/zero of=loopback.img bs=1024 count=204800
losetup /dev/loop0 loopback.img

format /dev/loop0
