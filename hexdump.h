#ifndef HEXDUMP_H
#define HEXDUMP_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief Prints a hexdump of a memory region.
 * @param description A description to print before the dump.
 * @param addr Pointer to the memory to dump.
 * @param len Number of bytes to dump.
 */
void hexdump(const char *description, const void *addr, int len);

#ifdef HEXDUMP_IMPL

inline void hexdump(const char *description, const void *addr, int len) {
    int i;
    unsigned char buff[17];
    const unsigned char *pc = (const unsigned char *)addr;

    // Output description if given.
    if (description != NULL) {
        printf("%s:\n", description);
    }

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n", len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).
        if ((i % 16) == 0) {
            // Don't print ASCII for the very first line.
            if (i != 0)
                printf("  |%s|\n", buff);

            // Output the offset.
            printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if (isprint(pc[i]))
            buff[i % 16] = pc[i];
        else
            buff[i % 16] = '.';
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf("  |%s|\n", buff);
}

#endif // HEXDUMP_IMPL

#endif // HEXDUMP_H
