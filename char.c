#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    // A complete 256-byte array sorted by "usage priority"
    unsigned char sorted_ascii[256] = {
        // Common lowercase (Rank 0-25)
        'e', 't', 'a', 'o', 'i', 'n', 's', 'h', 'r', 'd', 'l', 'c', 'u', 'm', 'w', 'f', 'g', 'y', 'p', 'b', 'v', 'k', 'j', 'x', 'q', 'z',
        // Common Uppercase (Rank 26-51)
        'E', 'T', 'A', 'O', 'I', 'N', 'S', 'H', 'R', 'D', 'L', 'C', 'U', 'M', 'W', 'F', 'G', 'Y', 'P', 'B', 'V', 'K', 'J', 'X', 'Q', 'Z',
        // Numbers & Basic Punctuation (Rank 52-71)
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '.', ',', '!', '?', '-', '_', '(', ')', '\n',
        // Symbols (Rank 72-94)
        '[', ']', '{', '}', '/', '\\', '|', '&', '*', '+', '=', '<', '>', '@', '#', '$', '%', '^', '`', '~', '\'', '\"', ';', ':',
    };

    // Fill the remaining slots (95-255) with any characters not yet included
    int filled = 95;
    for (int i = 0; i < 256; i++) {
        int already_exists = 0;
        for (int j = 0; j < filled; j++) {
            if (sorted_ascii[j] == (unsigned char)i) {
                already_exists = 1;
                break;
            }
        }
        if (!already_exists) {
            sorted_ascii[filled++] = (unsigned char)i;
        }
    }

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) return 1;

    unsigned char read_byte;

    // Loop indefinitely
    while (read(fd, &read_byte, 1) > 0) {
        // read_byte is 0-255, matching an index in our sorted_ascii array
        unsigned char target_char = sorted_ascii[read_byte];

        // Print the character
        // Note: Non-printable chars may appear as blank or symbols in your terminal
        printf("%c", target_char);

        // Print the calculation 255 / value (using the mapped character's ASCII value)
        if (target_char != 0) {
            // Uncomment the line below if you want to see the math alongside the char
            // printf(" [%.2f] ", 255.0 / target_char);
        }
 
        fflush(stdout);
      usleep(100000);
    }

    close(fd);
    return 0;
}
