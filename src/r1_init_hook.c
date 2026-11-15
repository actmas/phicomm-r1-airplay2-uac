#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

typedef long ssize_t;
typedef int pid_t;
typedef long time_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define O_RDONLY    00000000
#define O_WRONLY    00000001
#define O_RDWR      00000002
#define O_CREAT     00000100
#define O_TRUNC     00001000
#define O_NONBLOCK  00004000

#define POLLIN      0x0001
#define POLLHUP     0x0010

#define __NR_init_module 128

/* Errno */
#define EAGAIN      11
#define EWOULDBLOCK 11
extern int *__errno(void);
#define errno (*__errno())

/* Libc functions */
int open(const char *, int, ...);
int close(int);
int access(const char *, int);
ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);
int usleep(unsigned long);
int sleep(unsigned int);
long syscall(long, ...);
pid_t fork(void);
pid_t setsid(void);
int dup2(int, int);
void exit(int);
size_t strlen(const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
char *strncpy(char *, const char *, size_t);
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int vsnprintf(char *, size_t, const char *, va_list);
int snprintf(char *, size_t, const char *, ...);
int system(const char *);
int kill(pid_t, int);
struct timeval {
    time_t tv_sec;
    long tv_usec;
};
int gettimeofday(struct timeval *, void *);
time_t time(time_t *);
int ioctl(int, unsigned long, ...);
int unlink(const char *);
int mkdir(const char *, unsigned int);
int chmod(const char *, unsigned int);
int mount(const char *, const char *, const char *, unsigned long, const void *);
int mkfifo(const char *, unsigned int);
pid_t waitpid(pid_t, int *, int);
int execl(const char *, const char *, ...);
#define LOCK_EX 2
#define LOCK_NB 4
int flock(int, int);
#define F_SETPIPE_SZ 1031
int fcntl(int, int, ...);

/* Socket API */
#define AF_UNIX 1
#define AF_INET 2
#define SOCK_DGRAM 2
#define SOCK_STREAM 1

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};
int socket(int, int, int);
int bind(int, const void *, unsigned int);

/* Poll */
struct pollfd {
    int fd;
    short events;
    short revents;
};
int poll(struct pollfd *fds, unsigned long nfds, int timeout);

/* Pthread */
typedef unsigned long pthread_t;
typedef struct {
    int volatile value;
} pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER { 0 }

int pthread_create(pthread_t *, const void *, void *(*)(void *), void *);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);

/* Linux input event */
struct input_event {
    struct {
        long tv_sec;
        long tv_usec;
    } time;
    unsigned short type;
    unsigned short code;
    int value;
};
#define EV_SYN 0x00
#define EV_KEY 0x01
#define EV_ABS 0x03
#define ABS_X  0x00
#define KEY_VOLUMEDOWN 114
#define KEY_VOLUMEUP   115
#define KEY_MUTE       113

/* TinyALSA */
enum pcm_format {
    PCM_FORMAT_INVALID = -1,
    PCM_FORMAT_S16_LE = 0,
    PCM_FORMAT_S32_LE = 1,
    PCM_FORMAT_S8 = 2,
    PCM_FORMAT_S24_LE = 3,
    PCM_FORMAT_S24_3LE = 4,
    PCM_FORMAT_MAX,
};

#define PCM_OUT 0x00000000

struct pcm_config {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    enum pcm_format format;
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;
    int avail_min;
};

struct pcm;
struct mixer;
struct mixer_ctl;

struct pcm *pcm_open(unsigned int card, unsigned int device, unsigned int flags, struct pcm_config *config);
int pcm_is_ready(struct pcm *pcm);
const char *pcm_get_error(struct pcm *pcm);
int pcm_write(struct pcm *pcm, const void *data, unsigned int count);
int pcm_close(struct pcm *pcm);

struct mixer *mixer_open(unsigned int card);
void mixer_close(struct mixer *mixer);
struct mixer_ctl *mixer_get_ctl(struct mixer *mixer, unsigned int id);
struct mixer_ctl *mixer_get_ctl_by_name(struct mixer *mixer, const char *name);
int mixer_ctl_set_value(struct mixer_ctl *ctl, unsigned int id, int value);
int mixer_ctl_set_enum_by_string(struct mixer_ctl *ctl, const char *string);

/* Audio Config */
static struct pcm_config s_config_48k = {
    .channels = 2,
    .rate = 48000,
    .period_size = 1024,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 4096,
    .stop_threshold = 8192,
    .silence_threshold = 0,
    .avail_min = 1024,
};

static struct pcm_config s_config_44k = {
    .channels = 2,
    .rate = 44100,
    .period_size = 1024,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 2048,
    .stop_threshold = 4096,
    .silence_threshold = 0,
    .avail_min = 1024,
};

/* State */
static pthread_mutex_t s_audio_lock = PTHREAD_MUTEX_INITIALIZER;
static struct pcm *s_pcm = NULL;
static struct mixer *s_mixer = NULL;
static int s_active_rate = 0;
static int s_usb_playing = 0;
static int s_airplay_active = 0;
static int s_vol = 180;       // Default ~85% volume
static int s_is_muted = 0;
static int s_last_touch_pos = -1;

#define MIN_VOL 0
#define MAX_VOL 207

/* Klog helper */
static void klog(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (len > 0) {
        int fd = open("/dev/kmsg", O_WRONLY);
        if (fd >= 0) {
            write(fd, buf, (size_t)len);
            close(fd);
        }
    }
}

static void write_sys(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, val, strlen(val));
        close(fd);
    }
}

/* Embedded Kernel Module */
#include "uac1_gadget_blob.h"

static int s_dsp_initialized = 0;

/* Harman Kardon AK7755 Electronic Crossover & DSP Initialization */
static void init_harman_dsp(struct mixer *m) {
    if (!m) return;
    klog("<4>[r1_uac] === Initializing Harman Kardon AK7755 Electronic Crossover & DSP ===\n");

    struct mixer_ctl *ctl;

    /* 1. Memory partitioning required for active electronic crossover */
    ctl = mixer_get_ctl_by_name(m, "DRAM Size(Bank1:Bank0)");
    if (ctl) mixer_ctl_set_value(ctl, 0, 1); /* 1024:1024 */

    ctl = mixer_get_ctl_by_name(m, "DLRAM Mode(Bank1:Bank0)");
    if (ctl) mixer_ctl_set_value(ctl, 0, 2); /* 2048:6144 (time alignment delay line) */

    ctl = mixer_get_ctl_by_name(m, "POMODE DLRAM Pointer 0");
    if (ctl) mixer_ctl_set_value(ctl, 0, 1); /* OFREG (delay pointer offset from OFREG) */

    /* 2. Download Harman Kardon firmware / active crossover filter coefficients */
    ctl = mixer_get_ctl_by_name(m, "DSP Firmware PRAM");
    if (ctl) mixer_ctl_set_enum_by_string(ctl, "data2");

    ctl = mixer_get_ctl_by_name(m, "DSP Firmware CRAM");
    if (ctl) mixer_ctl_set_enum_by_string(ctl, "data2");

    ctl = mixer_get_ctl_by_name(m, "DSP Firmware OFREG");
    if (ctl) mixer_ctl_set_enum_by_string(ctl, "data2");

    /* 3. Direct DAC audio input from DSP active crossover output */
    ctl = mixer_get_ctl_by_name(m, "DAC MUX");
    if (ctl) mixer_ctl_set_enum_by_string(ctl, "DSP");

    /* 4. Power on Analog LineOut Amplifiers 1 & 2 (Tweeter & Woofer channels) */
    ctl = mixer_get_ctl_by_name(m, "LineOut Amp1");
    if (ctl) mixer_ctl_set_enum_by_string(ctl, "On");

    ctl = mixer_get_ctl_by_name(m, "LineOut Amp2");
    if (ctl) mixer_ctl_set_enum_by_string(ctl, "On");

    /* 5. Set analog Line Out volumes to max headroom (15) */
    ctl = mixer_get_ctl_by_name(m, "Line Out Volume 1");
    if (ctl) mixer_ctl_set_value(ctl, 0, 15);

    ctl = mixer_get_ctl_by_name(m, "Line Out Volume 2");
    if (ctl) mixer_ctl_set_value(ctl, 0, 15);

    ctl = mixer_get_ctl_by_name(m, "Line Out Volume 3");
    if (ctl) mixer_ctl_set_value(ctl, 0, 15);

    /* 6. Apply digital volume */
    ctl = mixer_get_ctl_by_name(m, "DAC Digital Volume L");
    if (ctl) mixer_ctl_set_value(ctl, 0, s_vol);

    ctl = mixer_get_ctl_by_name(m, "DAC Digital Volume R");
    if (ctl) mixer_ctl_set_value(ctl, 0, s_vol);

    s_dsp_initialized = 1;
    klog("<4>[r1_uac] === Harman Kardon AK7755 DSP Initialization Complete ===\n");
}

/* Mixer Controls */
static void set_amp_state(struct mixer *m, int active) {
    if (!m) return;
    struct mixer_ctl *dac_mute = mixer_get_ctl_by_name(m, "DAC Mute");
    struct mixer_ctl *amp1 = mixer_get_ctl_by_name(m, "LineOut Amp1");
    struct mixer_ctl *amp2 = mixer_get_ctl_by_name(m, "LineOut Amp2");
    struct mixer_ctl *lo_vol1 = mixer_get_ctl_by_name(m, "Line Out Volume 1");
    struct mixer_ctl *lo_vol2 = mixer_get_ctl_by_name(m, "Line Out Volume 2");

    if (active) {
        if (amp1) mixer_ctl_set_enum_by_string(amp1, "On");
        if (amp2) mixer_ctl_set_enum_by_string(amp2, "On");
        if (lo_vol1) mixer_ctl_set_value(lo_vol1, 0, 15);
        if (lo_vol2) mixer_ctl_set_value(lo_vol2, 0, 15);
        if (dac_mute) mixer_ctl_set_value(dac_mute, 0, 0); /* Unmute */
    } else {
        if (dac_mute) mixer_ctl_set_value(dac_mute, 0, 1); /* Mute */
    }
}

static void apply_volume(struct mixer *m, int vol) {
    if (!m) return;
    struct mixer_ctl *dac_l = mixer_get_ctl_by_name(m, "DAC Digital Volume L");
    struct mixer_ctl *dac_r = mixer_get_ctl_by_name(m, "DAC Digital Volume R");
    if (dac_l) mixer_ctl_set_value(dac_l, 0, vol);
    if (dac_r) mixer_ctl_set_value(dac_r, 0, vol);
}

/* Music Rhythm-Reactive Ambient Light */
static volatile int s_audio_energy = 0;

static void feed_audio_samples(const void *buf, size_t bytes) {
    const int16_t *s = (const int16_t *)buf;
    int count = (int)(bytes / 2);
    int32_t peak = 0;
    for (int i = 0; i < count; i += 8) {
        int32_t v = s[i];
        if (v < 0) v = -v;
        if (v > peak) peak = v;
    }
    if (peak > s_audio_energy) {
        s_audio_energy = peak;
    }
}

static unsigned long get_monotonic_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static void hsv_to_rgb(int h, int v, int *r, int *g, int *b) {
    if (v <= 0) { *r = 0; *g = 0; *b = 0; return; }
    h = h % 360;
    if (h < 0) h += 360;
    int region = h / 60;
    int remainder = (h - (region * 60)) * 255 / 60;
    int p = 0;
    int q = (v * (255 - remainder)) / 255;
    int t = (v * remainder) / 255;
    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

static void write_led_color(int r, int g, int b) {
    static int last_r = -1, last_g = -1, last_b = -1;
    if ((r || g || b) && r == last_r && g == last_g && b == last_b) {
        return;
    }
    last_r = r; last_g = g; last_b = b;

    char buf[16];
    int len = snprintf(buf, sizeof(buf), "7fff %02x%02x%02x", r, g, b);
    if (len != 11) return;

    int fd0 = open("/sys/class/leds/multi_leds0/led_color", O_WRONLY);
    if (fd0 >= 0) {
        write(fd0, buf, 11);
        close(fd0);
    }

    int fd1 = open("/sys/class/leds/multi_leds1/led_color", O_WRONLY);
    if (fd1 >= 0) {
        write(fd1, buf, 11);
        close(fd1);
    }
}

static void update_playback_light(int is_playing) {
    static int s_hue = 200; // Deep elegant blue/cyan start
    static int s_envelope = 0;
    static int s_was_playing = 0;

    if (!is_playing) {
        if (s_was_playing) {
            // Smooth musical fade out
            if (s_envelope > 500) {
                s_envelope = (s_envelope * 7) / 10;
                int brightness = 20 + (s_envelope * 60) / 10000;
                if (brightness > 255) brightness = 255;
                int r, g, b;
                hsv_to_rgb(s_hue, brightness, &r, &g, &b);
                write_led_color(r, g, b);
                return;
            }
            s_envelope = 0;
            s_was_playing = 0;
        }
        write_led_color(0, 0, 0);
        return;
    }

    if (s_is_muted) {
        write_led_color(255, 40, 0); // Warm amber when muted
        s_was_playing = 1;
        return;
    }

    s_was_playing = 1;

    // Read current audio peak and clear atomically
    int current_energy = s_audio_energy;
    s_audio_energy = 0;

    // Fast attack on beats, smooth musical decay
    if (current_energy > s_envelope) {
        s_envelope = (s_envelope * 3 + current_energy * 7) / 10;
    } else {
        s_envelope = (s_envelope * 88) / 100;
    }

    // Map audio peak (0-32767) to brightness (45-255)
    // 45 base brightness ensures a visible, elegant ambient glow at all times during playback!
    int brightness = 45 + (s_envelope * 210) / 14000;
    if (brightness > 255) brightness = 255;

    // Slowly cycle hue for living color ambiance
    s_hue = (s_hue + 1) % 360;

    int r, g, b;
    hsv_to_rgb(s_hue, brightness, &r, &g, &b);

    write_led_color(r, g, b);
}

/* Dedicated ambient light worker thread (completely isolated from ALSA playback) */
static void *ambient_light_thread(void *arg) {
    (void)arg;
    while (1) {
        int is_playing = (s_airplay_active || s_usb_playing);
        update_playback_light(is_playing);
        usleep(is_playing ? 60000 : 1000000);
    }
    return NULL;
}

/* Top Circular Touch Slider */
static void handle_touch_event(struct mixer *m, int touch_fd) {
    struct input_event ev[16];
    ssize_t n = read(touch_fd, ev, sizeof(ev));
    if (n <= 0) return;

    int count = (int)(n / (ssize_t)sizeof(struct input_event));
    for (int i = 0; i < count; i++) {
        if (ev[i].type == EV_KEY) {
            // Reset touch position tracking on finger touch-down or lift-up
            s_last_touch_pos = -1;
        } else if (ev[i].type == EV_ABS && ev[i].code == ABS_X) {
            int pos = ev[i].value;
            if (s_last_touch_pos >= 0) {
                int delta = pos - s_last_touch_pos;
                // Circular wrap-around for 256-position ring (0..255)
                if (delta > 128) delta -= 256;
                else if (delta < -128) delta += 256;

                // Spurious jump filter: finger lift/land gives large jumps
                if (delta > 30 || delta < -30) {
                    delta = 0;
                }

                if (delta != 0) {
                    s_vol += delta * 2;
                    if (s_vol < MIN_VOL) s_vol = MIN_VOL;
                    if (s_vol > MAX_VOL) s_vol = MAX_VOL;
                    if (s_is_muted) s_is_muted = 0;
                    apply_volume(m, s_vol);
                    klog("<4>[r1_uac] Touch slider: pos=%d delta=%d -> vol=%d/%d\n",
                         pos, delta, s_vol, MAX_VOL);
                }
            }
            s_last_touch_pos = pos;
        }
    }
}

/* Center Mute Button */
static void handle_key_event(struct mixer *m, int key_fd) {
    struct input_event ev[8];
    ssize_t n = read(key_fd, ev, sizeof(ev));
    if (n <= 0) return;

    int count = (int)(n / (ssize_t)sizeof(struct input_event));
    for (int i = 0; i < count; i++) {
        if (ev[i].type == EV_KEY && ev[i].value == 1) { // Key down
            s_is_muted = !s_is_muted;
            if (s_is_muted) {
                apply_volume(m, 0);
                klog("<4>[r1_uac] Mute button PRESSED -> MUTED\n");
            } else {
                apply_volume(m, s_vol);
                klog("<4>[r1_uac] Mute button PRESSED -> UNMUTED (vol=%d)\n", s_vol);
            }
        }
    }
}

/* --- Supervisor Threads for AirPlay 2 Daemons --- */

static void *nqptp_supervisor_thread(void *arg) {
    (void)arg;
    while (1) {
        if (access("/data/local/airplay2/nqptp", 0) == 0) {
            klog("<4>[r1_uac] Starting NQPTP PTP daemon...\n");
            pid_t pid = fork();
            if (pid == 0) {
                for (int i = 3; i < 1024; i++) {
                    close(i);
                }
                int fd = open("/proc/self/oom_score_adj", O_WRONLY);
                if (fd >= 0) {
                    write(fd, "-1000", 5);
                    close(fd);
                }
                execl("/data/local/airplay2/nqptp", "nqptp", "-v", (char *)NULL);
                exit(1);
            }
            if (pid > 0) {
                int status = 0;
                waitpid(pid, &status, 0);
                klog("<3>[r1_uac] NQPTP exited (status=%d), restarting in 1s...\n", status);
            }
        }
        sleep(1);
    }
    return NULL;
}

static void *shairport_supervisor_thread(void *arg) {
    (void)arg;
    sleep(2); // Wait for NQPTP to bind ports 319/320 and initialize shared memory
    while (1) {
        if (access("/data/local/airplay2/shairport-sync", 0) == 0) {
            klog("<4>[r1_uac] Starting Shairport-Sync AirPlay 2 daemon...\n");
            pid_t pid = fork();
            if (pid == 0) {
                for (int i = 3; i < 1024; i++) {
                    close(i);
                }
                int fd = open("/proc/self/oom_score_adj", O_WRONLY);
                if (fd >= 0) {
                    write(fd, "-1000", 5);
                    close(fd);
                }
                execl("/data/local/airplay2/shairport-sync",
                      "shairport-sync",
                      "-c", "/data/local/airplay2/shairport-sync.conf",
                      (char *)NULL);
                exit(1);
            }
            if (pid > 0) {
                int status = 0;
                waitpid(pid, &status, 0);
                klog("<3>[r1_uac] Shairport-Sync exited (status=%d), restarting in 1s...\n", status);
            }
        }
        sleep(1);
    }
    return NULL;
}

/* --- Main Unified Audio Loop --- */

static void run_uac1_bridge(void) {
    klog("<4>[r1_uac] bridge: starting unified AirPlay 2 + USB DAC daemon...\n");

    int uac_fd = -1;
    for (int retry = 0; retry < 50; retry++) {
        uac_fd = open("/dev/uac1_audio", O_RDONLY);
        if (uac_fd >= 0) break;
        usleep(100000);
    }
    if (uac_fd >= 0) {
        klog("<4>[r1_uac] bridge: /dev/uac1_audio opened (fd=%d)\n", uac_fd);
    } else {
        klog("<3>[r1_uac] WARN: /dev/uac1_audio not ready yet, running in AirPlay 2 mode\n");
    }

    int touch_fd = -1;
    for (int retry = 0; retry < 30; retry++) {
        touch_fd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
        if (touch_fd >= 0) break;
        usleep(100000);
    }
    klog("<4>[r1_uac] bridge: /dev/input/event1 opened (fd=%d)\n", touch_fd);

    int key_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
    klog("<4>[r1_uac] bridge: /dev/input/event0 opened (fd=%d)\n", key_fd);

    int ap2_pipe_fd = open("/dev/shm/airplay2_audio", O_RDWR | O_NONBLOCK);
    if (ap2_pipe_fd >= 0) {
        fcntl(ap2_pipe_fd, F_SETPIPE_SZ, 524288);
    }
    klog("<4>[r1_uac] bridge: /dev/shm/airplay2_audio opened (fd=%d)\n", ap2_pipe_fd);

    s_mixer = mixer_open(2);
    if (s_mixer) {
        init_harman_dsp(s_mixer);
    }

    char audio_buf[4096];
    struct pollfd pfds[4];
    time_t last_airplay_activity = 0;

    while (1) {
        if (uac_fd < 0) {
            uac_fd = open("/dev/uac1_audio", O_RDONLY);
            if (uac_fd >= 0) {
                klog("<4>[r1_uac] bridge: /dev/uac1_audio opened (fd=%d)\n", uac_fd);
            }
        }
        pfds[0].fd = uac_fd;
        pfds[0].events = (uac_fd >= 0) ? POLLIN : 0;
        pfds[0].revents = 0;

        pfds[1].fd = touch_fd;
        pfds[1].events = (touch_fd >= 0) ? POLLIN : 0;
        pfds[1].revents = 0;

        pfds[2].fd = key_fd;
        pfds[2].events = (key_fd >= 0) ? POLLIN : 0;
        pfds[2].revents = 0;

        if (ap2_pipe_fd < 0) {
            ap2_pipe_fd = open("/dev/shm/airplay2_audio", O_RDWR | O_NONBLOCK);
            if (ap2_pipe_fd >= 0) {
                fcntl(ap2_pipe_fd, F_SETPIPE_SZ, 524288);
            }
        }
        pfds[3].fd = ap2_pipe_fd;
        pfds[3].events = (ap2_pipe_fd >= 0) ? POLLIN : 0;
        pfds[3].revents = 0;

        int ret = poll(pfds, 4, 300); // 300ms timeout
        time_t now = time(NULL);

        // Process touch slider
        if (ret > 0 && touch_fd >= 0 && (pfds[1].revents & POLLIN)) {
            handle_touch_event(s_mixer, touch_fd);
        }

        // Process center button
        if (ret > 0 && key_fd >= 0 && (pfds[2].revents & POLLIN)) {
            handle_key_event(s_mixer, key_fd);
        }

        // Process AirPlay 2 stream (High Priority)
        if (ret > 0 && ap2_pipe_fd >= 0 && (pfds[3].revents & (POLLIN | POLLHUP))) {
            static size_t ap2_rem = 0;
            static char ap2_frame_buf[8192 + 4];
            while (1) {
                ssize_t n = read(ap2_pipe_fd, ap2_frame_buf + ap2_rem, sizeof(ap2_frame_buf) - 4 - ap2_rem);
                if (n > 0) {
                    last_airplay_activity = now;
                    size_t total = ap2_rem + (size_t)n;
                    size_t valid = total & ~3; // Align to 4-byte stereo 16-bit frame
                    ap2_rem = total - valid;

                    if (valid > 0) {
                        feed_audio_samples(ap2_frame_buf, valid);
                        pthread_mutex_lock(&s_audio_lock);
                        if (!s_airplay_active || !s_pcm || s_active_rate != 48000) {
                            if (s_pcm) {
                                pcm_close(s_pcm);
                                s_pcm = NULL;
                                s_active_rate = 0;
                                usleep(10000); // 10ms settle time
                            }
                            if (!s_mixer) s_mixer = mixer_open(2);
                            if (s_mixer && !s_dsp_initialized) init_harman_dsp(s_mixer);
                            set_amp_state(s_mixer, 1);
                            apply_volume(s_mixer, s_vol);

                            s_pcm = pcm_open(2, 0, PCM_OUT, &s_config_48k);
                            if (s_pcm && pcm_is_ready(s_pcm)) {
                                s_active_rate = 48000;
                                s_airplay_active = 1;
                                klog("<4>[r1_uac] AirPlay 2 ACTIVE @ 48000Hz (vol=%d)\n", s_vol);
                            } else {
                                klog("<3>[r1_uac] AirPlay 2 pcm_open failed\n");
                                if (s_pcm) { pcm_close(s_pcm); s_pcm = NULL; }
                                set_amp_state(s_mixer, 0);
                            }
                        }
                        if (s_pcm && s_active_rate == 48000 && !s_is_muted) {
                            pcm_write(s_pcm, ap2_frame_buf, (unsigned int)valid);
                        }
                        pthread_mutex_unlock(&s_audio_lock);
                    }

                    if (ap2_rem > 0) {
                        memcpy(ap2_frame_buf, ap2_frame_buf + valid, ap2_rem);
                    }
                } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    break; // Fully drained pending pipe data
                } else {
                    close(ap2_pipe_fd);
                    ap2_pipe_fd = open("/dev/shm/airplay2_audio", O_RDWR | O_NONBLOCK);
                    if (ap2_pipe_fd >= 0) {
                        fcntl(ap2_pipe_fd, F_SETPIPE_SZ, 524288);
                    }
                    break;
                }
            }
        }

        // Check AirPlay 2 idle timeout (1.5 seconds)
        if (s_airplay_active && (now - last_airplay_activity >= 2)) {
            pthread_mutex_lock(&s_audio_lock);
            if (s_airplay_active) {
                if (s_pcm) {
                    pcm_close(s_pcm);
                    s_pcm = NULL;
                    s_active_rate = 0;
                    usleep(10000);
                }
                if (s_mixer && !s_usb_playing) {
                    set_amp_state(s_mixer, 0);
                }
                s_airplay_active = 0;
                klog("<4>[r1_uac] AirPlay 2 IDLE (Amps OFF)\n");
            }
            pthread_mutex_unlock(&s_audio_lock);
        }

        // Process USB Audio stream
        if (s_airplay_active) {
            // Discard USB audio while AirPlay 2 is actively playing
            if (ret > 0 && (pfds[0].revents & POLLIN)) {
                read(uac_fd, audio_buf, sizeof(audio_buf));
            }
        } else if (ret > 0 && (pfds[0].revents & POLLIN)) {
            ssize_t n = read(uac_fd, audio_buf, sizeof(audio_buf));
            if (n > 0) {
                feed_audio_samples(audio_buf, (size_t)n);
                pthread_mutex_lock(&s_audio_lock);
                if (!s_airplay_active) {
                    if (!s_usb_playing || !s_pcm || s_active_rate != 48000) {
                        if (s_pcm) {
                            pcm_close(s_pcm);
                            s_pcm = NULL;
                            s_active_rate = 0;
                            usleep(10000);
                        }
                        if (!s_mixer) s_mixer = mixer_open(2);
                        if (s_mixer && !s_dsp_initialized) init_harman_dsp(s_mixer);
                        set_amp_state(s_mixer, 1);
                        apply_volume(s_mixer, s_vol);

                        s_pcm = pcm_open(2, 0, PCM_OUT, &s_config_48k);
                        if (s_pcm && pcm_is_ready(s_pcm)) {
                            s_active_rate = 48000;
                            s_usb_playing = 1;
                            klog("<4>[r1_uac] USB playback ACTIVE @ 48000Hz (vol=%d)\n", s_vol);
                        } else {
                            klog("<3>[r1_uac] USB pcm_open failed\n");
                            if (s_pcm) { pcm_close(s_pcm); s_pcm = NULL; }
                            set_amp_state(s_mixer, 0);
                        }
                    }
                    if (s_pcm && s_active_rate == 48000 && !s_is_muted) {
                        pcm_write(s_pcm, audio_buf, (unsigned int)n);
                    }
                }
                pthread_mutex_unlock(&s_audio_lock);
            }
        } else if (ret == 0 || (pfds[0].revents & POLLHUP)) {
            pthread_mutex_lock(&s_audio_lock);
            if (s_usb_playing && !s_airplay_active) {
                if (s_pcm) {
                    pcm_close(s_pcm);
                    s_pcm = NULL;
                    s_active_rate = 0;
                    usleep(10000);
                }
                if (s_mixer) {
                    set_amp_state(s_mixer, 0);
                }
                s_usb_playing = 0;
                klog("<4>[r1_uac] USB playback IDLE (Amps OFF)\n");
            }
            pthread_mutex_unlock(&s_audio_lock);
        }
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // Singleton check via Linux Abstract UNIX domain socket.
    // Abstract sockets start with '\0', have NO filesystem permissions or mount dependencies,
    // and are automatically cleaned up by the kernel on process termination.
    int lock_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (lock_sock >= 0) {
        struct sockaddr_un sun;
        memset(&sun, 0, sizeof(sun));
        sun.sun_family = AF_UNIX;
        sun.sun_path[0] = 0; // Abstract namespace
        const char *lock_name = "r1_uac_firmware_hook_lock";
        size_t name_len = strlen(lock_name);
        if (name_len > sizeof(sun.sun_path) - 2) name_len = sizeof(sun.sun_path) - 2;
        memcpy(sun.sun_path + 1, lock_name, name_len);
        unsigned int addrlen = (unsigned int)(sizeof(unsigned short) + 1 + name_len);
        if (bind(lock_sock, (const void *)&sun, addrlen) < 0) {
            close(lock_sock);
            return 0; // Another instance is already running! Silently exit immediately.
        }
    }

    // Mount tmpfs on /dev/shm only if not already mounted
    if (access("/dev/shm/airplay2_audio", 0) != 0 && access("/dev/shm/nqptp", 0) != 0) {
        mkdir("/dev/shm", 0777);
        mount("tmpfs", "/dev/shm", "tmpfs", 0, "size=8M");
        chmod("/dev/shm", 0777);
    }

    klog("<4>[r1_uac] init_hook v6 Unified (AirPlay 2 + USB DAC) started\n");

    // Load embedded uac1_gadget.ko directly from memory (patches SELinux to permissive mode!)
    long mret = syscall(__NR_init_module, uac1_gadget_ko, (unsigned long)uac1_gadget_ko_len, "");
    if (mret == 0) {
        klog("<4>[r1_uac] init_module SUCCEEDED! uac1_gadget loaded & SELinux patched\n");
    } else {
        klog("<3>[r1_uac] init_module returned %ld (errno=%d)\n", mret, errno);
    }
    klog("<4>[r1_uac] /dev/shm mounted as tmpfs (8MB)\n");

    // 2. Create AirPlay 2 FIFO
    unlink("/dev/shm/airplay2_audio");
    mkfifo("/dev/shm/airplay2_audio", 0666);
    chmod("/dev/shm/airplay2_audio", 0666);
    int pfd = open("/dev/shm/airplay2_audio", O_RDWR | O_NONBLOCK);
    if (pfd >= 0) {
        fcntl(pfd, F_SETPIPE_SZ, 524288);
        close(pfd);
    }

    // 3. Configure USB gadget to uac1,adb
    write_sys("/sys/class/android_usb/android0/enable", "0");
    write_sys("/sys/class/android_usb/android0/idVendor", "2207");
    write_sys("/sys/class/android_usb/android0/idProduct", "0010");
    write_sys("/sys/class/android_usb/android0/functions", "uac1,adb");
    write_sys("/sys/class/android_usb/android0/enable", "1");
    klog("<4>[r1_uac] USB gadget configured to uac1,adb\n");

    // 4. Spawn background unified audio daemon
    pid_t bpid = fork();
    if (bpid == 0) {
        setsid();
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, 0);
            dup2(devnull, 1);
            dup2(devnull, 2);
            close(devnull);
        }

        // Set LMK immunity for bridge and all spawned children
        int fd = open("/proc/self/oom_score_adj", O_WRONLY);
        if (fd >= 0) {
            write(fd, "-1000", 5);
            close(fd);
        }

        // Start NQPTP PTP clock sync supervisor thread
        pthread_t ptpid;
        pthread_create(&ptpid, NULL, nqptp_supervisor_thread, NULL);

        // Start Shairport-Sync AirPlay 2 supervisor thread
        pthread_t ap2id;
        pthread_create(&ap2id, NULL, shairport_supervisor_thread, NULL);

        // Start Ambient Light thread (completely decoupled from ALSA audio loop!)
        pthread_t light_tid;
        pthread_create(&light_tid, NULL, ambient_light_thread, NULL);

        // Run USB DAC + AirPlay 2 bridge in main thread of child
        while (1) {
            run_uac1_bridge();
            sleep(1);
        }
        exit(0);
    }
    klog("<4>[r1_uac] unified audio bridge spawned with pid %d\n", (int)bpid);

    exit(0);
}
