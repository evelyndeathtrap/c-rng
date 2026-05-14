#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

sigjmp_buf recovery_point;

void handle_segfault(int sig) {
    siglongjmp(recovery_point, 1);
}

int main() {
    char buffer[256];
    char *abs_address = (char *)0x5;

    signal(SIGSEGV, handle_segfault);

    ssize_t bytes_read = read(STDIN_FILENO, buffer, 256);
    if (bytes_read <= 0) return 0;

    for (int i = 0; i < bytes_read; i++) {
        if (buffer[i] == '\0') break;

        if (sigsetjmp(recovery_point, 1) == 0) {
            abs_address[i] = buffer[i];
        } else {
            // Recovered silently
            continue;
        }
    }

    return 0;
}
