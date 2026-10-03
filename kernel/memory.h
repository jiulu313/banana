#ifndef MEMORY_H
#define MEMORY_H

void memory_init(void);

void *kmalloc(unsigned int size);

void *memset(void *dest, int value, unsigned int count);
void *memcpy(void *dest, const void *src, unsigned int count);

#endif