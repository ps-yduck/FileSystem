/**
 * @file fs.c
 * @author Hamza
 * @brief
 * @version 0.1
 * @date 2023-11-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "fs.h"

static int MOUNT_FLAG = 0;
static union block SUPERBLOCK;
static union block BLOCK_BITMAP;
static union block INODE_BITMAP;

int util_create_dir_file(union block *inode_block, struct inode *file_inode, union block *dir_data_block, char **path_tokens, int i, int is_directory, uint32_t *ret_inode_number);
int util_read_loop(struct inode file_inode, void *buf, size_t count, off_t offset);
int util_write_loop(uint32_t inode_number, struct inode file_inode, union block inode_block, void *buf, size_t count, off_t offset);
int util_recursive_remove(uint32_t child_inode_number, struct inode child_file_inode, union block child_inode_block, union block parent_dir_data_block, int parent_entry, struct inode parent_inode);
uint32_t util_calc_inode_blocks(uint32_t inodes_count);
void fs_unmount()
{
    if (MOUNT_FLAG == 0)
    {
    
    return;
    }
   
    MOUNT_FLAG = 0;
}
int fs_format()
{

   

    uint32_t disk_sizee = disk_size();
    if (disk_sizee < 5)
    {
        //very small disk
        return -1;
    }

 
    union block void_block;
    memset(void_block.data, 0, BLOCK_SIZE);

    for (uint32_t i = 0; i < disk_sizee; i++)
    {
        disk_write(i, void_block.data);
    }
    union block superblock;
    superblock.superblock.s_blocks_count = disk_sizee; 
    superblock.superblock.s_inodes_count = disk_sizee;
    uint32_t inode_blocks = util_calc_inode_blocks(disk_sizee);
  
    superblock.superblock.s_block_bitmap = 1;
    superblock.superblock.s_inode_bitmap = 2;
    superblock.superblock.s_inode_table_block_start = 3;
    superblock.superblock.s_data_blocks_start = 3 + inode_blocks;

    disk_write(0, superblock.data);

    union block block_bitmap;
    memset(block_bitmap.data, 0, BLOCK_SIZE);

    block_bitmap.bitmap[0] = 1;
    block_bitmap.bitmap[1] = 1;
    block_bitmap.bitmap[2] = 1;
    for (uint32_t i = 3; i < superblock.superblock.s_blocks_count; i++)
    {
        block_bitmap.bitmap[i] = 0;
    }

    union block inode_bitmap;
    memset(inode_bitmap.data, 0, BLOCK_SIZE);

    for (uint32_t i = 0; i < superblock.superblock.s_inodes_count; i++)
    {
        inode_bitmap.bitmap[i] = 0;
    }


    for (uint32_t i = 0; i < inode_blocks; i++)
    {
        union block inode_block;
        memset(inode_block.data, 0, BLOCK_SIZE);
        block_bitmap.bitmap[3 + i] = 1;
        for (int j = 0; j < INODES_PER_BLOCK; j++)
        {
            inode_block.inodes[j].i_size = 0;
            inode_block.inodes[j].i_is_directory = 0;
            for (int k = 0; k < INODE_DIRECT_POINTERS; k++)
            {
                inode_block.inodes[j].i_direct_pointers[k] = 0;
            }
            inode_block.inodes[j].i_single_indirect_pointer = 0;
            inode_block.inodes[j].i_double_indirect_pointer = 0;
        }

        disk_write(3 + i, inode_block.data);
    }

    // root
    union block root_dir;
    memset(root_dir.data, 0, BLOCK_SIZE);

    inode_bitmap.bitmap[0] = 1;
    disk_write(2, inode_bitmap.data);

    block_bitmap.bitmap[3 + inode_blocks] = 1;
    disk_write(1, block_bitmap.data);

    union block inode_block;
    disk_read(3, inode_block.data);
    inode_block.inodes[0].i_is_directory = 1;
    inode_block.inodes[0].i_direct_pointers[0] = 3 + inode_blocks;
    inode_block.inodes[0].i_size = BLOCK_SIZE;
    disk_write(3, inode_block.data);

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        root_dir.directory_block.entries[i].inode_number = 0;
        strcpy(root_dir.directory_block.entries[i].name, "");
    }

    disk_write(3 + inode_blocks, root_dir.data);

    return 0;
}

int fs_mount()
{

    if (MOUNT_FLAG == 1)
    {
        
        return -1;
    }

    disk_read(0, SUPERBLOCK.data);


    disk_read(SUPERBLOCK.superblock.s_block_bitmap, BLOCK_BITMAP.data);

    disk_read(SUPERBLOCK.superblock.s_inode_bitmap, INODE_BITMAP.data);

    MOUNT_FLAG = 1;
    return 0;
}
char **path_parser(char *path)
{
    char **path_tokens = malloc(sizeof(char *) * 100);
    char *path_copy = malloc(sizeof(char) * strlen(path));
    strcpy(path_copy, path);


    char *token = strtok(path_copy, "/");
    int i = 0;
    while (token != NULL)
    {
        path_tokens[i] = token;
        token = strtok(NULL, "/");
        i++;
    }
    path_tokens[i] = NULL;
    path_tokens = realloc(path_tokens, sizeof(char *) * (i + 1));

    return path_tokens;
}

int fs_create(char *path, int is_directory)
{

    if (MOUNT_FLAG == 0)
    {
        // mount err
        return -1;
    }

    char **path_tokens = path_parser(path);
    union block inode_block;
    disk_read(SUPERBLOCK.superblock.s_inode_table_block_start, inode_block.data);
    struct inode file_inode = inode_block.inodes[0];

    union block dir_data_block;
    disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);
  
    for (int i = 0; path_tokens[i] != NULL; i++)
    {

        // last token
        if (path_tokens[i + 1] == NULL)
        {
            
            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
            {
                if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
                {
                    // exists already
                    return 0;  
                }
            }

            // create
            u_int32_t ret_inode_number = 0;
            int success = util_create_dir_file(&inode_block, &file_inode, &dir_data_block,path_tokens, i, is_directory, &ret_inode_number);

            if (success == -1)
            {
                
                return -1;
            }
            else
            {
                // created
                return 0;
            }

           
        }

        // intermediate
        bool file_exists = false; 
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
        {
            if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
            {

              
                uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;
                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
                disk_read(inode_block_number, inode_block.data);
                file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];
                disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);
                file_exists = true;
                break;
            }
        }
        if (file_exists == true)
        {
            continue;
        }

        // make intermediate dir
        uint32_t ret_inode_number = 0;
        int success = util_create_dir_file(&inode_block, &file_inode, &dir_data_block, path_tokens, i, 1, &ret_inode_number);

        if (success == -1)
        {
            
            return -1;
        }
    }
    return -1; 
}

int fs_remove(char *path)
{
    if (MOUNT_FLAG == 0)
    {
        // mount err
        return -1;
    }

    char **path_tokens = path_parser(path);
    union block inode_block;
    disk_read(SUPERBLOCK.superblock.s_inode_table_block_start, inode_block.data);
    struct inode file_inode = inode_block.inodes[0];

    union block dir_data_block;
    disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);

    for (int i = 0; path_tokens[i] != NULL; i++)
    {

        // last file/dir
        if (path_tokens[i + 1] == NULL)
        {
           
            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
            {
                if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
                {
                    // exists
                    uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;
                    uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
                    disk_read(inode_block_number, inode_block.data);
                    struct inode file_inode_copy = file_inode; 
                    file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];

                    if (file_inode.i_is_directory == 0)
                    {
                        // deleting file
                     
                        for (int k = 0; k < INODE_DIRECT_POINTERS; k++)
                        {
                            if (file_inode.i_direct_pointers[k] != 0)
                            {
                                BLOCK_BITMAP.bitmap[file_inode.i_direct_pointers[k]] = 0;
                            }
                        }
                        if (file_inode.i_single_indirect_pointer != 0)
                        {
                            union block single_indirect_block;
                            disk_read(file_inode.i_single_indirect_pointer, single_indirect_block.data);
                            for (u_int32_t k = 0; k < INODE_INDIRECT_POINTERS_PER_BLOCK; k++)
                            {
                                if (single_indirect_block.pointers[k] != 0)
                                {
                                    BLOCK_BITMAP.bitmap[single_indirect_block.pointers[k]] = 0;
                                }
                            }
                            BLOCK_BITMAP.bitmap[file_inode.i_single_indirect_pointer] = 0;
                        }

                        disk_write(SUPERBLOCK.superblock.s_block_bitmap, BLOCK_BITMAP.data);

                        INODE_BITMAP.bitmap[inode_number] = 0;
                        disk_write(SUPERBLOCK.superblock.s_inode_bitmap, INODE_BITMAP.data);
                        dir_data_block.directory_block.entries[j].inode_number = 0;
                        strcpy(dir_data_block.directory_block.entries[j].name, "");
                        disk_write(file_inode_copy.i_direct_pointers[0], dir_data_block.data);
                        return 0;



                    }else { // delete directory

                        int success = util_recursive_remove(inode_number, file_inode, inode_block, dir_data_block, j, file_inode_copy);

                        return success;
                    }
                }
            }
            
            return -1;

            

            
        }

        // intermediate file/dir
        bool file_exists = false; 
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
        {
            if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
            {
                uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;

                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;

                disk_read(inode_block_number, inode_block.data);

                file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];
                disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);    

                if (file_inode.i_is_directory == 0)
                {
                    
                    return -1;
                }
                file_exists = true;
                break;
            }

        }
        if (file_exists == true)
        {
            continue;
        } else {
            //  pathe does not exist
            return -1;
        }
    }
    return -1;
}

int fs_read(char *path, void *buf, size_t count, off_t offset)
{

    if (MOUNT_FLAG == 0)
    {
        return -1;
    }

    char **path_tokens = path_parser(path); 

    union block inode_block;
    disk_read(SUPERBLOCK.superblock.s_inode_table_block_start, inode_block.data);
    struct inode file_inode = inode_block.inodes[0];

    union block dir_data_block;
    disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);

    for (int i = 0; path_tokens[i] != NULL; i++)
    {

        if (path_tokens[i + 1] == NULL)
        {
            // check if the file already exists
            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
            {
                if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
                {

                    uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;

                    uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
                    disk_read(inode_block_number, inode_block.data);
                    file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];

                    if (file_inode.i_is_directory == 1)
                    {
                        // cant read dir
                        return -1;
                    }

                    int success = util_read_loop(file_inode, buf, count, offset);
                    if (success == -1)
                    {
                        // error reading
                        return -1;
                    }
                    else
                    {
                        // return bytes
                        return success;
                    }
                }
            }
            return -1;
        }

        bool file_exists = false; 
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
        {
            if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
            {

            
                uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;
                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
                disk_read(inode_block_number, inode_block.data);
                file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];
                disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);
                file_exists = true;
                break;
            }
        }
        if (file_exists == false)
        {
            // path incorrect
            return -1;
        }
    }
    return -1;
}

int fs_write(char *path, void *buf, size_t count, off_t offset)
{
   
    if (MOUNT_FLAG == 0)
    {
       
        return -1;
    }

    char **path_tokens = path_parser(path);

 
    union block inode_block;
    disk_read(SUPERBLOCK.superblock.s_inode_table_block_start, inode_block.data);
    struct inode file_inode = inode_block.inodes[0];

    union block dir_data_block;
    disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);

    for (int i = 0; path_tokens[i] != NULL; i++)
    {

    
        if (path_tokens[i + 1] == NULL)
        {
            // check if the file already exists
            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
            {
                if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
                {

                    uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;
                    uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
                    disk_read(inode_block_number, inode_block.data);
                    file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];

      
                    if (file_inode.i_is_directory == 1)
                    {
                        //  write to dir ?
                        return -1;
                    }

                    int suc = util_write_loop(inode_number, file_inode, inode_block, buf, count, offset);
                    if (suc == -1)
                    {
                        
                        return -1;
                    }
                    else
                    {
                        
                        return 0;
                    }
                }
            }

            // create a file
            uint32_t ret_inode_number = 0;
            int success = util_create_dir_file(&inode_block, &file_inode, &dir_data_block, path_tokens, i, 0, &ret_inode_number);

            if (success == -1)
            {
                
                return -1;
            }
            else
            {
               
                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + ret_inode_number / INODES_PER_BLOCK;
              
                disk_read(inode_block_number, inode_block.data);
              
                file_inode = inode_block.inodes[ret_inode_number % INODES_PER_BLOCK];
                int suc = util_write_loop(ret_inode_number, file_inode, inode_block, buf, count, offset);
                if (suc == -1)
                {
                    // printf("Error: Could not write to file.\n");
                    return -1;
                }
                else
                {
                    
                    return 0;
                }
            }

            
        }

        // intermediat
        bool file_exists = false; 
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
        {
            if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
            {

        
                uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;
                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;

                disk_read(inode_block_number, inode_block.data);
   
                file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];
                disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);
  
                file_exists = true;
                break;
            }

           
        }
        if (file_exists == true)
        {
            continue;
        }

        // crete intermediate dir
        uint32_t ret_inode_number = 0;
        int success = util_create_dir_file(&inode_block, &file_inode, &dir_data_block, path_tokens, i, 1, &ret_inode_number);

        if (success == -1)
        {
            
            return -1;
        }
    }
    return -1; // empty path
}

int fs_list(char *path)
{
  
    if (MOUNT_FLAG == 0)
    {
       
        return -1;
    }

    char **path_tokens = path_parser(path);

   
    union block inode_block;
    disk_read(SUPERBLOCK.superblock.s_inode_table_block_start, inode_block.data);
    struct inode file_inode = inode_block.inodes[0];

    union block dir_data_block;
    disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);


//     if root path deal outside
    if (path_tokens[0] == NULL)
    {
        // print all entries
        // printf("here");
        for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
        {
            if (dir_data_block.directory_block.entries[i].inode_number != 0)
            {
                printf("%s ", dir_data_block.directory_block.entries[i].name);
             
                uint32_t inode_number = dir_data_block.directory_block.entries[i].inode_number;
            
                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
            
                disk_read(inode_block_number, inode_block.data);
             
                file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];

                printf("%ld\n", file_inode.i_size);
            }
        }
        return 0;
    }


    for (int i = 0; path_tokens[i] != NULL; i++)
    {

     
        if (path_tokens[i + 1] == NULL)
        {
            // check if the file already exists
            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
            {
                if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
                {
                   
                    uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;
        
                    uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
               
                    disk_read(inode_block_number, inode_block.data);
                  
                    file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];

                    
                    if (file_inode.i_is_directory == 0)
                    {
                       
                        return -1;
                    }

              
                    disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);
              
                    for (int k = 0; k < DIRECTORY_ENTRIES_PER_BLOCK; k++)
                    {
                        if (dir_data_block.directory_block.entries[k].inode_number != 0)
                        {
                            printf("%s ", dir_data_block.directory_block.entries[k].name);
                          
                            uint32_t inode_number = dir_data_block.directory_block.entries[k].inode_number;
                         
                            uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
                     
                            disk_read(inode_block_number, inode_block.data);
                         
                            file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];

                            printf("%ld\n", file_inode.i_size);
                        }
                    }
                    return 0;
                }
            }
            
            return -1;

            

            
        }

       
        bool file_exists = false; 
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
        {
            if (strcmp(dir_data_block.directory_block.entries[j].name, path_tokens[i]) == 0)
            {

            
                uint32_t inode_number = dir_data_block.directory_block.entries[j].inode_number;
                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
            
                disk_read(inode_block_number, inode_block.data);
       
                file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];
                disk_read(file_inode.i_direct_pointers[0], dir_data_block.data);    

                if (file_inode.i_is_directory == 0)
                {
                    
                    return -1;
                }

               
                file_exists = true;
                break;
            }

    
        }
        if (file_exists == true)
        {
            continue;
        } else {
            // printf("Error: File does not exist.\n");
            return -1;
        }
    }
    return -1; 
}

void fs_stat()
{
    if (MOUNT_FLAG == 0)
    {
        
        return;
    }

    printf("Superblock:\n");
    printf("    Blocks: %d\n", SUPERBLOCK.superblock.s_blocks_count);
    printf("    Inodes: %d\n", SUPERBLOCK.superblock.s_inodes_count);
    printf("    Inode Table Block Start: %d\n", SUPERBLOCK.superblock.s_inode_table_block_start);
    printf("    Data Blocks Start: %d\n", SUPERBLOCK.superblock.s_data_blocks_start);
    printf("    Block Bitmap: %d\n", SUPERBLOCK.superblock.s_block_bitmap);
    printf("    Inode Bitmap: %d\n", SUPERBLOCK.superblock.s_inode_bitmap);
}

int util_create_dir_file(union block *inode_block, struct inode *file_inode, union block *dir_data_block, char **path_tokens, int i, int is_directory, uint32_t *ret_inode_number)
{
   
    union block inode_block_copy;
    memcpy(inode_block_copy.data, inode_block->data, BLOCK_SIZE);
    


    uint32_t inode_number = 0;
    for (uint32_t j = 0; j < SUPERBLOCK.superblock.s_inodes_count; j++)
    {
        if (INODE_BITMAP.bitmap[j] == 0)
        {
            inode_number = j;
            *ret_inode_number = j;
            break;
        }
    }
    if (inode_number == 0)
    {
       
        return -1;
    }


    uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;
    disk_read(inode_block_number, inode_block->data);

    if (is_directory == 1) 
    {
      
        uint32_t block_number = 0;
        for (uint32_t j = 0; j < SUPERBLOCK.superblock.s_blocks_count; j++)
        {
            if (BLOCK_BITMAP.bitmap[j] == 0)
            {
                block_number = j;
    
                INODE_BITMAP.bitmap[inode_number] = 1;
                disk_write(SUPERBLOCK.superblock.s_inode_bitmap, INODE_BITMAP.data);
                break;
            }
        }
        if (block_number == 0)
        {
            // printf("Error: No empty blocks.\n");
            return -1;
        }

        BLOCK_BITMAP.bitmap[block_number] = 1;
        disk_write(SUPERBLOCK.superblock.s_block_bitmap, BLOCK_BITMAP.data);
        inode_block->inodes[inode_number % INODES_PER_BLOCK].i_is_directory = is_directory;
        inode_block->inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[0] = block_number;
        inode_block->inodes[inode_number % INODES_PER_BLOCK].i_size = BLOCK_SIZE;
        disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block->data);

        union block dir_data_block_new;
        memset(dir_data_block_new.data, 0, BLOCK_SIZE);
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
        {
            dir_data_block_new.directory_block.entries[j].inode_number = 0;
            strcpy(dir_data_block_new.directory_block.entries[j].name, "");
        }
        disk_write(block_number, dir_data_block_new.data);

   
        int entry_number = -1;
        for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
        {
            if (dir_data_block->directory_block.entries[j].inode_number == 0)
            {
                entry_number = j;
                break;
            }
        }
        if (entry_number == -1)
        {
            // printf("Error: No empty directory entries.\n");
            return -1;
        }

        dir_data_block->directory_block.entries[entry_number].inode_number = inode_number;
        strcpy(dir_data_block->directory_block.entries[entry_number].name, path_tokens[i]);
        disk_write(file_inode->i_direct_pointers[0], dir_data_block->data);
        *file_inode = inode_block->inodes[inode_number % INODES_PER_BLOCK];

        *dir_data_block = dir_data_block_new;

        return 0;
    }

    // file creation only happens at end

    INODE_BITMAP.bitmap[inode_number] = 1;
    disk_write(SUPERBLOCK.superblock.s_inode_bitmap, INODE_BITMAP.data);

    inode_block->inodes[inode_number % INODES_PER_BLOCK].i_is_directory = is_directory;

    inode_block->inodes[inode_number % INODES_PER_BLOCK].i_size = 0;

    for (int j = 0; j < INODE_DIRECT_POINTERS; j++)
    {
        inode_block->inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[j] = 0;
    }
    inode_block->inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer = 0;
    inode_block->inodes[inode_number % INODES_PER_BLOCK].i_double_indirect_pointer = 0;


    disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block->data);

    int entry_number = -1;
    for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++) 
    {
        if (dir_data_block->directory_block.entries[j].inode_number == 0)
        {
            entry_number = j;
            break;
        }
    }
    if (entry_number == -1)
    {
        // printf("Error: No empty directory entries.\n");
        return -1;
    }
  
    dir_data_block->directory_block.entries[entry_number].inode_number = inode_number;
    strcpy(dir_data_block->directory_block.entries[entry_number].name, path_tokens[i]);
    disk_write(file_inode->i_direct_pointers[0], dir_data_block->data);

    return 0;
}

int util_read_loop(struct inode file_inode, void *buf, size_t count, off_t offset)
{
    int bytes_read = 0;
    if (count == 0)
    {
        return -1;
    }
    if (offset > file_inode.i_size)
    {
        return -1;
    }

    // if offset + count > file size , todo : ask abdullah
    if (offset + count > file_inode.i_size)
    {
        count = file_inode.i_size - offset;
    }

    // possible starting position for offset
    if (offset < BLOCK_SIZE * INODE_DIRECT_POINTERS)
    {

        uint32_t starting_block = offset / BLOCK_SIZE;
        offset = offset % BLOCK_SIZE;

      
        union block file_data_block;
        disk_read(file_inode.i_direct_pointers[starting_block], file_data_block.data);
 

        if (count < BLOCK_SIZE - offset)
        {
            memcpy(buf, file_data_block.data + offset, count);
            bytes_read += count;
            return bytes_read;
        }
        else
        {
            memcpy(buf, file_data_block.data + offset, BLOCK_SIZE - offset);
            buf += BLOCK_SIZE - offset;
            bytes_read += BLOCK_SIZE - offset;
            count -= BLOCK_SIZE - offset;
        }
        for (uint32_t i = starting_block + 1; i < INODE_DIRECT_POINTERS; i++)
        {
            if (count == 0)
            {
                return bytes_read;
            }

            if (count < BLOCK_SIZE)
            {
                disk_read(file_inode.i_direct_pointers[i], file_data_block.data);
                memcpy(buf, file_data_block.data, count);
                bytes_read += count;
                return bytes_read;
            }
            else
            {
                memcpy(buf, file_data_block.data, BLOCK_SIZE);
                buf += BLOCK_SIZE;
                bytes_read += BLOCK_SIZE;
                count -= BLOCK_SIZE;
            }
        }

        // read indirect block
        union block indirect_block;
        disk_read(file_inode.i_single_indirect_pointer, indirect_block.data);
        for (uint32_t i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            if (count == 0)
            {
                return bytes_read;
            }

            if (count < BLOCK_SIZE)
            {
                disk_read(indirect_block.pointers[i], file_data_block.data);
                memcpy(buf, file_data_block.data, count);
                bytes_read += count;
                return bytes_read;
            }
            else
            {
                memcpy(buf, file_data_block.data, BLOCK_SIZE);
                buf += BLOCK_SIZE;
                bytes_read += BLOCK_SIZE;
                count -= BLOCK_SIZE;
            }
        }

        // diff starting point from indirect blocks
    } 
    else if (offset < BLOCK_SIZE * (INODE_DIRECT_POINTERS + INODE_INDIRECT_POINTERS_PER_BLOCK)) 
    {
       
        union block indirect_block;
        disk_read(file_inode.i_single_indirect_pointer, indirect_block.data);
      
        uint32_t starting_block = (offset - BLOCK_SIZE * INODE_DIRECT_POINTERS) / BLOCK_SIZE;

        offset = offset % BLOCK_SIZE;

  

        union block file_data_block;
        disk_read(indirect_block.pointers[starting_block], file_data_block.data);
        if (count < BLOCK_SIZE - offset)
        {
            memcpy(buf, file_data_block.data + offset, count);
            bytes_read += count;
            return bytes_read;
        }
        else
        {
            memcpy(buf, file_data_block.data + offset, BLOCK_SIZE - offset);
            buf += BLOCK_SIZE - offset;
            bytes_read += BLOCK_SIZE - offset;
            count -= BLOCK_SIZE - offset;
        }




        for (uint32_t i = starting_block + 1; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            if (count == 0)
            {
                return bytes_read;
            }

            if (count < BLOCK_SIZE)
            {
                disk_read(indirect_block.pointers[i], file_data_block.data);
                memcpy(buf, file_data_block.data, count);
                bytes_read += count;
                return bytes_read;
            }
            else
            {
                memcpy(buf, file_data_block.data, BLOCK_SIZE);
                buf += BLOCK_SIZE;
                bytes_read += BLOCK_SIZE;
                count -= BLOCK_SIZE;
            }
        }

        
    }
    else
    {
       //double indirect implemented earlier before announcement :(
    }
    if (count == 0)
    {
        return bytes_read;
    } else {
        return -1;
    }
}

int util_find_free_block_num(uint32_t *block_number)
{
    for (uint32_t j = 0; j < SUPERBLOCK.superblock.s_blocks_count; j++)
    {
        if (BLOCK_BITMAP.bitmap[j] == 0)
        {
            *block_number = j;
            // set block bitmap
            BLOCK_BITMAP.bitmap[*block_number] = 1;
            disk_write(SUPERBLOCK.superblock.s_block_bitmap, BLOCK_BITMAP.data);
            return 0;
        }
    }
    return -1;
}

int util_write_loop(uint32_t inode_number, struct inode file_inode, union block inode_block, void *buf, size_t count, off_t offset)
{
  
    if (count == 0)
    {
        return -1;
    }

    
    if (offset  > file_inode.i_size)  //in first write assuming offset and size zero
    {
        return -1;
    }

    if (file_inode.i_size == 0){

       
        
      
        for (uint32_t i = 0; i < INODE_DIRECT_POINTERS; i++)
        {
            if (count == 0)
            {
                return 0;
            }

            if (count < BLOCK_SIZE)
            {
                uint32_t block_number = 0;
                int success = util_find_free_block_num(&block_number);
                if (success == -1)  
                {
                   
                    return -1;
                }
                
               
                union block file_data_block;
                memcpy(file_data_block.data, buf, count);
                disk_write(block_number, file_data_block.data);

      
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += count;
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i] = block_number;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                return 0;
            }
            else
            {
            
                uint32_t block_number = 0;
                int success = util_find_free_block_num(&block_number);
                if (success == -1) 
                {
                    // printf("Error: No empty blocks.\n");
                    return -1;
                }

                union block file_data_block;
                memcpy(file_data_block.data, buf, BLOCK_SIZE);
                disk_write(block_number, file_data_block.data);

                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE;
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i] = block_number;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                buf += BLOCK_SIZE;
                count -= BLOCK_SIZE;
            }
        }

        // indirect layer
        uint32_t indirect_block_number = 0;
        int success = util_find_free_block_num(&indirect_block_number);
        if (success == -1)  
        {
            
            return -1;
        }

        
        // inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE; // should i_size be file size or indirect block size . todo
        inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer = indirect_block_number;
        disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

        union block indirect_block;
        memset(indirect_block.data, 0, BLOCK_SIZE);

        for (uint32_t i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            if (count == 0)
            {
                return 0;
            }

            if (count < BLOCK_SIZE)
            {
                uint32_t block_number = 0;
                int success = util_find_free_block_num(&block_number);
                if (success == -1)  // partial write
                {
                    
                    return -1;
                }

           
                union block file_data_block;
                memcpy(file_data_block.data, buf, count);
                disk_write(block_number, file_data_block.data);
                indirect_block.pointers[i] = block_number;
                disk_write(indirect_block_number, indirect_block.data);
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += count;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                return 0;
            }
            else
            {
            
                uint32_t block_number = 0;
                int success = util_find_free_block_num(&block_number);
                if (success == -1)  
                {
                    return -1;
                }

                union block file_data_block;
                memcpy(file_data_block.data, buf, BLOCK_SIZE);
                disk_write(block_number, file_data_block.data);

                indirect_block.pointers[i] = block_number;
                disk_write(indirect_block_number, indirect_block.data);
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                buf += BLOCK_SIZE;
                count -= BLOCK_SIZE;
            }
        }

        
        

    } else 
    {

        // first find location where to start direct block, single indirect block or double indirect block

        if (offset < BLOCK_SIZE * INODE_DIRECT_POINTERS)
        {

            uint32_t starting_block = offset / BLOCK_SIZE;
            offset = offset % BLOCK_SIZE;
            union block file_data_block;
            disk_read(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[starting_block], file_data_block.data);
            if (count < BLOCK_SIZE - offset)
            {
                memcpy(file_data_block.data + offset, buf, count);
                disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[starting_block], file_data_block.data);
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += count;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);
                return 0;
            }
            else
            {
                memcpy(file_data_block.data + offset, buf, BLOCK_SIZE - offset);
                disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[starting_block], file_data_block.data);

                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE - offset;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                buf += BLOCK_SIZE - offset;
                count -= BLOCK_SIZE - offset;
            }
            for (uint32_t i = starting_block + 1; i < INODE_DIRECT_POINTERS; i++)
            {
                if (count == 0)
                {
                    return 0;
                }

                if (count < BLOCK_SIZE)
                {
              
                    if (inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i] == 0)
                    {
                        uint32_t block_number = 0;
                        int success = util_find_free_block_num(&block_number);
                        if (success == -1)  // partial write
                        {
                          
                            return -1;
                        }
                        inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i] = block_number;
                        disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);
                    }
                    
                    disk_read(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i], file_data_block.data);
                    memcpy(file_data_block.data, buf, count);
                    disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i], file_data_block.data);
                    return 0;
                }
                else
                {
                     if (inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i] == 0)
                    {
                        uint32_t block_number = 0;
                        int success = util_find_free_block_num(&block_number);
                        if (success == -1)  
                        {
                            
                            return -1;
                        }

                       
                        inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i] = block_number;
                        disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);
                    }
                    
                    memcpy(file_data_block.data, buf, BLOCK_SIZE);
                    disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_direct_pointers[i], file_data_block.data);
                    inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE;
                    disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);
                    buf += BLOCK_SIZE;
                    count -= BLOCK_SIZE;
                }
            }

            if (inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer == 0)
            {
                uint32_t block_number = 0;
                int success = util_find_free_block_num(&block_number);
                if (success == -1)  
                {
                   
                    return -1;
                }

                
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer = block_number;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                union block indirect_block;
                for (u_int32_t i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
                {
                    indirect_block.pointers[i] = 0;
                }
                disk_write(block_number, indirect_block.data);
            }
            

            // read from indirect block
            union block indirect_block;
            disk_read(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer, indirect_block.data);
            for (uint32_t i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                if (count == 0)
                {
                    return 0;
                }

                if (count < BLOCK_SIZE)
                {
              
                    if (indirect_block.pointers[i] == 0)
                    {
                        uint32_t block_number = 0;
                        int success = util_find_free_block_num(&block_number);
                        if (success == -1) 
                        {
                          
                            return -1;
                        }

                       
                        indirect_block.pointers[i] = block_number;
                        disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer, indirect_block.data);
                    }
                    
                    disk_read(indirect_block.pointers[i], file_data_block.data);
                    memcpy(file_data_block.data, buf, count);
                    disk_write(indirect_block.pointers[i], file_data_block.data);


                
                    inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += count;
                    disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);
                    return 0;
                }
                else
                {
                    if (indirect_block.pointers[i] == 0)
                    {
                        uint32_t block_number = 0;
                        int success = util_find_free_block_num(&block_number);
                        if (success == -1)  
                        {
                      
                            return -1;
                        }

                        
                        indirect_block.pointers[i] = block_number;
                        disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer, indirect_block.data);
                    }
                    
                    memcpy(file_data_block.data, buf, BLOCK_SIZE);
                    disk_write(indirect_block.pointers[i], file_data_block.data);

               
                    inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE;
                    disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                    buf += BLOCK_SIZE;
                    count -= BLOCK_SIZE;
                }
            }

            
        }
         else if (offset < BLOCK_SIZE * (INODE_DIRECT_POINTERS + INODE_INDIRECT_POINTERS_PER_BLOCK))
        {
            
            union block indirect_block;
        
            uint32_t starting_block = (offset - BLOCK_SIZE * INODE_DIRECT_POINTERS) / BLOCK_SIZE;
     
            offset = offset % BLOCK_SIZE;

      
            disk_read(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer, indirect_block.data);

            union block file_data_block;

            if (count < BLOCK_SIZE - offset)
            {
                
                
                disk_read(indirect_block.pointers[starting_block], file_data_block.data);
                memcpy(file_data_block.data + offset, buf, count);
                disk_write(indirect_block.pointers[starting_block], file_data_block.data);

           
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += count;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);
                return 0;
            }
            else
            {
                
                disk_read(indirect_block.pointers[starting_block], file_data_block.data);
                memcpy(file_data_block.data + offset, buf, BLOCK_SIZE - offset);
                disk_write(indirect_block.pointers[starting_block], file_data_block.data);

        
                inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE - offset;
                disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                buf += BLOCK_SIZE - offset;
                count -= BLOCK_SIZE - offset;
            }
            for (uint32_t i = starting_block + 1; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                if (count == 0)
                {
                    return 0;
                }

                if (count < BLOCK_SIZE)
                {
                   
                    if (indirect_block.pointers[i] == 0)
                    {
                        uint32_t block_number = 0;
                        int success = util_find_free_block_num(&block_number);
                        if (success == -1)  // partial write
                        {
                       
                            return -1;
                        }

               
                        indirect_block.pointers[i] = block_number;
                        disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer, indirect_block.data);
                    }
                    
                    disk_read(indirect_block.pointers[i], file_data_block.data);
                    memcpy(file_data_block.data, buf, count);
                    disk_write(indirect_block.pointers[i], file_data_block.data);


                    // update inode
                    inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += count;
                    disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);
                    return 0;
                }
                else
                {
                    if (indirect_block.pointers[i] == 0)
                    {
                        uint32_t block_number = 0;
                        int success = util_find_free_block_num(&block_number);
                        if (success == -1)  // partial write
                        {
                       
                            return -1;
                        }

                  
                        indirect_block.pointers[i] = block_number;
                        disk_write(inode_block.inodes[inode_number % INODES_PER_BLOCK].i_single_indirect_pointer, indirect_block.data);
                    }
                    
                    memcpy(file_data_block.data, buf, BLOCK_SIZE);
                    disk_write(indirect_block.pointers[i], file_data_block.data);

              
                    inode_block.inodes[inode_number % INODES_PER_BLOCK].i_size += BLOCK_SIZE;
                    disk_write(SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK, inode_block.data);

                    buf += BLOCK_SIZE;
                    count -= BLOCK_SIZE;
                }
            }
            


        }else {
        // double ndirect
               

        }


    
                
        
    }

    return -1; //todo


}


int util_recursive_remove(uint32_t child_inode_number, struct inode child_file_inode, union block child_inode_block, union block parent_dir_data_block, int parent_entry, struct inode parent_inode)
{
    // if file is a directory
    if (child_file_inode.i_is_directory == 1)
    {
        union block dir_data_block_new;
        disk_read(child_file_inode.i_direct_pointers[0], dir_data_block_new.data);

        for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
        {
            if (dir_data_block_new.directory_block.entries[i].inode_number != 0)
            {
             
                uint32_t inode_number = dir_data_block_new.directory_block.entries[i].inode_number;
                uint32_t inode_block_number = SUPERBLOCK.superblock.s_inode_table_block_start + inode_number / INODES_PER_BLOCK;

                union block inode_block;
                disk_read(inode_block_number, inode_block.data);

                struct inode file_inode = inode_block.inodes[inode_number % INODES_PER_BLOCK];
                int new_parent_entry = i;

                util_recursive_remove(inode_number, file_inode, inode_block, dir_data_block_new, new_parent_entry, child_file_inode);
            }
        }
    } else {

        //  base case leaf node
        for (int k = 0; k < INODE_DIRECT_POINTERS; k++)
        {
            if (child_file_inode.i_direct_pointers[k] != 0)
            {
                BLOCK_BITMAP.bitmap[child_file_inode.i_direct_pointers[k]] = 0;
            }
        }
        if (child_file_inode.i_single_indirect_pointer != 0)
        {
            union block single_indirect_block;
            disk_read(child_file_inode.i_single_indirect_pointer, single_indirect_block.data);
            for (int k = 0; k < INODE_INDIRECT_POINTERS_PER_BLOCK; k++)
            {
                if (single_indirect_block.pointers[k] != 0)
                {
                    BLOCK_BITMAP.bitmap[single_indirect_block.pointers[k]] = 0;
                }
            }
            BLOCK_BITMAP.bitmap[child_file_inode.i_single_indirect_pointer] = 0;
        }

        disk_write(SUPERBLOCK.superblock.s_block_bitmap, BLOCK_BITMAP.data);
        INODE_BITMAP.bitmap[child_inode_number] = 0;
        disk_write(SUPERBLOCK.superblock.s_inode_bitmap, INODE_BITMAP.data);

        parent_dir_data_block.directory_block.entries[parent_entry].inode_number = 0;
        strcpy(parent_dir_data_block.directory_block.entries[parent_entry].name, "");
        disk_write(parent_inode.i_direct_pointers[0], parent_dir_data_block.data);

        return 0;


    }

    // update currrents' parent stuff
    BLOCK_BITMAP.bitmap[child_file_inode.i_direct_pointers[0]] = 0;
    disk_write(SUPERBLOCK.superblock.s_block_bitmap, BLOCK_BITMAP.data);

    INODE_BITMAP.bitmap[child_inode_number] = 0;
    disk_write(SUPERBLOCK.superblock.s_inode_bitmap, INODE_BITMAP.data);

    parent_dir_data_block.directory_block.entries[parent_entry].inode_number = 0;
    strcpy(parent_dir_data_block.directory_block.entries[parent_entry].name, "");
    disk_write(parent_inode.i_direct_pointers[0], parent_dir_data_block.data);

    return 0;
}

uint32_t util_calc_inode_blocks(uint32_t inodes_count){
    uint32_t inode_blocks = ceil ((double)inodes_count / INODES_PER_BLOCK);
    return inode_blocks;
}