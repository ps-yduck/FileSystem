/**
 * @file fs.h
 * @brief This header file contains the structures and constants used in the file system.
 *
 * This header file contains the following structures:
 * - superblock: contains information about the file system.
 * - inode: contains information about a file or directory.
 * - directory_entry: contains information about a directory entry.
 * - directory_block: contains an array of directory entries.
 * - block: contains all possible types of blocks in the file system.
 *
 * This header file also defines the following constants:
 * - INODE_SIZE: size of an inode in bytes.
 * - INODES_PER_BLOCK: number of inodes that can fit in a block.
 * - INODE_DIRECT_POINTERS: number of direct pointers in an inode.
 * - INODE_INDIRECT_POINTERS_PER_BLOCK: number of indirect pointers that can fit in a block.
 * - DIRECTORY_ENTRY_SIZE: size of a directory entry in bytes.
 * - DIRECTORY_NAME_SIZE: maximum size of a directory name in bytes.
 * - DIRECTORY_ENTRIES_PER_BLOCK: number of directory entries that can fit in a block.
 *
 * This header file includes the following header files:
 * - stdint.h: defines integer types.
 * - disk.h: defines constants and functions related to the disk.
 */
#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <sys/types.h>

#include "disk.h"

#define INODE_SIZE 64
#define INODES_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define INODE_DIRECT_POINTERS 11
#define INODE_INDIRECT_POINTERS_PER_BLOCK (BLOCK_SIZE / sizeof(uint32_t))

#define DIRECTORY_ENTRY_SIZE 32
#define DIRECTORY_NAME_SIZE 28
#define DIRECTORY_ENTRIES_PER_BLOCK (BLOCK_SIZE / DIRECTORY_ENTRY_SIZE)
#define DIRECTORY_DEPTH_LIMIT 10

#define FLAGS_PER_BLOCK (BLOCK_SIZE / sizeof(uint32_t))

/**
 * @brief The superblock structure contains information about the file system.
 *
 * @param s_blocks_count Total number of blocks in the file system.
 * @param s_inodes_count Total number of inodes in the file system.
 * @param s_inode_bitmap Block number of the inode bitmap.
 * @param s_block_bitmap Block number of the block bitmap.
 * @param s_inode_table_block_start Starting block number of the inode table.
 * @param s_data_blocks_start Starting block number of the data blocks.
 */
struct superblock
{
    uint32_t s_blocks_count;
    uint32_t s_inodes_count;
    uint32_t s_block_bitmap;
    uint32_t s_inode_bitmap;
    uint32_t s_inode_table_block_start;
    uint32_t s_data_blocks_start;
};

/**
 * @brief The inode structure contains information about a file or directory.
 *
 * @param i_is_directory Flag indicating whether the inode represents a directory.
 * @param i_size Size of the file or directory in bytes.
 * @param i_direct_pointers Array of direct pointers to data blocks.
 * @param i_single_indirect_pointer Pointer to a block containing indirect pointers to data blocks.
 * @param i_double_indirect_pointer Pointer to a block containing indirect pointers to blocks containing indirect pointers to data blocks.
 */
struct inode
{
    uint64_t i_size;
    uint32_t i_is_directory;
    uint32_t i_direct_pointers[INODE_DIRECT_POINTERS];
    uint32_t i_single_indirect_pointer;
    uint32_t i_double_indirect_pointer;
};

/**
 * @brief The directory_entry structure contains information about a directory entry.
 *
 * @param inode_number Inode number of the file or directory.
 * @param name Name of the file or directory.
 */
struct directory_entry
{
    uint32_t inode_number;
    char name[DIRECTORY_NAME_SIZE];
};

/**
 * @brief The directory_block structure contains an array of directory entries.
 *
 * @param entries Array of directory entries.
 */
struct directory_block
{
    struct directory_entry entries[DIRECTORY_ENTRIES_PER_BLOCK];
};

/**
 * @brief The block union contains all possible types of blocks in the file system.
 *
 * @param superblock Superblock structure.
 * @param inodes Array of inodes.
 * @param bitmap Array of bitmap blocks.
 * @param directory_block Directory block structure.
 * @param data Array of data blocks.
 * @param pointers Array of indirect pointers.
 */
union block
{
    struct superblock superblock;                         // Superblock
    struct inode inodes[INODES_PER_BLOCK];                // Inode block
    uint32_t bitmap[FLAGS_PER_BLOCK];       // Bitmap block (inode or data)
    struct directory_block directory_block;               // Directory block
    uint8_t data[BLOCK_SIZE];                             // Data block
    uint32_t pointers[INODE_INDIRECT_POINTERS_PER_BLOCK]; // Indirect pointer block
};

/**
 * @brief Formats the file system.
 *
 * @return 0 on success, -1 on failure.
 */
int fs_format();

/**
 * @brief Mounts the file system.
 *
 * @return 0 on success, -1 on failure.
 */
int fs_mount();

/**
 * @brief Create a file or directory at the specified absolute path.
 *
 * If intermediate directories leading to the path do not exist, create them.
 * The provided path must start with a slash (/) and be absolute.
 * Use a flag to specify whether the path represents a file or a directory.
 *
 * @param path The path of the file or directory to create.
 * @param is_directory Flag indicating whether the file or directory to create is a directory.
 * @return 0 on success, -1 on failure.
 */
int fs_create(char *path, int is_directory);

/**
 * @brief Remove the file or directory at the specified absolute path.
 *
 * If the path represents a directory, remove all files and directories inside it recursively (does NOT mean you are required to use recursion).
 * The provided path must start with a slash (/) and be absolute.
 *
 * @param path The path of the file or directory to remove.
 * @return 0 on success, -1 on failure.
 */
int fs_remove(char *path);

/**
 * @brief Reads data from a file at the specified path and stores it in the buffer pointed to by buf.
 * 
 * Think of edge cases such as reading from an offset other than 0, reading more bytes than the file contains, file not existing, etc.
 * 
 * @param path The path of the file to be read.
 * @param buf The buffer to store the read data.
 * @param count The number of bytes to be read.
 * @param offset The offset from the beginning of the file to start reading from.
 * 
 * @return On success, the number of bytes read is returned. On error, -1 is returned.
 */
int fs_read(char *path, void *buf, size_t count, off_t offset);

/**
 * @brief Writes data from the buffer pointed to by buf to a file at the specified path.
 * 
 * Create the file if it doesn't exist.
 * 
 * 
 * @param path The path of the file to be written to.
 * @param buf The buffer containing the data to be written.
 * @param count The number of bytes to be written.
 * @param offset The offset from the beginning of the file to start writing to.
 * 
 * @return On success, the number of bytes written is returned. On error, -1 is returned.
 */
int fs_write(char *path, void *buf, size_t count, off_t offset);

/**
 * @brief Lists all files and directories in the directory at the specified path.
 * 
 * Format: <name> <size>
 * 
 * @param path The path of the directory to be listed.
 * 
 * @return 0 on success, -1 on failure.
 */
int fs_list(char *path);

/**
 * @brief Displays statistics about the file system.
 * 
 */
void fs_stat();


#endif