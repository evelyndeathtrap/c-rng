#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 44100
#define CHANNELS 1
#define ITERATIONS 255

int main() {
    long loops;
    int rc;
    int size;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    short *buffer;

    /* Open PCM device for playback */
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) return 1;

    /* Set hardware parameters */
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);
    val = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
    
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) return 1;

    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames * 2 * CHANNELS; 
    buffer = (short *) malloc(size);

    double x_start = -2.0, y_start = -1.2;
    double x_end = 1.0, y_end = 1.2;
    double step = 0.0001;
    double cx = x_start, cy = y_start;

    while (1) {
        for (int i = 0; i < frames; i++) {
            double zx = 0, zy = 0;
            int iter = 0;
            
            // Mandelbrot iteration: z = z^2 + c
            while (zx*zx + zy*zy < 4 && iter < ITERATIONS) {
                double xtemp = zx*zx - zy*zy + cx;
                zy = 2*zx*zy + cy;
                zx = xtemp;
                iter++;
            }

            // Map iteration count to a 16-bit PCM sample
            buffer[i] = (short)((iter * 256) - 32768);

            // Advance through the complex plane
            cx += step;
            if (cx > x_end) {
                cx = x_start;
                cy += step;
                if (cy > y_end) cy = y_start;
            }
        }

        rc = snd_pcm_writei(handle, buffer, frames);
        if (rc == -EPIPE) snd_pcm_prepare(handle);
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return 0;
}
