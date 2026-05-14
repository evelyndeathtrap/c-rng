int main() {
    unsigned char buffer[256];
    unsigned char *abs_address = (unsigned char *)0x5;

    // Register the crash handler
    signal(SIGSEGV, handle_segfault);


    while (1) {
        // 1. Read from stdin
        ssize_t bytes_read = read(STDIN_FILENO, buffer, 256);
        if (bytes_read <= 0) break;

        // 2. Set the "Safe Point"
        if (sigsetjmp(recovery_point, 1) == 0) {

            // 3. The Forbidden Write
            for (int i = 0; i < bytes_read; i++) {
                abs_address[i] = buffer[i]; // <--- CRASH OCCURS H>
            }

        } else {
            // This runs only after the crash happens
        }
    }

    return
      0;
}
