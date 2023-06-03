#include "stdio.h"

#define DEBUGGING 0

#define min(x,y) (x<y)?x:y

void pkt_hex_dump(unsigned char *data, unsigned int len);
