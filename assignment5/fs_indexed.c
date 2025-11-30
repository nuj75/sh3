#include "fs_ops.h"

struct FileSystem fs;
struct FileIds *file_id_head;

struct FileIds
{
    int id;
    struct FileIds *next;
};

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

    file_id_head = malloc(sizeof(struct FileIds));
    file_id_head->id = MAX_FILES - 1;
    for (int i = MAX_FILES - 2; i >= 0; i--)
    {
        struct FileIds *temp = malloc(sizeof(struct FileIds));
        temp->id = i;
        temp->next = file_id_head;
        file_id_head = temp;
    }

    printf("Filesystem initialized with %d blocks of %d bytes each.\n", TOTAL_BLOCKS, BLOCK_SIZE);
}

int getFileInformationBlockId()
{
    struct FileIds *temp = file_id_head;

    int new_id = file_id_head->id;

    file_id_head = file_id_head->next;

    free(temp);

    return new_id;
}

void returnFreeBlock(struct block *return_block)
{
    struct FreeBlockNode *block_node = malloc(sizeof(struct FreeBlockList));

    block_node->blk = return_block;
    block_node->next = fs.vcb.free_block_list.head;

    fs.vcb.free_block_list.head = block_node;
    fs.vcb.free_block_list.count += 1;
}

void printFreeBlocks()
{
    struct FreeBlockNode *block_pointer = fs.vcb.free_block_list.head;

    printf("Free Blocks (%d): ", fs.vcb.free_block_list.count);

    while (block_pointer->next != NULL)
    {
        printf("[%d] ->", block_pointer->blk->block_number);
        block_pointer = block_pointer->next;
    }

    printf("[%d]\n", block_pointer->blk->block_number);
}

// take the first block in the free block list. update the head of the file
// return the block
struct block *allocateFreeBlock()
{
    struct FreeBlockNode *head_block_node = fs.vcb.free_block_list.head;
    struct block *returnBlock = head_block_node->blk;
    fs.vcb.free_block_list.head = fs.vcb.free_block_list.head->next;

    free(head_block_node);
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

    if (fs.vcb.free_block_list.count * BLOCK_SIZE < size || BLOCK_SIZE * BLOCK_SIZE / 8 < size)
    {
        printf("Not enough space\n");
        return;
    }

    int block_count = ceil((double)size / (double)BLOCK_SIZE);
    struct block *index_block = allocateFreeBlock();
    for (int i = 0; i < block_count; i++)
    {
        void *block = allocateFreeBlock();
        memcpy(index_block->data + i * 8, &block, 8);
    }
    int file_id = getFileInformationBlockId();
    int file_index = fs.vcb.num_files_made;
    fs.vcb.files[file_index] = malloc(sizeof(struct FileInformation));

    fs.vcb.files[file_index]->id = file_id;
    snprintf(fs.vcb.files[file_index]->name, sizeof(fs.vcb.files[file_index]->name), "%s", filename);
    fs.vcb.files[file_index]->block_count = block_count + 1;
    fs.vcb.files[file_index]->index_block = index_block;
    fs.vcb.files[file_index]->file_size = size;

    fs.vcb.num_files_made += 1;
}

void deleteFile(const char *filename)
{
    int i = -1;

    // loop that quits if file doesn't exist, and continues with i as the right index otherwise
    while (fs.vcb.files[++i] == NULL || strcmp(fs.vcb.files[i]->name, filename) != 0)
    {
        if (fs.vcb.files[i] != NULL)
        {
            printf("||%s||\n", fs.vcb.files[i]->name);
        }
        if ((i + 1) >= MAX_FILES)
        {
            printf("File not found\n");
            return;
        }
    }

    for (int j = 0; j < fs.vcb.files[i]->block_count - 1; j++)
    {
        struct block *block_to_free;
        memcpy(&block_to_free, fs.vcb.files[i]->index_block->data + j * 8, 8);
        returnFreeBlock(block_to_free);
    }

    returnFreeBlock(fs.vcb.files[i]->index_block);
    free(fs.vcb.files[i]->index_block);

    struct FileIds *new_file_id = malloc(sizeof(struct FileIds));
    new_file_id->id = fs.vcb.files[i]->id;
    new_file_id->next = file_id_head;
    file_id_head = new_file_id;

    free(fs.vcb.files[i]);
    fs.vcb.files[i] = NULL;
    fs.vcb.num_files_made -= 1;

    printf("File '%s' deleted.\n", filename);
}

void listFiles()
{
    printf("Root Directory Listing (%d files)\n", fs.vcb.num_files_made);

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (fs.vcb.files[i] == NULL)
        {
            continue;
        }

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

        printf("  %d data blocks | FIB ID:=%d\n", fs.vcb.files[i]->block_count, fs.vcb.files[i]->id);
    }
}

// incorrect number of blocks
// printinf files is weird
//  fibid are weird