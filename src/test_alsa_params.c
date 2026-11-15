#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

struct pcm_config {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    int format;
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;
    int avail_min;
};

struct pcm;
extern struct pcm *pcm_open(unsigned int card, unsigned int device, unsigned int flags, struct pcm_config *config);
extern int pcm_is_ready(struct pcm *pcm);
extern const char *pcm_get_error(struct pcm *pcm);
extern int pcm_close(struct pcm *pcm);

int main() {
    unsigned int rates[] = { 44100, 48000 };
    unsigned int period_sizes[] = { 256, 512, 1024, 2048, 4096 };
    unsigned int period_counts[] = { 2, 4, 8, 16 };

    for (int r = 0; r < 2; r++) {
        for (int p = 0; p < 5; p++) {
            for (int c = 0; c < 4; c++) {
                struct pcm_config cfg = {
                    .channels = 2,
                    .rate = rates[r],
                    .period_size = period_sizes[p],
                    .period_count = period_counts[c],
                    .format = 0, // PCM_FORMAT_S16_LE
                    .start_threshold = period_sizes[p] * (period_counts[c] / 2),
                    .stop_threshold = period_sizes[p] * period_counts[c],
                    .silence_threshold = 0,
                    .avail_min = period_sizes[p],
                };
                struct pcm *pcm = pcm_open(2, 0, 0, &cfg);
                if (pcm && pcm_is_ready(pcm)) {
                    printf("OK: rate=%u period_size=%u period_count=%u (buf=%u samples, %.1f ms)\n",
                           rates[r], period_sizes[p], period_counts[c],
                           period_sizes[p] * period_counts[c],
                           (double)(period_sizes[p] * period_counts[c]) * 1000.0 / rates[r]);
                    pcm_close(pcm);
                } else {
                    // printf("FAIL: rate=%u period_size=%u period_count=%u: %s\n",
                    //        rates[r], period_sizes[p], period_counts[c], pcm ? pcm_get_error(pcm) : "null");
                    if (pcm) pcm_close(pcm);
                }
                usleep(5000);
            }
        }
    }
    return 0;
}
