#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <signal.h>

int console_fd = -1;

void cleanup(int signum) {
    if (console_fd != -1) {
        ioctl(console_fd, KIOCSOUND, 0);
        close(console_fd);
    }
    exit(0);
}

int main() {
    int urandom_fd;
    unsigned short raw_word; 

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    console_fd = open("/dev/console", O_WRONLY);
    if (console_fd == -1) {
        perror("Failed to open /dev/console");
        return 1;
    }

    urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd == -1) {
        perror("Failed to open /dev/urandom");
        close(console_fd);
        return 1;
    }

    while (1) {
        if (read(urandom_fd, &raw_word, 2) == 2) {
            // Modulo clamps the range, then we offset it to the bottom floor
            // Restricts counts to 59660 - 65535 (Strictly below 20 Hz)
            int infrasonic_count = 59660 + (raw_word % (65535 - 59660));

            ioctl(console_fd, KIOCSOUND, infrasonic_count);
            usleep(20000); // Slightly longer sleep so the low wave can cycle
        }
    }

    cleanup(0);
    return 0;
}
