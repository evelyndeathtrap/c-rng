#include <stdio.h>

int main() {
    int c;

    // The "forever" loop
    for (;;) {
        c = getchar();

        // Check for End Of File (EOF) to allow clean exit via Ctrl+D
        if (c == EOF) {
            break;
        }

        // Inline Assembly:
        // 1. Move the char into the RAX register
        // 2. 'movzx' treats the input as an unsigned char and zeros out the rest of rax
        __asm__ __volatile__ (
            "movzx rax, %0"
            :
            : "r" ((unsigned char)c)
            : "rax"
        );

        /* 
           Note: In a real-world debugging scenario, RAX is volatile. 
           Any code after this assembly block might overwrite RAX. 
        */
    }

    return
      0;
}
