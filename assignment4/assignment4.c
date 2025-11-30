#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 128
#define PHYSICAL_MEMORY_SIZE 32768
#define TLB_SIZE 16
#define LOGICAL_ADDRESS_SPACE 65536


struct TLBEntry
{
    int page_number;
    int frame_number;
    int valid; 
};


struct TLBEntry tlb[TLB_SIZE];
int tlb_tail = 0;
int page_table[NUM_PAGES];
signed char physical_memory[PHYSICAL_MEMORY_SIZE];
int memory_tail = 0;
int frame_to_page[NUM_FRAMES]; 


int page_fault_count = 0;
int tlb_hit_count = 0;
int total_addresses = 0;


signed char *init_backstore()
{
    int file = open("BACKING_STORE.bin", O_RDONLY);
    signed char *mmap_pointer = mmap(0, LOGICAL_ADDRESS_SPACE, PROT_READ, MAP_PRIVATE, file, 0);
    close(file);
    return mmap_pointer;
}

// Clean up backing store
void clear_backstore(signed char *mmap_pointer)
{
    munmap(mmap_pointer, LOGICAL_ADDRESS_SPACE);
}

// Initialize data structures
void initialize()
{
<<<<<<< HEAD
    signed char page_hit = page_table[page_number];

    if (page_hit & 0x01 == 1)
=======
    // Initialize page table to -1 (no page in memory)
    for (int i = 0; i < NUM_PAGES; i++)
>>>>>>> 2907e4852636661df0e634445f8c063481827f52
    {
        page_table[i] = -1;
    }
    
    // Initialize TLB
    for (int i = 0; i < TLB_SIZE; i++)
    {
        tlb[i].valid = 0;
        tlb[i].page_number = -1;
        tlb[i].frame_number = -1;
    }
    
    // Initialize frame_to_page mapping
    for (int i = 0; i < NUM_FRAMES; i++)
    {
        frame_to_page[i] = -1;
    }
}

// Search TLB for a page number
int search_TLB(int page_number)
{
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb[i].valid && tlb[i].page_number == page_number)
        {
            tlb_hit_count++;
            return tlb[i].frame_number;
        }
    }
    return -1; 
}

<<<<<<< HEAD
    page_table[page_number] = frame_number;

=======
// Add entry to TLB
void add_TLB(int page_number, int frame_number)
{
    tlb[tlb_tail].page_number = page_number;
    tlb[tlb_tail].frame_number = frame_number;
    tlb[tlb_tail].valid = 1;
    tlb_tail = (tlb_tail + 1) % TLB_SIZE;
}

// Update TLB when a page is replaced
void TLB_update(int old_page_number)
{
    // Find and invalidate the old page's TLB entry
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb[i].valid && tlb[i].page_number == old_page_number)
        {
            tlb[i].valid = 0; 
            return;
        }
    }
}


int handle_page_fault(int page_number, signed char *backstore_mmap)
{
    page_fault_count++;
    
    int frame_number = memory_tail / PAGE_SIZE;
    int old_page_number = frame_to_page[frame_number];
    
    // If frame is occupied, update old page's page table entry
    if (old_page_number != -1)
    {
        page_table[old_page_number] = -1;
        // Update TLB if old page has an entry
        TLB_update(old_page_number);
    }
    
    // Copy page from backing store to physical memory
    memcpy(physical_memory + (frame_number * PAGE_SIZE), 
           backstore_mmap + (page_number * PAGE_SIZE), 
           PAGE_SIZE);
    
    // Update page table
    page_table[page_number] = frame_number;
    
    // Update frame_to_page mapping
    frame_to_page[frame_number] = page_number;
    
    // Update memory_tail for FIFO
    memory_tail = (memory_tail + PAGE_SIZE) % PHYSICAL_MEMORY_SIZE;
    
>>>>>>> 2907e4852636661df0e634445f8c063481827f52
    return frame_number;
}

// Look up page table
int page_table_lookup(int page_number, signed char *backstore_mmap)
{
    // Check if page is in memory
    if (page_table[page_number] != -1)
    {
        return page_table[page_number];
    }
    
    return handle_page_fault(page_number, backstore_mmap);
}

// Retrieve physical frame number for a page
int retrieve_physical(int page_number, signed char *backstore_mmap)
{
    // First check TLB
    int frame_number = search_TLB(page_number);
    if (frame_number != -1)
    {
        return frame_number;
    }
    
    // If not in TLB, look up page table
    frame_number = page_table_lookup(page_number, backstore_mmap);
    
    add_TLB(page_number, frame_number);
    
    return frame_number;
}

int main()
{
    initialize();
    
    signed char *backstore_mmap = init_backstore();
    
    FILE *fptr = fopen("addresses.txt", "r");
    
    char buffer[100];
    
    while (fgets(buffer, sizeof(buffer), fptr) != NULL)
    {
        // Parse logical address
        int logical_address = atoi(buffer);
        total_addresses++;
        
        // Extract page number and offset
        int page_number = (logical_address >> 8) & 0xFF;
        int offset = logical_address & 0xFF;
        
        // Get frame number
        int frame_number = retrieve_physical(page_number, backstore_mmap);
        
        // Calculate physical address
        int physical_address = (frame_number * PAGE_SIZE) + offset;
        
        // Get value at physical address
        signed char value = physical_memory[physical_address];
        
        // Output results
        printf("Virtual address: %d Physical address: %d Value: %d\n", 
               logical_address, physical_address, value);
    }
    
    // Output statistics
    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_fault_count);
    printf("TLB Hits = %d\n", tlb_hit_count);
    
    fclose(fptr);
    clear_backstore(backstore_mmap);
    
    return 0;
}