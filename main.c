#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    FILE *file = fopen("cwords.txt", "r");
    if (!file) {
        perror("Error opening cwords.txt");
        return 1;
    }

    // 1. Count the lines in the file
    int line_count = 0;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        line_count++;
    }

    if (line_count == 0) {
        fprintf(stderr, "File is empty.\n");
        fclose(file);
        return 1;
    }

    // 2. Read a short from /dev/random
    unsigned short random_val;
    int rand_file = open("/dev/random", O_RDONLY);
    if (rand_file < 0) {
        perror("Error opening /dev/random");
        fclose(file);
        return 1;
    }
    
    if (read(rand_file, &random_val, sizeof(random_val)) != sizeof(random_val)) {
        perror("Error reading from /dev/random");
        close(rand_file);
        fclose(file);
        return 1;
    }
    close(rand_file);

    // 3. Determine the target line index
    int target_line = random_val % line_count;

    // 4. Retrieve and print the word
    rewind(file);
    int current_line = 0;
    while (fgets(buffer, sizeof(buffer), file)) {
        if (current_line == target_line) {
            printf("%s", buffer);
            break;
        }
        current_line++;
    }

    fclose(file);
    retur
      n 0;
}
