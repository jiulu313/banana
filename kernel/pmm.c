#include "pmm.h"
#include "e820.h"
#include "memory.h"

#define PAGE_SIZE 4096

#define MAX_MEMORY (128 * 1024 * 1024)
#define MAX_PAGES  (MAX_MEMORY / PAGE_SIZE)

static unsigned char bitmap[MAX_PAGES / 8];

static unsigned int total_pages = 0;
static unsigned int free_pages = 0;


//bitmap辅助函数
static void bitmap_set(unsigned int page)
{
    bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(unsigned int page)
{
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static int bitmap_test(unsigned int page)
{
    return bitmap[page / 8] & (1 << (page % 8));
}


//初始化
void pmm_init(void)
{
    memset(bitmap, 0xFF, sizeof(bitmap));

    total_pages = MAX_PAGES;
    free_pages = 0;

    volatile unsigned int *count =
        (volatile unsigned int *)E820_COUNT_ADDRESS;

    struct e820_entry *entries =
        (struct e820_entry *)E820_BUFFER_ADDRESS;

    for (unsigned int i = 0; i < *count; i++) {

        struct e820_entry *entry = &entries[i];

        if (entry->type != 1) {
            continue;
        }

        unsigned int start = (unsigned int)entry->base;
        unsigned int end = (unsigned int)(entry->base + entry->length);

        start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        end &= ~(PAGE_SIZE - 1);

        for (unsigned int addr = start; addr < end; addr += PAGE_SIZE) {
            unsigned int page = addr / PAGE_SIZE;
            if (page < MAX_PAGES) {
                bitmap_clear(page);
                free_pages++;
            }
        }
    }

    // 当前先保留前 2MB
    for (unsigned int addr = 0;
         addr < 0x200000;
         addr += PAGE_SIZE) {

        unsigned int page = addr / PAGE_SIZE;

        if (!bitmap_test(page)) {
            bitmap_set(page);

            if (free_pages > 0) {
                free_pages--;
            }
        }
    }
}


void *pmm_alloc_page(void)
{
    for (unsigned int page = 0; page < MAX_PAGES;page++) {

        if (!bitmap_test(page)) {
            bitmap_set(page);

            if (free_pages > 0) {
                free_pages--;
            }

            return (void *)(page * PAGE_SIZE);
        }
    }

    return 0;
}


void pmm_free_page(void *addr)
{
    unsigned int address = (unsigned int)addr;

    if (address % PAGE_SIZE != 0) {
        return;
    }

    unsigned int page = address / PAGE_SIZE;

    if (page >= MAX_PAGES) {
        return;
    }

    if (bitmap_test(page)) {
        bitmap_clear(page);
        free_pages++;
    }
}


unsigned int pmm_get_total_pages(void)
{
    return total_pages;
}


unsigned int pmm_get_free_pages(void)
{
    return free_pages;
}