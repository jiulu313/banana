#ifndef E820_H
#define E820_H

struct e820_entry
{
    unsigned long long base;
    unsigned long long length;
    unsigned int type;
    unsigned int acpi;
} __attribute__((packed));

void e820_print_map(void);

#endif