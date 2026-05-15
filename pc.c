#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <signal.h>

int console_fd = -1;

// Clean up and turn off the speaker on exit
void cleanup(int signum) {
    if (console_fd != -1) {
        // KIOCSOUND with 0 turns the sound off
        ioctl(console_fd, KIOCSOUND, 0);
        close(console_fd);
    }
    printf("\nSpeaker turned off. Exiting.\n");
    exit(0);
}

int main() {
    int urandom_fd;
    unsigned char byte_in;
    
    // Set up signal handling so Ctrl+C safely turns off the annoying buzz
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // 1. Open the console device to control the speaker
    // We try multiple entry points as permissions/availability can vary
    console_fd = open("/dev/tty0", O_WRONLY);
    if (console_fd == -1) {
        console_fd = open("/dev/vc/0", O_WRONLY);
    }
    if (console_fd == -1) {
        console_fd = open("/dev/console", O_WRONLY);
    }
    
    if (console_fd == -1) {
        perror("Failed to open console device (Are you root?)");
        return 1;
    }

    // 2. Open /dev/urandom for reading random bytes
    urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd == -1) {
        perror("Failed to open /dev/urandom");
        close(console_fd);
        return 1;
    }

    printf("Playing random frequencies. Press Ctrl+C to stop...\n");

    // 3. Main loop: Read random bytes and play them
    while (1) {
        if (read(urandom_fd, &byte_in, 1) == 1) {
            
            // Map the 0-255 byte value to a usable frequency range (e.g., 40Hz to 4000Hz)
            // Full range calculation: Min_Freq + (byte * step)
            int frequency = 40 + (byte_in * 15); 

            // Calculate the clock tick count for the PIT (Programmable Interval Timer)
            // PIT clock runs at 1.193180 MHz
            int pit_tick_count = 1193180 / frequency;

            // Send the count to the PC speaker via ioctl
            if (ioctl(console_fd, KIOCSOUND, pit_tick_count) < 0) {
                perror("ioctl KIOCSOUND failed");
                break;
            }

            // Adjust this delay (in microseconds) to change the "sample rate"
            // 20000 microseconds = 50 changes per second
            usleep(20000); 
        }
    }

    // Fallback cleanup if the loop breaks
    cleanup(0);
    return 0;
}
