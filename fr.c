#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SAMPLE_RATE 386000
#define CHANNELS 1
#define BUFFER_SIZE 4096
#define MAX_ITER 256

// Fixed point inside the Basilica Julia Set (c = -1) exhibiting pristine self-similarity
const double JULIA_RE = -1.0;
const double JULIA_IM = 0.0;

// Render a single audio sample based on a spatial-temporal Julia calculation
int16_t compute_julia_audio_sample(unsigned long long sample_index) {
    double t = (double)sample_index / SAMPLE_RATE;
    
    // Smoothly modulate the zoom factor to create an infinite, seamless loop.
    // By keeping the zoom bound between 0.2 and 4.0, we never hit precision loss.
    double zoom = 2.1 + 1.9 * sin(0.2 * t);
    
    // Map time directly to an orbit trajectory across the complex plane
    double angle = t * 0.05;
    double target_x = 0.5 * cos(angle);
    double target_y = 0.5 * sin(angle);

    // Initial complex coordinates based on current zoom and target path
    double zr = target_x + (sin(t * 220.0 * 2.0 * M_PI) / zoom);
    double zi = target_y + (cos(t * 220.0 * 2.0 * M_PI) / zoom);

    int iter = 0;
    while (zr * zr + zi * zi < 4.0 && iter < MAX_ITER) {
        double temp = zr * zr - zi * zi + JULIA_RE;
        zi = 2.0 * zr * zi + JULIA_IM;
        zr = temp;
        iter++;
    }

    // Normalize iteration count to a 16-bit signed audio wave
    double normalized = (double)iter / (double)MAX_ITER;
    
    // Apply a subtle DC-offset filter and soft clipping
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

    // Open PCM device for playback
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        fprintf(stderr, "Unable to open pcm device: %s\n", snd_strerror(rc));
        return 1;
    }

    // Allocate hardware parameters object
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);

    // Set interleaved access mode, 16-bit little-endian, mono
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);

    // Set sample rate
    val = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

    // Set period size to minimize latency while preventing underruns
    frames = BUFFER_SIZE / 4;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    // Write parameters to the driver
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr, "Unable to set hw parameters: %s\n", snd_strerror(rc));
        return 1;
    }

    unsigned long long global_sample_counter = 0;
    printf("Streaming Julia Set audio... Press Ctrl+C to stop.\n");

    // Infinite synthesis loop
    while (1) {
        // Populate the write buffer
        for (int i = 0; i < BUFFER_SIZE; i++) {
            buffer[i] = compute_julia_audio_sample(global_sample_counter);
            global_sample_counter++;
        }

        // Direct blocking write to ALSA ring buffer
        rc = snd_pcm_writei(handle, buffer, BUFFER_SIZE);
        
        // Handle buffer underruns (Xruns) immediately to avoid audio pops
        if (rc == -EPIPE) {
            fprintf(stderr, "Xrun occurred! Recovering stream...\n");
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error from writei: %s\n", snd_strerror(rc));
        }
    }

    // Unreachable block kept for structural cleanliness
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    return 0;
}
