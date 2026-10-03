#include "e820.h"
#include "vga.h"

#define E820_COUNT_ADDRESS  0x4FF0
#define E820_BUFFER_ADDRESS 0x5000

void e820_print_map(void)
{
    volatile unsigned int *count =
        (volatile unsigned int *)E820_COUNT_ADDRESS;

    struct e820_entry *entries =
        (struct e820_entry *)E820_BUFFER_ADDRESS;

    vga_write("E820 entries: ");
    vga_write_hex(*count);
    vga_write("\n");

    for (unsigned int i = 0; i < *count; i++) {

        struct e820_entry *entry = &entries[i];

        vga_write("entry ");
        vga_write_hex(i);
        vga_write("\n");

        vga_write("  base low : ");
        vga_write_hex((unsigned int)entry->base);
        vga_write("\n");

        vga_write("  base high: ");
        vga_write_hex((unsigned int)(entry->base >> 32));
        vga_write("\n");

        vga_write("  len low  : ");
        vga_write_hex((unsigned int)entry->length);
        vga_write("\n");

        vga_write("  len high : ");
        vga_write_hex((unsigned int)(entry->length >> 32));
        vga_write("\n");

        vga_write("  type     : ");
        vga_write_hex(entry->type);
        vga_write("\n");
    }
}