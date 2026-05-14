#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Function to call the hardware random number generator
unsigned long long get_rdrand() {
    unsigned long long val;
    unsigned char ok;
    // Attempt to read a 64-bit hardware random number
    __asm__ volatile ("rdrand %0; setc %1"
                      : "=r" (val), "=qm" (ok));
    
    // If rdrand failed (rare), fallback to a standard rand()
    if (!ok) return (unsigned long long)rand();
    return val;
}

int main() {
    FILE *file = fopen("cwords.txt", "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    // 1. Count lines to establish the scale
    int lines = 0;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        lines++;
    }

    if (lines == 0) {
        fclose(file);
        return 1;
    }

    // 2. Wait for 5 seconds (5,000,000 microseconds)
    usleep(5000000);

    // 3. Generate hardware random number
    unsigned long long r = get_rdrand();

    // 4. Calculate index as requested: r / lines
    // We use modulo as a safety net if r / lines exceeds the line count
    int word_index = (r / lines) % lines;

    // 5. Retrieve and print that specific line
    rewind(file);
    int current = 0;
    while (fgets(buffer, sizeof(buffer), file)) {
        if (current == word_index) {
            printf("%s", buffer);
            break;
        }
        current++;
    }

    fclose(file);
    return 0;
}
