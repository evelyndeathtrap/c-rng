
#include <stdio.h>

int main(void) {
    unsigned long long c;

    // Open urandom for reading, not writing
    FILE* fp = fopen("/dev/urandom", "rb");
    if (fp == NULL) return 1;

    fread(&c, sizeof(c), 1, fp);
    fclose(fp);

    // FIX: Get the address of 'c' (&c) and treat it as a pointer >
    unsigned char* cc = (unsigned char*)&c;

    // FIX: fwrite(data, size, count, stream)
    fwrite(cc, 1, sizeof(c), stdout);

    return
      0;
}
