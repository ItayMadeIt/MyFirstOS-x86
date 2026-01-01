#ifndef __INODE_H__
#define __INODE_H__

// On-disk inode
#include "core/num_defs.h"
#include <stdbool.h>

#define EXT2_MAGIC 0xEF53
#define EXT2_BLOCK_SIZE(base) (1024 << (base))
#define EXT2_SB_OFFSET 1024


// File types in directory entry
enum ext2_disk_file_type 
{
    EXT2_FT_UNKNOWN   = 0x1,
    EXT2_FT_CHAR_DEV  = 0x2,
    EXT2_FT_DIR       = 0x4,
    EXT2_FT_BLOCK_DEV = 0x6,
    EXT2_FT_REG_FILE  = 0x8,
    EXT2_FT_SYMLINK   = 0xA,
    EXT2_FT_UNIX_SOCK = 0xC,
};

typedef struct __attribute__((packed)) ext2_inode_disk
{
    u16 mode;       // file type + permissions
    u16 uid;
    u32 size;
    u32 atime;
    u32 ctime;
    u32 mtime;
    u32 dtime;
    u16 gid;
    u16 links_count;
    u32 blocks;     // number of [sb.block_size]-byte sectors
    u32 flags;
    u32 osd1;

    u32 block[15];  // block pointers (12 direct + indirects)

    u32 generation;
    u32 file_acl;
    u32 dir_acl;
    u32 faddr;
    u8  os_specific[12];
} ext2_inode_disk_t;

static inline bool ext2_inode_is_type(ext2_inode_disk_t* inode_disk, enum ext2_disk_file_type type)
{
    return (inode_disk->mode & 0xF000) >> 12 == type;
}

// On-disk superblock (fixed location at byte 1024)
typedef struct __attribute__((packed)) ext2_disk_superblock
{
    u32 inodes_count;
    u32 blocks_count;
    u32 res_blocks_count;
    u32 free_blocks_count;
    u32 free_inodes_count;

    u32 first_data_block;
    u32 log_block_size;  // block size = 1024 << log_block_size
    u32 log_frag_size;   // fragment size = 1024 << log_frag_size
    u32 blocks_per_group;
    u32 frags_per_group;
    u32 inodes_per_group;

    u32 mount_time; // posix time
    u32 write_time; // posix time

    u16 mount_count;
    u16 max_mount_count;
    u16 magic;          // must be EXT2_MAGIC
    u16 state;
    u16 on_error;
    u16 minor_version;

    u32 last_check_time; // posix time
    u32 check_interval;  // interval for consistency check
    u32 os_id;
    u32 major_version;

    u16 res_uid;
    u16 res_gid;

    // there is more but ignored for now (extended superblock)

    u32 non_res_inode;
    u16 inode_size;

} ext2_disk_superblock_t;

// Block Group Descriptor
typedef struct __attribute__((packed)) ext2_disk_group_desc
{
    u32 block_bitmap;
    u32 inode_bitmap;
    u32 inode_table;
    u16 free_blocks_count;
    u16 free_inodes_count;
    u16 used_dirs_count;

    u8 _reserved[14];
    
} ext2_disk_group_desc_t;


// Directory entry
typedef struct __attribute__((packed)) ext2_disk_dir_entry
{
    u32 inode;     // inode number
    u16 rec_len;   // length of this entry
    u8  name_len;  // length of name
    u8  file_type; // file type (only if s_rev_level > 0)
    char   name[]; // variable length
} ext2_disk_dir_entry_t;



#endif // __INODE_H__