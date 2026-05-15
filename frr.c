#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#define SAMPLE_RATE 386000
#define CHANNELS 1
#define BUFFER_SIZE 4096
#define MAX_ITER 256

// Fourier Analysis Config for the Entropy Stream
#define FOURIER_WINDOW 64    // Size of the random data block to analyze
#define N_HARMONICS 8        // Number of Fourier Series harmonics to extract

const double JULIA_RE = -1.0;
const double JULIA_IM = 0;

int random_fd = -1;

// Global arrays to hold the active Fourier Series spectrum of our entropy
double a_coefficients[N_HARMONICS + 1];
double b_coefficients[N_HARMONICS + 1];

// Reads a window of random data and calculates its Fourier Series coefficients
void analyze_entropy_fourier() {
    unsigned char raw_bytes[FOURIER_WINDOW];
    
    // Read a block of raw chaos from the kernel
    if (read(random_fd, raw_bytes, FOURIER_WINDOW) != FOURIER_WINDOW) {
        return; // Skip update if entropy pool is temporarily starving
    }

    // Convert raw bytes into a normalized signal between -1.0 and 1.0
    double signal[FOURIER_WINDOW];
    double sum = 0.0;
    for (int t = 0; t < FOURIER_WINDOW; t++) {
        signal[t] = ((double)raw_bytes[t] / 127.5) - 1.0;
        sum += signal[t];
    }

    // 1. Calculate DC Component (a_0)
    a_coefficients[0] = sum / FOURIER_WINDOW;

    // 2. Compute the fundamental and harmonic coefficients via discrete integration
    for (int n = 1; n <= N_HARMONICS; n++) {
        double a_n_sum = 0.0;
        double b_n_sum = 0.0;
        
        for (int t = 0; t < FOURIER_WINDOW; t++) {
            double angle = 2.0 * M_PI * n * t / FOURIER_WINDOW;
            a_n_sum += signal[t] * cos(angle);
            b_n_sum += signal[t] * sin(angle);
        }
        
        // Store the realized Fourier coefficients
        a_coefficients[n] = (2.0 / FOURIER_WINDOW) * a_n_sum;
        b_coefficients[n] = (2.0 / FOURIER_WINDOW) * b_n_sum;
    }
}

// Render a single audio sample 
int16_t compute_julia_audio_sample(unsigned long long sample_index) {
    double t = (double)sample_index / SAMPLE_RATE;
    
    // Periodically run a Fourier analysis on a new batch of random data.
    // Draining /dev/random too fast stalls the audio thread, so we update every 1024 frames.
    if (sample_index % 1024 == 0) {
        analyze_entropy_fourier();
    }
    
    // Use the 1st and 2nd harmonic amplitudes of the random data to warp the fractal
    // This maps the spectral energy of /dev/random directly to physical space.
    double entropy_warp_x = a_coefficients[1] * 0.15 + b_coefficients[2] * 0.05;
    double entropy_warp_y = b_coefficients[1] * 0.15 + a_coefficients[2] * 0.05;
    
    // Use the total spectral energy shift (DC component + higher harmonic) to modulate zoom
    double spectral_zoom = 1.5 + fabs(a_coefficients[0] * 2.0) + fabs(a_coefficients[3] * 1.5);
    double zoom = (2.1 + 1.9 * sin(0.2 * t)) * spectral_zoom;
    if (zoom < 0.1) zoom = 0.1;
    
    double angle = t * 0.05;
    double target_x = 0.5 * cos(angle) + entropy_warp_x;
    double target_y = 0.5 * sin(angle) + entropy_warp_y;

    double zr = target_x + (sin(t * 220.0 * 2.0 * M_PI) / zoom);
    double zi = target_y + (cos(t * 220.0 * 2.0 * M_PI) / zoom);

    int iter = 0;
    while (zr * zr + zi * zi < 4.0 && iter < MAX_ITER) {
        double temp = zr * zr - zi * zi + JULIA_RE;
        zi = 2.0 * zr * zi + JULIA_IM;
        zr = temp;
        iter++;
    }

    double normalized = (double)iter / (double)MAX_ITER;
    int16_t sample = (int16_t)((normalized * 2.0 - 1.0) * 28000.0);
    return sample;
}

int main() {
    int rc;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    int16_t buffer[BUFFER_SIZE];

    // Setup entropy pool
    random_fd = open("/dev/random", O_RDONLY);
    if (random_fd < 0) {
        perror("Failed to open /dev/random, switching to /dev/urandom");
        random_fd = open("/dev/urandom", O_RDONLY);
        if (random_fd < 0) {
            fprintf(stderr, "Critical Error: No entropy source accessible.\n");
            return 1;
        }
    }

    // Initialize Fourier coefficient arrays to baseline values
    memset(a_coefficients, 0, sizeof(a_coefficients));
    memset(b_coefficients, 0, sizeof(b_coefficients));

    // Open ALSA Playback Stream
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        fprintf(stderr, "Unable to open pcm device: %s\n", snd_strerror(rc));
        close(random_fd);
        return 1;
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);

    val = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
    frames = BUFFER_SIZE / 4;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr, "Unable to set hw parameters: %s\n", snd_strerror(rc));
        close(random_fd);
        return 1;
    }

    unsigned long long global_sample_counter = 0;
    printf("Streaming Julia Set modulated by /dev/random Fourier Series... Press Ctrl+C\n");

    while (1) {
        for (int i = 0; i < BUFFER_SIZE; i++) {
            buffer[i] = compute_julia_audio_sample(global_sample_counter);
            global_sample_counter++;
        }

        rc = snd_pcm_writei(handle, buffer, BUFFER_SIZE);
        
        if (rc == -EPIPE) {
            fprintf(stderr, "Xrun recovery activated.\n");
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error from writei: %s\n", snd_strerror(rc));
        }
    }

    close(random_fd);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    return 0;
}
