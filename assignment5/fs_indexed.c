#include "fs_ops.h"

struct FileSystem fs;
struct FileIds *file_id_head;

struct FileIds
{
    int id;
    struct FileIds *next;
};

/**
 * Initialize file system structure.
 *
 * Create blocks and the free block list.
 * Make File Information Blocks, initialize number of files made.
 * Create available file_id linked list.
 */
void init_FS()
{
    // create vcb to place into file system
    struct VolumeControlBlock *vcb = malloc(sizeof(struct VolumeControlBlock));
    vcb->num_files_made = 0;

    // create blocks and place them into the free block list
    struct FreeBlockList *fb_list = malloc(sizeof(struct FreeBlockList));
    fb_list->count = TOTAL_BLOCKS;
    fb_list->head = (struct FreeBlockNode *)-1;
    for (int i = TOTAL_BLOCKS - 1; i >= 0; i--)
    {
        struct block *new_block = malloc(sizeof(struct block));
        new_block->block_number = i;
        vcb->blocks[i] = *new_block;

        struct FreeBlockNode *node = malloc(sizeof(struct FreeBlockNode)); // freed when block is allocated to file
        node->blk = new_block;
        if (fb_list->head != (struct FreeBlockNode *)-1)
            node->next = fb_list->head;
        fb_list->head = node;
    }
    // put free block list into vcb and the vcb into the file system
    vcb->free_block_list = *fb_list;
    fs.vcb = *vcb;

    // create free file id list
    file_id_head = (struct FileIds *)-1;
    for (int i = MAX_FILES - 1; i >= 0; i--)
    {
        struct FileIds *temp = malloc(sizeof(struct FileIds)); // freed when id is retrieved
        temp->id = i;
        if (file_id_head != (struct FileIds *)-1)
            temp->next = file_id_head;
        file_id_head = temp;
    }

    printf("Filesystem initialized with %d blocks of %d bytes each.\n", TOTAL_BLOCKS, BLOCK_SIZE);
}

/**
 * Retrieve free file id from linked list.
 *
 * Pop FileIds structure from the stack and return it.
 */
int getFileInformationBlockId()
{
    struct FileIds *temp = file_id_head;

    int new_id = file_id_head->id;

    file_id_head = file_id_head->next;

    free(temp);

    return new_id;
}

/**
 * Add a block back into the free block list
 *
 * Make a free block node and place it at the top of the stack
 */
void returnFreeBlock(struct block *return_block)
{
    struct FreeBlockNode *block_node = malloc(sizeof(struct FreeBlockList)); // freed when block is allocated

    block_node->blk = return_block;
    block_node->next = fs.vcb.free_block_list.head;

    fs.vcb.free_block_list.head = block_node;
    fs.vcb.free_block_list.count += 1;
}

/**
 * Print ids of free blocks.
 *
 * Iterate through the free block list and print the ids of the blocks.
 */
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

/**
 * Retrieve a block from the free block list.
 *
 * Pop the top element in the stack and return it.
 */
struct block *allocateFreeBlock()
{
    struct FreeBlockNode *head_block_node = fs.vcb.free_block_list.head;
    struct block *returnBlock = head_block_node->blk;
    fs.vcb.free_block_list.head = fs.vcb.free_block_list.head->next;

    free(head_block_node);
    fs.vcb.free_block_list.count -= 1;

    return returnBlock;
}

/**
 * Create a new file in the file system.
 *
 * check if the system can accommodate the new file.
 * allocate blocks to the file and put pointers into the index block.
 * create file informaiton block and insert information
 */
void createFile(const char *filename, int size)
{
    // check if max_files limit hasn't been hit
    if (fs.vcb.num_files_made == MAX_FILES)
    {
        printf("Max files created.\n");
        return;
    }

    // check if there is enough space for the file.
    // check if the size of the file is small enough so that the
    //      index block can contain all necessary blocks
    if (fs.vcb.free_block_list.count * BLOCK_SIZE < size || BLOCK_SIZE * BLOCK_SIZE / 8 < size)
    {
        printf("Not enough space\n");
        return;
    }

    // allocate blocks and put their addresses into the index block
    int data_block_count = ceil((double)size / (double)BLOCK_SIZE);
    struct block *index_block = allocateFreeBlock();
    for (int i = 0; i < data_block_count; i++)
    {
        void *block = allocateFreeBlock();
        memcpy(index_block->data + i * 8, &block, 8);
    }

    // create file information block and insert appropriate information
    int file_index = fs.vcb.num_files_made;
    fs.vcb.files[file_index] = malloc(sizeof(struct FileInformation)); // freed when file is deleted
    int file_id = getFileInformationBlockId();

    fs.vcb.files[file_index]->id = file_id;
    snprintf(fs.vcb.files[file_index]->name, sizeof(fs.vcb.files[file_index]->name), "%s", filename);
    fs.vcb.files[file_index]->block_count = data_block_count + 1;
    fs.vcb.files[file_index]->index_block = index_block;
    fs.vcb.files[file_index]->file_size = size;

    fs.vcb.num_files_made += 1;
}

/**
 * Delete file from file system.
 *
 * Find file in the file array.
 * Return all data blocks.
 * Push file id into stack.
 * Free FIB struct.
 */
void deleteFile(const char *filename)
{
    int i = -1;

    // Iterate through the file array and break if the file name is the same as the param.
    // If the file is not in the array, then print error message and return
    while (fs.vcb.files[++i] == NULL || strcmp(fs.vcb.files[i]->name, filename) != 0)
    {
        if ((i + 1) >= MAX_FILES)
        {
            printf("File not found\n");
            return;
        }
    }

    // return all data blocks and the index block to the file system
    for (int j = 0; j < fs.vcb.files[i]->block_count - 1; j++)
    {
        struct block *block_to_free;
        memcpy(&block_to_free, fs.vcb.files[i]->index_block->data + j * 8, 8);
        returnFreeBlock(block_to_free);
    }
    returnFreeBlock(fs.vcb.files[i]->index_block);

    // push file id into the FileIds stack
    struct FileIds *new_file_id = malloc(sizeof(struct FileIds)); // freed when file id is retrieved
    new_file_id->id = fs.vcb.files[i]->id;
    new_file_id->next = file_id_head;
    file_id_head = new_file_id;

    // free file information block
    free(fs.vcb.files[i]);
    fs.vcb.files[i] = NULL;
    fs.vcb.num_files_made -= 1;

    printf("File '%s' deleted.\n", filename);
}

/**
 * Print information about all files in the system.
 *
 * Iterate through file array and print information from the FIB struct
 */
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
        for (int j = 0; j < 15 - strlen(fs.vcb.files[i]->name); j++) // align text
        {
            printf(" ");
        }

        printf("|");

        for (int j = 0; j < 15 - log10(fs.vcb.files[i]->file_size) - 6; j++) // align text
        {
            printf(" ");
        }

        printf("%d bytes |", fs.vcb.files[i]->file_size);

        printf("  %d data blocks | FIB ID:=%d\n", fs.vcb.files[i]->block_count, fs.vcb.files[i]->id);
    }
}
