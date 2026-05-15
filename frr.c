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
#define FOURIER_WINDOW 64    
#define N_HARMONICS 8        

// Basilica Julia Set baseline constants
const double JULIA_RE = -1.0;
const double JULIA_IM = 0.0001;

int random_fd = -1;

// Global arrays holding the extracted overtone weights from /dev/random
double a_coefficients[N_HARMONICS + 1];
double b_coefficients[N_HARMONICS + 1];

// Reads a window of random data and calculates its Fourier Series coefficients
void analyze_entropy_fourier() {
    unsigned char raw_bytes[FOURIER_WINDOW];
    
    if (read(random_fd, raw_bytes, FOURIER_WINDOW) != FOURIER_WINDOW) {
        return; 
    }

    double signal[FOURIER_WINDOW];
    double sum = 0.0;
    for (int t = 0; t < FOURIER_WINDOW; t++) {
        signal[t] = ((double)raw_bytes[t] / 127.5) - 1.0;
        sum += signal[t];
    }

    a_coefficients[0] = sum / FOURIER_WINDOW;

    for (int n = 1; n <= N_HARMONICS; n++) {
        double a_n_sum = 0.0;
        double b_n_sum = 0.0;
        
        for (int t = 0; t < FOURIER_WINDOW; t++) {
            double angle = 2.0 * M_PI * n * t / FOURIER_WINDOW;
            a_n_sum += signal[t] * cos(angle);
            b_n_sum += signal[t] * sin(angle);
        }
        
        a_coefficients[n] = (2.0 / FOURIER_WINDOW) * a_n_sum;
        b_coefficients[n] = (2.0 / FOURIER_WINDOW) * b_n_sum;
    }
}

// Render a single audio sample 
int16_t compute_julia_audio_sample(unsigned long long sample_index) {
    double t = (double)sample_index / SAMPLE_RATE;
    
    if (sample_index % 1024 == 0) {
        analyze_entropy_fourier();
    }
    
    // --- OVERTONE ENGINE FOR THE FRACAL FORMULA ---
    const double fundamental_freq = 220.0;
    double overtone_sine_sum = 0.0;
    double overtone_cosine_sum = 0.0;

    // Synthesize the harmonic overtone wave arrays driven by entropy values
    for (int n = 1; n <= N_HARMONICS; n++) {
        double phase_angle = t * (fundamental_freq * n) * 2.0 * M_PI;
        overtone_sine_sum   += a_coefficients[n] * sin(phase_angle);
        overtone_cosine_sum += b_coefficients[n] * cos(phase_angle);
    }
    
    // Setup clean spatial orbits 
    double zoom = 2.1 + 1.9 * sin(0.2 * t);
    double angle = t * 0.05;
    double target_x = 0.5 * cos(angle);
    double target_y = 0.5 * sin(angle);

    // Initial complex coordinates Z_0
    double zr = target_x + (sin(t * 220.0 * 2.0 * M_PI) / zoom);
    double zi = target_y + (cos(t * 220.0 * 2.0 * M_PI) / zoom);

    // Scale the overtone feedback so it introduces texture without completely exploding the orbits
    double overtone_injection_real = overtone_sine_sum * 0.04;
    double overtone_injection_imag = overtone_cosine_sum * 0.04;

    int iter = 0;
    while (zr * zr + zi * zi < 4.0 && iter < MAX_ITER) {
        // Standard Julia calculation: Z_{k+1} = Z_k^2 + c
        // Instantly modified by adding the random real and imaginary resonant overtone components.
        // Formula: Z_{k+1} = (Z_k^2 + c) + Z_overtones
        double temp = zr * zr - zi * zi + JULIA_RE + overtone_injection_real;
        zi = 2.0 * zr * zi + JULIA_IM + overtone_injection_imag;
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

    random_fd = open("/dev/random", O_RDONLY);
    if (random_fd < 0) {
        perror("Failed to open /dev/random, switching to /dev/urandom");
        random_fd = open("/dev/urandom", O_RDONLY);
        if (random_fd < 0) {
            fprintf(stderr, "Critical Error: No entropy source accessible.\n");
            return 1;
        }
    }

    memset(a_coefficients, 0, sizeof(a_coefficients));
    memset(b_coefficients, 0, sizeof(b_coefficients));

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
    printf("Streaming Julia Formula with integrated Overtone Entropy... Press Ctrl+C\n");

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
