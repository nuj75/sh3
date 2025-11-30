#ifndef fsops

#define fsops

#define BLOCK_SIZE 1024
#define TOTAL_BLOCKS 64
#define MAX_FILES 10

struct block
{
    unsigned char data[BLOCK_SIZE];
    int block_number;
};

struct file_information
{
    int id;
    char name[32];
    int file_size;
    struct block *index_block;
    int block_count;
};

struct FreeBlockNode
{
    struct block *blk;
    struct FreeBlockNode *next;
};

struct FreeBlockList
{
    struct FreeBlockNode *head;
    int count;
};

struct VolumeControlBlock
{
    struct FreeBlockList free_block_list;
    struct block blocks[TOTAL_BLOCKS];
    struct file_information *files[MAX_FILES];
    int status[MAX_FILES];
    int num_files_made;
};

struct FileSystem
{
    struct VolumeControlBlock vcb;
};

void init_FS();
void createFile(const char *filename, int size);
void listFiles();
struct block *allocateFreeBlock();
void returnFreeBlock(struct block *block);
void printFreeBlocks();
void deleteFile(const char *filename);

extern struct FileSystem fs;
#endif