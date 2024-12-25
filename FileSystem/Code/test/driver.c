#include "fs.h"
#include "disk.h"

#include <string.h>
#include <stdlib.h>

char *DISK = "disk.img";
int DISK_SIZE = 16;

int main()
{
    // Initialize the disk.
    if (disk_init(DISK, DISK_SIZE) == -1)
    {
        printf("ERROR: Could not initialize disk.\n");
        return -1;
    }

    // Format the disk.
    if (fs_format() == -1)
    {
        printf("ERROR: Could not format disk.\n");
        return -1;
    }

    // Mount the disk.
    if (fs_mount() == -1)
    {
        printf("ERROR: Could not mount disk.\n");
        return -1;
    }

    // Your testing code goes here.

    // Close the disk.
    if (disk_close() == -1)
    {
        printf("ERROR: Could not close disk.\n");
        return -1;
    }

    return 0;
}