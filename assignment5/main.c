#include "fs_ops.h"

int main()
{
    init_FS();
    printf("\n\n");

    createFile("new1.txt", 4096);
    listFiles();
    printFreeBlocks();

    printf("\n\n");

    createFile("new2.txt", 1023);
    listFiles();
    printFreeBlocks();

    printf("\n\n");

    deleteFile("new1.txt");
    listFiles();
    printFreeBlocks();

    printf("\n\n");

    deleteFile("new2.txt");
    listFiles();
    printFreeBlocks();
}
