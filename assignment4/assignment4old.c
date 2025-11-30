#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct TLBStruct
{
    signed char page_number;
    signed char frame_number;
};

struct TLBStruct *tlb_arr[16] = {NULL};
int tlb_tail = 0;

signed char page_table[256] = {0};
signed char memory[32768] = {0};
int memory_tail = 0;

signed char *init_backstore()
{
    int file = open("BACKING_STORE.bin", O_RDONLY);
    signed char *mmap_pointer = mmap(0, 65536, PROT_READ, MAP_PRIVATE, file, 0);

    return mmap_pointer;
}

void clear_backstore(signed char *mmap_pointer)
{
    munmap(mmap_pointer, 65536);
}

signed char page_table_lookup(signed char page_number, signed char *backstore_mmap)
{
    signed char page_hit = page_table[page_number];

    if (page_hit & 0xFE == 1)
    {
        return page_hit >> 1;
    }

    memcpy(memory + memory_tail, backstore_mmap + 256 * page_number, 256);
    signed char frame_number = memory_tail >> 8;
    memory_tail = (memory_tail + 256) % 32768;

    return frame_number;
}

signed char search_TLB(signed char page_number)
{
    for (int i = 0; i < 16; i++)
    {
        if (tlb_arr[(tlb_tail + 16 - i) % 16] != NULL && tlb_arr[(tlb_tail + 16 - i) % 16]->page_number == page_number)
        {
            return tlb_arr[(tlb_tail + 16 - i) % 16]->frame_number;
        }
    }

    return -1;
}

void add_TLB(signed char page_number, signed char frame_number)
{
    struct TLBStruct *new_entry = malloc(sizeof(struct TLBStruct));
    new_entry->page_number = page_number;
    new_entry->frame_number = frame_number;

    free(tlb_arr[tlb_tail]);

    tlb_arr[tlb_tail] = new_entry;

    tlb_tail = (tlb_tail + 1) % 16;
}

void TLB_update(signed char frame_number, signed char new_page_number)
{
    for (int i = 0; i < 16; i++)
    {
        if (tlb_arr[(tlb_tail + 16 - i) % 16] != NULL && tlb_arr[(tlb_tail + 16 - i) % 16]->frame_number == frame_number)
        {
            tlb_arr[(tlb_tail + 16 - i) % 16]->page_number = new_page_number;
        }
    }
}

signed char retrieve_physical(signed char page_number, signed char *backstore_mmap)
{
    signed char tlb_lookup = search_TLB(page_number);

    if (tlb_lookup != -1)
    {
        return tlb_lookup;
    }

    signed char frame_number = page_table_lookup(page_number, backstore_mmap);

    TLB_update(frame_number, page_number);
    add_TLB(page_number, frame_number);

    return frame_number;
}

int main()
{
    signed char *backstore_mmap = init_backstore();

    FILE *fptr = fopen("addresses.txt", "r");

    char buffer[100];

    while (fgets(buffer, sizeof(buffer), fptr) != NULL)
    {
        buffer[strlen(buffer) - 2] = '\0';

        int virtual_addr = atoi(buffer);
        signed char page_number = virtual_addr >> 8;
        signed char offset = (virtual_addr & 0x00FF);
        signed char frame_number = retrieve_physical(page_number, backstore_mmap);

        signed char physical_address = (frame_number << 8) + offset;
    }

    clear_backstore(backstore_mmap);
}
