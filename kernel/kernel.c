void kernel_main(void)
{
    volatile char *vga = (volatile char *)0xB8000;

    const char *message = "hello from banana C kernel!";

    int i = 0;
    while (message[i] != '\0')
    {
        vga[i * 2] = message[i];
        vga[i * 2 + 1] = 0x0F;
        i++;
    }
    

 

    while (1) {
    }
}