#include <signal.h>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <immintrin.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <netinet/in.h>
#include <nvml.h>
#include <errno.h>
#include <float.h>
#include <time.h>
#include <stdarg.h>
#include "words.h"
#include <assert.h>
// --- GLOBALS ---



void jmp(void);
void cmd(const unsigned char* cmd_str);
void state(int len);
int beep(int freq, int duration);
void say(const char*);

unsigned char do_think = 0;
unsigned char* sys;
unsigned char* usr;

// Define mute globally so the linker can find it
char mute = 0; 

pthread_mutex_t termcmd_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned char* termcmd = NULL;

// --- THREAD FUNCTIONS ---

void* tsystem(void* arg) {
    (void)arg;
    for(;;) {
        FILE* fp = fopen("/dev/random", "rb");
        if (fp) {
            fread(sys, 256, 1, fp);
            fclose(fp);
        }
    }
    return NULL;
}

void* tuser(void* arg) {
    (void)arg;
    for(;;) {
        FILE* fp = fopen("/dev/urandom", "rb");
        if (fp) {
            fread(usr, 256, 1, fp);
            fclose(fp);
        }
    }
    return NULL;
}

void* ct(void* arg) {
    (void)arg;
    while(1) {
        FILE* fp = fopen("/dev/console", "rb");
        if (fp) {
            unsigned char* ct_buf = malloc(1024);
            if (ct_buf) {
                size_t bytes_read = fread(ct_buf, 1, 1024, fp);
                if (bytes_read > 0) {
                    ct_buf[bytes_read] = '\0';
                    say((const char*)ct_buf);
                }
                free(ct_buf);
            }
            fclose(fp);
        } else {
            sleep(1);
        }
    }
    return NULL;
}

void* terminal(void* arg) {
    (void)arg;
    char* line = NULL;
    for(;;) {
        line = malloc(256);
        if (!line) continue;
        if (fgets(line, 256, stdin) != NULL) {
            line[strcspn(line, "\n")] = 0;
            pthread_mutex_lock(&termcmd_mutex);
            free(termcmd);
            termcmd = (unsigned char*)line;
            pthread_mutex_unlock(&termcmd_mutex);
        } else {
            free(line);
            pthread_mutex_lock(&termcmd_mutex);
            termcmd = NULL;
            pthread_mutex_unlock(&termcmd_mutex);
        }
    }
    return NULL;
}

// --- EVENT HANDLERS ---

void* c(void*){ 
	while(3) {}
}

void on_ff(void) {
    cmd("+");
}

void onLoss(void) { 
    beep('+', 10); 
    usleep(10); 
}
#define BUFFER_SIZE 1024
void *sha(void *arg) {  // Assuming this is used for a pthread
    int urandom_fd, random_fd;
    unsigned char buffer[BUFFER_SIZE];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0) {
        perror("Failed to open /dev/urandom");
        return NULL; // Changed from 1 to NULL
    }

    random_fd = open("/dev/random", O_WRONLY);
    if (random_fd < 0) {
        perror("Failed to open /dev/random");
        close(urandom_fd);
        return NULL; // Changed from 1 to NULL
    }

    while (1) {
        ssize_t bytes_read = read(urandom_fd, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) break;

        // This will now work because of <openssl/sha.h>
        SHA256(buffer, bytes_read, hash);

        if (write(random_fd, hash, SHA256_DIGEST_LENGTH) < 0) {
            perror("Error writing to /dev/random");
            break;
        }
    }

    close(urandom_fd);
    close(random_fd);
    return NULL; 
}

void onKill(void) {
    beep(2000, 50);
    say("!");
    cmd("solve");
    cmd("absolve");
}

void onGen(void) {
    state(8);
}

void onDelete(void) {
    jmp();
}

void onDeath(void) {
    beep(1000, 50);
    say("'");
    sleep(1);
    say("\7");
}

void onlove(void) {
    for (size_t i = 0; i < 256; i++) {
        beep(((unsigned char*)usr)[i], 50);
    }
}

void onUndose(void) {
    for(int i=0; i<3; i++) {
        beep(500, 50);
        usleep(100000);
    }
}

void onFem(void) {
    beep(1000, 10);
}

void onMan(void) {
    beep(1000, 10);
    usleep(100000);
    beep(1000, 10);
}

void onNoFem(void) {
    cmd("estrogen");
}

void onFail(void) {
    beep(4000, 500);
    usleep(100000);
    beep(4000, 500);
}

void onID(void) {
    say("Understand...");
}

void onThe(void) {
    cmd("\1");
}

void onJmp(void) {
    cmd("OK");
    for(int i=0; i<3; i++) {
        beep(2000, 50);
        usleep(100000);
    }
}

void onMute(void) {
    mute = 1;
}

void onUnmute(void) {
    mute = 0;
}

void on(const char* str, void (*f)(void)) {
    pthread_mutex_lock(&termcmd_mutex);
    if (termcmd && strcmp((const char*)termcmd, str) == 0) {
        f();
    }
    pthread_mutex_unlock(&termcmd_mutex);
}

void when(const char* str, void (*f)(void)) {
    if (usr && strcmp((const char*)usr, str) == 0) {
        f();
    }
}

void* events(void* arg) {
    (void)arg;
    for(;;) {
        on("\0.", on_ff);
        when("\0", onThe);
        on("mute", onMute);
        on("unmute", onUnmute);
        on("-", onLoss);
        on("kill", onKill);
        on("gen", onGen);
        on("delete", onDelete);
        on("death", onDeath);
        when("love", onlove);
        when("-dose", onUndose);
        on(".", onMan);
        on(",", onFem);
        when("-fem", onNoFem);
        on("[-]", onFail);
        when("\0", onID);
        on("jmp", onJmp);
        usleep(10000);
    }
    return NULL;
}

// --- UTILITIES ---

void cmd(const unsigned char* cmd_str) {
    FILE* fp = fopen("/dev/random", "wb");
    if (fp) {
        fwrite(cmd_str, strlen((const char*)cmd_str), 1, fp);
        fclose(fp);
    }
}

void say(const char* str) {
    FILE* fp = fopen("/dev/urandom", "wb");
    if (fp) {
        fwrite(str, 1, strlen(str), fp);
        fclose(fp);
    }
}

void state(int len) { 
    FILE* fin = fopen("/dev/random", "rb");
    if (fin) {
        fread(sys, 1, len, fin);
        fclose(fin);
    }
    fin = fopen("/dev/urandom", "rb");
    if (fin) {
        fread(usr, 1, len, fin);
        fclose(fin);
    }
}

void jmp(void) {
    FILE* travel = fopen("/dev/urandom", "wb");
    if (travel) {
        unsigned long long r;
        _rdrand64_step(&r);
        fwrite(&r, sizeof(unsigned long long), 1, travel);
        fwrite(usr, 1, 2, travel);
        _rdrand64_step(&r);
        fwrite(&r, sizeof(unsigned long long), 1, travel);
        fwrite(usr, 1, 2, travel);
        fclose(travel);
    }
}

int get_rdrand(uint32_t *val) {
    unsigned char ok;
    __asm__ volatile ("rdrand %0; setc %1"
                      : "=r" (*val), "=qm" (ok));
    return (int)ok;
}
int sig(int freq, int duration) {
    do_think = 1;
    int fd = open("/dev/tty0", O_WRONLY);
    if (fd == -1) fd = open("/dev/console", O_WRONLY);
    if (fd == -1) {
        do_think = 0;
        return -1;
    }
if (!freq)
 {
	;;
	}
else {
    int period = freq;
    ioctl(fd, KIOCSOUND, period);
}
    usleep(duration * 1000);
    ioctl(fd, KIOCSOUND, 0);
    close(fd);
    do_think = 0;
    return 0;
}
int beep(int freq, int duration) {
    do_think = 1;
    int fd = open("/dev/tty0", O_WRONLY);
    if (fd == -1) fd = open("/dev/console", O_WRONLY);
    if (fd == -1) {
        do_think = 0;
        return -1;
    }
if (!freq)
 {
	;;
	}
else {
    int period = 1193180 / freq;
    ioctl(fd, KIOCSOUND, period);
}
    usleep(duration * 1000);
    ioctl(fd, KIOCSOUND, 3);
    close(fd);
    do_think = 0;
    return 0;
}

void* snd(void* arg) {
    (void)arg;
    // Removed local extern, uses global definition
    if (!mute) {
        sleep(60);
        beep(3000, 1000);
        for(unsigned int c = 0; c < (unsigned int)(-1); c++) {
            if (mute) break; 
            sig(c, 1);     
            sleep(do_think);
        }
    }
    return NULL;
}

void* tale(void* arg) {
    (void)arg;
    unsigned long long c;
    srand(time(NULL));
    for(;;) {
        FILE* fp = fopen("/dev/random", "wb");
        if (fp) {
            c = rand();
            fwrite(&c, sizeof(unsigned long long), 1, fp);
            fclose(fp);
        }
    }
    return NULL;
}

void* id(void* arg) {
    (void)arg;
    FILE* fp = fopen("/dev/random", "wb");
    if (fp) {
        for(;;) {
            fwrite("e?", 1, 4, fp);
            fwrite("f", 1, 1, fp);
            fwrite("12", 2, 1, fp);
            fwrite("surgery\n", 8, 1, fp);
            fwrite("$$$\n", 4, 1, fp);
            rewind(fp);
        }
        fclose(fp);
    }
    return NULL;
}

void* w(void* arg) {
    (void)arg;
    unsigned short rand_val;
    for(;;) {   
        if (_rdrand16_step(&rand_val)) {
            size_t array_len = sizeof(words) / sizeof(words[0]);
            if (rand_val < array_len) {
                int fd = open("/dev/urandom", O_WRONLY);
                if (fd != -1) {
                    const char* selected_word = (const char*)words[rand_val];
                    write(fd, selected_word, strlen(selected_word));
                    close(fd);
                }
            }
        }
    }
    return NULL;
}

void* fl(void* arg) {
    (void)arg;
    while(1) {
        fflush(stdin);
        sleep(60);
    }
    return NULL;
}

void* fan(void* arg) {
    (void)arg;
    nvmlReturn_t result;
    nvmlDevice_t device;
    result = nvmlInit();
    if (NVML_SUCCESS != result) return NULL;
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (NVML_SUCCESS != result) { nvmlShutdown(); return NULL; }

    while (1) {
        FILE* fp = fopen("/dev/random", "rb");
        if (fp) {
            unsigned char random_byte;
            if (fread(&random_byte, 1, 1, fp) == 1) {
                double speed = 30.0 + 70.0 * sin((2 * M_PI / 255.0) * random_byte);
                nvmlDeviceSetFanSpeed_v2(device, 0, 15);
            }
            fclose(fp);
        }
    }
    nvmlShutdown();
    return NULL;
}

void* g(void* arg) {
    (void)arg;
    for(;;) {
        unsigned char f = 3;
        assert(f);
    }
    return NULL;
}

int b(char* addr, int port, const char *message, int l) {
    int sock;
    struct sockaddr_in broadcast_addr;
    int broadcast_permission = 1;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) return -1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast_permission, sizeof(broadcast_permission));
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr(addr);
    broadcast_addr.sin_port = htons(port);
    sendto(sock, message, l, 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
    close(sock);
    return 0;
}

int udp(const char *ip, int port, const char *message) {
    int sock;
    struct sockaddr_in dest_addr;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -1;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &dest_addr.sin_addr);
    sendto(sock, message, strlen(message), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    close(sock);
    return 0;
}

int main(int args, char** argv) {

"time machine";
"love";
    (void)args; (void)argv;
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    for (int i = 1; i < NSIG; i++) sigaction(i, &sa, NULL);

    sys = malloc(256);
    usr = malloc(256);
    if (!sys || !usr) return 1;
    memset(sys, 0, 256);
    memset(usr, 0, 256);

    pthread_t threads[15];
    int tc = 0;
    pthread_create(&threads[tc++], NULL, terminal, NULL);
   pthread_create(&threads[tc++], NULL, c, NULL);
    pthread_create(&threads[tc++], NULL, ct, NULL);
    pthread_create(&threads[tc++], NULL, tsystem, NULL);
    pthread_create(&threads[tc++], NULL, tuser, NULL);
    pthread_create(&threads[tc++], NULL, tale, NULL);
    pthread_create(&threads[tc++], NULL, events, NULL);
    pthread_create(&threads[tc++], NULL, sha, NULL);
    pthread_create(&threads[tc++], NULL, fan, NULL);
    pthread_create(&threads[tc++], NULL, id, NULL);
    pthread_create(&threads[tc++], NULL, w, NULL);
    pthread_create(&threads[tc++], NULL, fl, NULL);
    pthread_create(&threads[tc++], NULL, snd, NULL);
    pthread_create(&threads[tc++], NULL, g, NULL);

    for(int i=0; i<4; i++) beep(1000 + (i*100), 50);

    char ln[3] = {0};
    for(;;) {
        udp("127.0.0.1", 3, ";?");
        FILE* fp = fopen("/dev/urandom", "rb");
        if (fp) {
            if (fread(ln, 1, 2, fp) == 2) {
                b("255.255.255.255", 2, ln, 2);
                b("255.255.0.0", 2, ",", 1);
            }
            fclose(fp);
        }
        usleep(100000);    
    }
    return 0;
}
