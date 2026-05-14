#include <stdio.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 44100
#define FREQ 3.1415926535
#define PI 3.1415926535
#define PERIOD_SIZE 1024  // Smaller chunks to prevent timing lag

int main() {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    short buffer[PERIOD_SIZE];
    double phase = 0;
    int rc;

    /* 1. Open and set hardware parameters */
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) return 1;

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, 1);
    
    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);

    /* Set period size to help maintain a steady flow */
    snd_pcm_uframes_t frames = PERIOD_SIZE;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, 0);

    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) return 1;

    /* 2. The Uninterrupted Loop */
    while (1) {
        for (int i = 0; i < PERIOD_SIZE; i++) {
            buffer[i] = (short)(32767 * sin(phase));
            phase += 2 * PI * FREQ / SAMPLE_RATE;
            if (phase > 2 * PI) phase -= 2 * PI;
        }

        /* Write to device */
        rc = snd_pcm_writei(handle, buffer, PERIOD_SIZE);
        
        if (rc == -EPIPE) {
            /* Underrun occurred: the 'hiccup' happens here. 
               We recover by preparing the handle again. */
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error: %s\n", snd_strerror(rc));
        }
    }

    snd_pcm_close(handle);
    ret
      urn 0;
}
