#include "debugging.h"

void pkt_hex_dump(unsigned char *data, unsigned int len)
{
    int rowsize = 16;
    int i, l, linelen, remaining;
    int li = 0;
    unsigned char ch; 

    printf("\nPacket hex dump:\n");
    printf("Packet Size = %u\n", len);

    remaining = len;
    for (i = 0; i < len; i += rowsize) {
        printf("%06d\t", li);

        linelen = min(remaining, rowsize);
        remaining -= rowsize;

        for (l = 0; l < linelen; l++) {
            ch = data[l];
            printf("%02X ", (unsigned int) ch);
        }

        data += linelen;
        li += 10; 

        printf("\n");
    }
}
