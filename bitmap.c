#include "bitmap.h"
#include "testfs.h"
#include "super.h"



// checks for the next free bit in bitmap (used for block and inode bitmaps)
static inline int get_free_bit_from_bitmap(struct super_block *sb, char *bitmap, int start_pos, int *num);


inline int bitmap_get_free_data_block_num(struct super_block *sb, int *block_num)
{
        struct testfs_info *testfs_info         = (struct testfs_info *)sb->s_fs_info;
        struct testfs_superblock *testfs_sb     = testfs_info->sb;
        int start_block_num                     = testfs_sb->itable + testfs_sb->itable_size;

	printk(KERN_INFO "testfs: requesting free data block\n");
        return get_free_bit_from_bitmap(sb, testfs_info->block_bitmap, start_block_num, block_num);
}


inline int bitmap_get_free_inode_num(struct super_block *sb, int *inode_num)
{
        struct testfs_info *testfs_info         = (struct testfs_info *)sb->s_fs_info;
        struct testfs_superblock *testfs_sb     = testfs_info->sb;

	printk(KERN_INFO "testfs: requesting free inode number\n");
        return get_free_bit_from_bitmap(sb, testfs_info->inode_bitmap, testfs_sb->itable, inode_num);
}


inline int bitmap_free_data_block_num(struct super_block *sb, int block_num)
{
        struct testfs_info *testfs_info         = (struct testfs_info *)sb->s_fs_info;
        struct testfs_superblock *testfs_sb     = testfs_info->sb;
        int start_block_num                     = testfs_sb->itable + testfs_sb->itable_size;

        int mask  = 0x80;
        int block = (block_num - start_block_num);
        int byte  = (block / 8);
        int bit   = block % 8;

        testfs_info->block_bitmap[byte] = testfs_info->block_bitmap[byte] ^ (mask >> bit);
	sb->s_dirt = 1;

        return 0;
}


inline int bitmap_free_inode_num(struct super_block *sb, int inode_num)
{
        struct testfs_info *testfs_info         = (struct testfs_info *)sb->s_fs_info;
        struct testfs_superblock *testfs_sb     = testfs_info->sb;
        int start_inode_num                     = testfs_sb->itable;

        int mask  = 0x80;
        int block = (inode_num - start_inode_num);
        int byte  = (block / 8);
        int bit   = block % 8;

        testfs_info->inode_bitmap[byte] = testfs_info->inode_bitmap[byte] ^ (mask >> bit);
	sb->s_dirt = 1;

        return 0;
}


static inline int get_free_bit_from_bitmap(struct super_block *sb, char *bitmap, int start_pos, int *num)
{
        struct testfs_info *testfs_info         = (struct testfs_info *)sb->s_fs_info;
        struct testfs_superblock *testfs_sb     = testfs_info->sb;

        int i,bit,byte                          = 0;
        int mask                                = 0x80;
        int block_size                          = testfs_sb->block_size;
        int block_counter                       = 0;

        for (i=0;i<block_size;i++) {
                byte = bitmap[i];
                for (bit=0;bit<7;bit++) {
                        byte = byte << (bit == 0 ? 0 : 1);
                        if ((byte & mask) < 1) {
                                /* we found the first free block, so we mark it as used */
                                bitmap[i] = bitmap[i] ^ (mask >> bit);
                                goto block_found;
                        }
                        block_counter++;
                }
        }

        return -1;

block_found:
        *num = block_counter + start_pos;
	printk(KERN_INFO "testfs: found free block %d\n", *num);
	sb->s_dirt = 1;
        return 0;

}




