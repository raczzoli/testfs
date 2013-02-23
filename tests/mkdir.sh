#!/bin/bash rm -rf /mnt/dir*
for i in `seq 1 5`; do
	mkdir /home/ranger/tests/dir$i
	for j in `seq 1 5`;
	do
        	mkdir /home/ranger/tests/dir$i/subdir$j
		for k in `seq 1 5`;
	        do
        	        mkdir /home/ranger/tests/dir$i/subdir$j/subdir$k
		done
	done
done
