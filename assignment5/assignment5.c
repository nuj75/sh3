#include "fs_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

struct FileSystem fs;

void init_FS()
{
    struct VolumeControlBlock *vcb = malloc(sizeof(struct VolumeControlBlock));
    vcb->num_files_made = 0;

    struct FreeBlockList *fb_list = malloc(sizeof(struct FreeBlockList));
    fb_list->count = TOTAL_BLOCKS;
    fb_list->head = (struct FreeBlockNode *)-1;
    for (int i = TOTAL_BLOCKS - 1; i >= 0; i--)
    {
        struct block *new_block = malloc(sizeof(struct block));
        new_block->block_number = i;
        vcb->blocks[i] = *new_block;

        struct FreeBlockNode *node = malloc(sizeof(struct FreeBlockNode));
        node->blk = new_block;
        if (fb_list->head != (struct FreeBlockNode *)-1)
            node->next = fb_list->head;
        fb_list->head = node;
    }

    vcb->free_block_list = *fb_list;
    fs.vcb = *vcb;
}
int getFileInformationBlock()
{
    return fs.vcb.num_files_made;
}

// take the first block in the free block list. update the head of the file
// return the block
struct block *allocateFreeBlock()
{
    struct block *returnBlock = fs.vcb.free_block_list.head->blk;
    fs.vcb.free_block_list.head = fs.vcb.free_block_list.head->next;
    fs.vcb.free_block_list.count -= 1;

    return returnBlock;
}

// make sure that there aren't more files than allowed
// make sure that there are enough blocks
// take blocks off of the free block list. use the allocateFreeBlock function
// set the block count, set an id, the name for the file and make an index block
//      put this information into a file information block
// increment files made integer
void createFile(const char *filename, int size)
{
    if (fs.vcb.num_files_made == MAX_FILES)
    {
        printf("Max files created.\n");
        return;
    }

    if (fs.vcb.free_block_list.count * BLOCK_SIZE < size)
    {
        printf("Not enough space\n");
        return;
    }

    int block_count = ceil((double)size / (double)BLOCK_SIZE);
    struct block *index_block = allocateFreeBlock();
    for (int i = 0; i < block_count; i++)
    {
        struct block *block = allocateFreeBlock();
        memcpy(index_block->data + i * 8, block, 8);
    }
    int file_id = getFileInformationBlock();
    fs.vcb.files[file_id] = malloc(sizeof(struct block));

    fs.vcb.files[file_id]->id = file_id;
    snprintf(fs.vcb.files[file_id]->name, sizeof(fs.vcb.files[file_id]->name), "%s", filename);
    fs.vcb.files[file_id]->block_count = block_count;
    fs.vcb.files[file_id]->index_block = index_block;
    fs.vcb.files[file_id]->file_size = size;

    fs.vcb.num_files_made += 1;
}

void listFiles()
{
    printf("Root Directory Listing (%d files)\n", fs.vcb.num_files_made);

    for (int i = 0; i < fs.vcb.num_files_made; i++)
    {

        printf("%s", fs.vcb.files[i]->name);
        for (int j = 0; j < 15 - strlen(fs.vcb.files[i]->name); j++)
        {
            printf(" ");
        }

        printf("|");

        for (int j = 0; j < 15 - log10(fs.vcb.files[i]->file_size) - 6; j++)
        {
            printf(" ");
        }

        printf("%d bytes |", fs.vcb.files[i]->file_size);

        printf("  %d data blocks", fs.vcb.files[i]->block_count);
        for (int j = 0; j < 25 - 14 - log10(fs.vcb.files[i]->block_count); j++)
        {
            printf(" ");
        }

        printf(" FIB ID:=%d\n", fs.vcb.files[i]->id);
    }
}

int main()
{
    init_FS();

    createFile("test.txt", 1025);

    for (int i = 0; i < sizeof(fs.vcb.files[0]->index_block); i++)
    {
        printf("%x\n", fs.vcb.files[0]->index_block->data[i]);
    }

    listFiles();
}