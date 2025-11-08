#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

// should this be unsigned or can it be signed

unsigned char page_table[256] = {0};
unsigned char memory[32768] = {0};
int memory_tail = 0;

signed char *init_backstore()
{
    int file = open("BACKING_STORE.bin", O_RDONLY);
    unsigned char *mmap_pointer = mmap(0, 65536, PROT_READ, MAP_PRIVATE, file, 0);

    return mmap_pointer;
}

void clear_backstore(unsigned char *mmap_pointer)
{
    munmap(mmap_pointer, 65536);
}

unsigned char translate_page(unsigned char page_number, unsigned char *backstore_mmap)
{
    unsigned char page_hit = page_table[page_number];

    if (page_hit & 0xFE == 1)
    {
        return page_hit >> 1;
    }

    memcpy(memory + memory_tail, backstore_mmap + 256 * page_number, 256);
    unsigned char frame_number = memory_tail >> 8;
    memory_tail = (memory_tail + 256) % 32768;

    return frame_number;
}
int main()
{
    unsigned char *backstore_mmap = init_backstore();

    FILE *fptr = fopen("addresses.txt", "r");

    unsigned char buffer[2];
    while (fgets(buffer, 2, fptr) != NULL)
    {
        printf("%X\n", translate_page(buffer[0], backstore_mmap));
    }

    clear_backstore(backstore_mmap);
}
