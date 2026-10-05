#ifndef TSS_H
#define TSS_H

void tss_init(void);
void tss_set_kernel_stack(unsigned int esp0);

unsigned int tss_get_address(void);
unsigned int tss_get_size(void);

#endif