#include "fs_indexed.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

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

    fs.vcb.num_files_made = 0;

    fs.vcb.free_block_list.count = TOTAL_BLOCKS;
    fs.vcb.free_block_list.head = NULL;
    for (int i = TOTAL_BLOCKS - 1; i >= 0; i--)
    {
        fs.vcb.blocks[i].block_number = i;

        struct FreeBlockNode *node = malloc(sizeof(struct FreeBlockNode)); // freed when block is allocated to file
        node->blk = &fs.vcb.blocks[i];

        node->next = fs.vcb.free_block_list.head;
        fs.vcb.free_block_list.head = node;
    }
    // put free block list into vcb and the vcb into the file system
    for (int i = 0; i < MAX_FILES; i++)
    {
        fs.vcb.files[i] = NULL;
        fs.vcb.status[i] = 0;
    }

    // create free file id list
    file_id_head = NULL;
    for (int i = 0; i < MAX_FILES; i++)
    {
        struct FileIds *temp = malloc(sizeof(struct FileIds)); // freed when id is retrieved
        temp->id = i;
        temp->next = file_id_head;
        file_id_head = temp;
    }

    printf("Filesystem initialized with %d blocks of %d bytes each.", TOTAL_BLOCKS, BLOCK_SIZE);
}

/**
 * Retrieve free file id from linked list.
 *
 * Pop FileIds structure from the stack and return it.
 */
int getFileInformationBlockId()
{
    if (file_id_head == NULL)
        return -1;

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
    struct FreeBlockNode *block_node = malloc(sizeof(struct FreeBlockNode)); // freed when block is allocated

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

    while (block_pointer != NULL)
    {
        printf("[%d]", block_pointer->blk->block_number);
        if (block_pointer->next != NULL)
            printf(" ->");
        block_pointer = block_pointer->next;
    }
    printf(" -> NULL\n");
}

/**
 * Retrieve a block from the free block list.
 *
 * Pop the top element in the stack and return it.
 */
struct block *allocateFreeBlock()
{
    struct FreeBlockNode *head_block_node = fs.vcb.free_block_list.head;
    if (head_block_node == NULL)
        return NULL;

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
    int data_block_count = ceil((double)size / BLOCK_SIZE);

    int index_capacity = BLOCK_SIZE / sizeof(struct block *);
    if (data_block_count > index_capacity)
    {
        printf("File too large for indexed allocation.\n");
        return;
    }

    // allocate index block
    struct block *index_block = allocateFreeBlock();

    // allocate data blocks and store in index block
    for (int i = 0; i < data_block_count; i++)
    {
        struct block *data_block = allocateFreeBlock();

        memcpy(index_block->data + i * sizeof(struct block *),
               &data_block,
               sizeof(struct block *));
    }
    // create file information block and insert appropriate information
    int file_index = 0;
    while (file_index < MAX_FILES && fs.vcb.files[file_index] != NULL)
        file_index++;

    fs.vcb.files[file_index] = malloc(sizeof(struct FileInformation));

    int file_id = getFileInformationBlockId();

    fs.vcb.files[file_index]->id = file_id;
    snprintf(fs.vcb.files[file_index]->name, sizeof(fs.vcb.files[file_index]->name), "%s", filename);

    fs.vcb.files[file_index]->block_count = data_block_count + 1;
    fs.vcb.files[file_index]->index_block = index_block;
    fs.vcb.files[file_index]->file_size = size;

    fs.vcb.num_files_made += 1;

    printf("File '%s' created with %d data blocks + 1 index block.\n", filename, data_block_count);
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
    int i = 0;

    // Iterate through the file array and break if the file name is the same as the param.
    // If the file is not in the array, then print error message and return
    while (i < MAX_FILES)
    {
        if (fs.vcb.files[i] != NULL &&
            strcmp(fs.vcb.files[i]->name, filename) == 0)
            break;

        i++;
    }

    if (i == MAX_FILES)
    {
        printf("File not found\n");
        return;
    }

    // return all data blocks and the index block to the file system
    int data_blocks = fs.vcb.files[i]->block_count - 1;
    for (int j = 0; j < data_blocks; j++)
    {
        struct block *block_to_free;

        memcpy(&block_to_free, fs.vcb.files[i]->index_block->data + j * sizeof(struct block *), sizeof(struct block *));

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
    printf("Root Directory Listing (%d files):\n", fs.vcb.num_files_made);

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (fs.vcb.files[i] == NULL)
            continue;

        int actual_data_blocks = fs.vcb.files[i]->block_count - 1;

        printf("  %-8s | %5d bytes |  %d data blocks | FIBID=%d\n",
               fs.vcb.files[i]->name,
               fs.vcb.files[i]->file_size,
               actual_data_blocks,
               fs.vcb.files[i]->id);
    }
}
