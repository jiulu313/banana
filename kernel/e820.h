#ifndef E820_H
#define E820_H


#define E820_COUNT_ADDRESS  0x4FF0
#define E820_BUFFER_ADDRESS 0x5000

struct e820_entry
{
    unsigned long long base;
    unsigned long long length;
    unsigned int type;
    unsigned int acpi;
} __attribute__((packed));

void e820_print_map(void);

#endif