#include "usercopy.h"
#include "paging.h"

int copy_from_user(
    void *kernel_dest,
    const void *user_src,
    unsigned int size)
{
    unsigned char *dest =
        (unsigned char *)kernel_dest;

    const unsigned char *src =
        (const unsigned char *)user_src;

    for (unsigned int i = 0; i < size; i++) {

        unsigned int user_address =
            (unsigned int)&src[i];

        if (!is_user_address_mapped(user_address)) {
            return 0;
        }

        //一个字节一个字节的copy
        dest[i] = src[i];
    }

    return 1;
}


int copy_string_from_user(
    char *kernel_dest,
    const char *user_src,
    unsigned int max_length)
{
    if (max_length == 0) {
        return 0;
    }

    for (unsigned int i = 0; i < max_length - 1; i++)
    {
        unsigned int address =
            (unsigned int)&user_src[i];

        if (!is_user_address_mapped(address)) {
            return 0;
        }

        char c = user_src[i];

        kernel_dest[i] = c;

        if (c == '\0') {
            return 1;
        }
    }

    /*
     * 超过最大长度还没碰到 '\0'
     */
    kernel_dest[max_length - 1] = '\0';

    return 0;
}


int copy_to_user(
    void *user_dest,
    const void *kernel_src,
    unsigned int size)
{
    unsigned char *dest =
        (unsigned char *)user_dest;

    const unsigned char *src =
        (const unsigned char *)kernel_src;

    for (unsigned int i = 0;
         i < size;
         i++)
    {
        unsigned int user_address =
            (unsigned int)&dest[i];

        if (!is_user_address_writable(
                user_address))
        {
            return 0;
        }

        dest[i] = src[i];
    }

    return 1;
}