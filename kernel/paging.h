#ifndef PAGING_H
#define PAGING_H

void paging_init(void);

int map_page(
    unsigned int virtual_address,
    unsigned int physical_address,
    unsigned int flags
);

int unmap_page(unsigned int virtual_address);

#endif