#ifndef EXT2_STRUCTS_H
#define EXT2_STRUCTS_H

#include <stdint.h>

// Sizes
#define EXT2_MAGIC 0xEF53
#define EXT2_BLOCK_SIZE(base) (1024 << (base))

// On-disk superblock (fixed location at byte 1024)
typedef struct __attribute__((packed)) ext2_superblock
{
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t res_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;

    uint32_t first_data_block;
    uint32_t log_block_size;  // block size = 1024 << log_block_size
    uint32_t log_frag_size;   // fragment size = 1024 << log_frag_size
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;

    uint32_t mount_time; // posix time
    uint32_t write_time; // posix time

    uint16_t mount_count;
    uint16_t max_mount_count;
    uint16_t magic;          // must be EXT2_MAGIC
    uint16_t state;
    uint16_t on_error;
    uint16_t minor_version;

    uint32_t last_check_time; // posix time
    uint32_t check_interval;  // interval for consistency check
    uint32_t os_id;
    uint32_t major_version;

    uint16_t res_uid;
    uint16_t res_gid;

    // there is more but ignored for now (extended superblock)

} ext2_superblock_t;


// Block Group Descriptor
typedef struct __attribute__((packed)) ext2_group_desc
{
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;

    uint16_t _reserved;
    uint32_t _pad[3];
} ext2_group_desc_t;


// On-disk inode
typedef struct __attribute__((packed)) ext2_inode_disk
{
    uint16_t mode;       // file type + permissions
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;     // number of 512-byte sectors
    uint32_t flags;
    uint32_t osd1;

    uint32_t block[15];  // block pointers (12 direct + indirects)

    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
} ext2_inode_disk_t;


// Directory entry
typedef struct __attribute__((packed)) ext2_dir_entry
{
    uint32_t inode;     // inode number
    uint16_t rec_len;   // length of this entry
    uint8_t  name_len;  // length of name
    uint8_t  file_type; // file type (only if s_rev_level > 0)
    char     name[];    // variable length
} ext2_dir_entry_t;


// File types in directory entry
enum ext2_file_type 
{
    EXT2_FT_UNKNOWN  = 0,
    EXT2_FT_REG_FILE = 1,
    EXT2_FT_DIR      = 2,
    EXT2_FT_SYMLINK  = 7,
};

#endif // EXT2_STRUCTS_H
