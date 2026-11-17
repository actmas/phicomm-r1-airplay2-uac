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
void *mmap(void *, size_t, int, int, int, long);
int munmap(void *, size_t);

/* Socket API */
#define AF_UNIX   1
#define AF_INET   2
#define AF_PACKET 17
#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#define SIOCGIFFLAGS  0x8913
#define SIOCSIFFLAGS  0x8914
#define SIOCGIFADDR   0x8915
#define SIOCGIFHWADDR 0x8927
#define SIOCGIFINDEX  0x8933
#define IFF_ALLMULTI  0x0200
#define ETH_P_ARP     0x0806

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};

struct sockaddr_ll {
    unsigned short sll_family;
    unsigned short sll_protocol;
    int sll_ifindex;
    unsigned short sll_hatype;
    unsigned char sll_pkttype;
    unsigned char sll_halen;
    unsigned char sll_addr[8];
};

int socket(int, int, int);
int bind(int, const void *, unsigned int);
ssize_t sendto(int, const void *, size_t, int, const void *, unsigned int);

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
static int s_usb_vol = 180;
static int s_usb_muted = 0;
static int s_airplay_active = 0;
static int s_bt_playing = 0;

static int buf_has_running_state(const char *buf, int len) {
    const char pat[] = "RUNNING";
    for (int i = 0; i <= len - 7; i++) {
        int match = 1;
        for (int j = 0; j < 7; j++) {
            if (buf[i + j] != pat[j]) { match = 0; break; }
        }
        if (match) return 1;
    }
    return 0;
}

static int is_card2_externally_active(void) {
    int fd = open("/proc/asound/card2/pcm0p/sub0/status", O_RDONLY);
    if (fd < 0) return 0;
    char buf[64];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    return buf_has_running_state(buf, (int)n);
}
static int s_vol = 231;       // Default 0 dB unity gain on AK7755 DAC (range 0..255)
static int s_is_muted = 0;
static int s_last_touch_pos = -1;

#define MIN_VOL 0
#define MAX_VOL 255

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
static volatile unsigned long s_last_nonzero_audio_ms = 0;

static unsigned long get_monotonic_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static void feed_audio_samples(const void *buf, size_t bytes) {
    const int16_t *s = (const int16_t *)buf;
    int count = (int)(bytes / 2);
    int32_t peak = 0;
    for (int i = 0; i < count; i += 8) {
        int32_t v = s[i];
        if (v < 0) v = -v;
        if (v > peak) peak = v;
    }
    if (peak > 24) {
        s_last_nonzero_audio_ms = get_monotonic_ms();
    }
    if (peak > s_audio_energy) {
        s_audio_energy = peak;
    }
}

static void hsv_to_rgb(int h, int v, int *r, int *g, int *b) {
    if (v <= 0) { *r = 0; *g = 0; *b = 0; return; }
    h = h % 360;
    if (h < 0) h += 360;
    int region = h / 60;
    int remainder = (h - (region * 60)) * 255 / 60;
    /* Soft saturation (~65% saturation) so transitions blend organically without harsh neon jumps */
    int p = (v * 35) / 100;
    int span = v - p;
    int q = p + (span * (255 - remainder)) / 255;
    int t = p + (span * remainder) / 255;
    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

static void cancel_led_effects(void) {
    static const char close_leds0[] =
        "100 1 1 1 000000 000000 100 0 15 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15";
    static const char close_leds1[] =
        "100 1 1 1 000000 000000 100 0 24 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39";
    int fd = open("/sys/class/leds/multi_leds0/color", O_WRONLY);
    if (fd >= 0) {
        write(fd, close_leds0, sizeof(close_leds0) - 1);
        close(fd);
    }
    fd = open("/sys/class/leds/multi_leds1/color", O_WRONLY);
    if (fd >= 0) {
        write(fd, close_leds1, sizeof(close_leds1) - 1);
        close(fd);
    }
}

static void write_led_color(int r, int g, int b) {
    static int last_r = -1, last_g = -1, last_b = -1;
    static int off_refresh = 0;
    if (r == 0 && g == 0 && b == 0) {
        if (last_r == 0 && last_g == 0 && last_b == 0) {
            if (++off_refresh < 4) return;
        } else {
            cancel_led_effects();
        }
        off_refresh = 0;
    } else if (last_r >= 0) {
        off_refresh = 0;
        int dr = r - last_r; if (dr < 0) dr = -dr;
        int dg = g - last_g; if (dg < 0) dg = -dg;
        int db = b - last_b; if (db < 0) db = -db;
        if (dr < 2 && dg < 2 && db < 2) return;
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

static int update_playback_light(int is_playing) {
    static int s_hue_x10 = 2000; /* 200.0 deg deep blue/cyan start */
    static int s_envelope = 0;
    static int s_smooth_bri = 0;

    if (!is_playing) {
        s_envelope = 0;
        if (s_smooth_bri > 0) {
            s_smooth_bri -= 16;
            if (s_smooth_bri < 0) s_smooth_bri = 0;
        }
        if (s_smooth_bri > 0) {
            int v_out = (s_smooth_bri * s_smooth_bri) / 255;
            if (v_out < 8) v_out = 8;
            int r, g, b;
            hsv_to_rgb(s_hue_x10 / 10, v_out, &r, &g, &b);
            write_led_color(r, g, b);
            return 1;
        }
        write_led_color(0, 0, 0);
        return 0;
    }

    if (s_is_muted) {
        write_led_color(255, 40, 0); /* Warm amber when muted */
        return 1;
    }

    /* Read current audio peak and clear atomically */
    int current_energy = s_audio_energy;
    s_audio_energy = 0;

    /* Heavy low-pass envelope at 120ms cadence: 15% attack, 90% decay */
    if (current_energy > s_envelope) {
        s_envelope = (s_envelope * 85 + current_energy * 15) / 100;
    } else {
        s_envelope = (s_envelope * 90) / 100;
    }

    int target_bri;
    if (s_bt_playing && !s_airplay_active && !s_usb_playing) {
        /* Gentle breathing glow during Bluetooth/Android playback */
        static int bt_phase = 0;
        bt_phase = (bt_phase + 1) % 40;
        int tri = (bt_phase < 20) ? bt_phase : (40 - bt_phase); /* 0..20 */
        target_bri = 100 + (tri * 90) / 20; /* 100..190 */
    } else {
        target_bri = 90 + (s_envelope * 145) / 16000;
        if (target_bri > 235) target_bri = 235;
    }

    /* Slew-rate limit brightness changes (<= 4 per 120ms frame) to prevent visible flicker */
    int diff = target_bri - s_smooth_bri;
    if (diff > 4) diff = 4;
    else if (diff < -4) diff = -4;
    s_smooth_bri += diff;

    /* Perceptual gamma curve (v^2 / 255), floor at 24 to avoid low-bit PWM stepping */
    int v_out = (s_smooth_bri * s_smooth_bri) / 255;
    if (v_out < 24 && s_smooth_bri > 0) v_out = 24;

    /* Slow, organic hue drift: +0.5 deg per 120ms frame (~86s per full cycle) */
    s_hue_x10 = (s_hue_x10 + 5) % 3600;

    int r, g, b;
    hsv_to_rgb(s_hue_x10 / 10, v_out, &r, &g, &b);

    write_led_color(r, g, b);
    return 1;
}

/* Dedicated ambient light worker thread (completely isolated from ALSA playback) */
static void *ambient_light_thread(void *arg) {
    (void)arg;
    unsigned int led_tick = 0;
    while (1) {
        if (led_tick % 40 == 0) {
            /* Restrict LED sysfs nodes to root so stock Phicomm services cannot overwrite idle LEDs */
            chmod("/sys/class/leds/multi_leds0/led_color", 0600);
            chmod("/sys/class/leds/multi_leds0/color", 0600);
            chmod("/sys/class/leds/multi_leds0/brightness", 0600);
            chmod("/sys/class/leds/multi_leds1/led_color", 0600);
            chmod("/sys/class/leds/multi_leds1/color", 0600);
            chmod("/sys/class/leds/multi_leds1/brightness", 0600);
            if (led_tick <= 80) {
                cancel_led_effects();
            }
        }
        led_tick++;
        int has_pcm_audio = 0;
        if (s_airplay_active || s_usb_playing) {
            unsigned long now = get_monotonic_ms();
            if (s_last_nonzero_audio_ms != 0 && (now - s_last_nonzero_audio_ms) <= 1500) {
                has_pcm_audio = 1;
            }
        }
        int is_playing = (has_pcm_audio || s_bt_playing);
        int active_or_fading = update_playback_light(is_playing);
        usleep(active_or_fading ? 120000 : 250000);
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

#define WIFI_IFNAMSIZ 16
#define WIFI_SIOCDEVPRIVATE 0x89F0
#define WLC_SET_PM  86
#define WLC_SET_VAR 263

struct wifi_ifreq {
    char ifr_name[WIFI_IFNAMSIZ];
    union {
        char pad[16];
        short ifr_flags;
        int ifr_ifindex;
        unsigned char ifr_raw[16];
        void *ifr_data;
    };
};

struct wl_ioctl_cmd {
    uint32_t cmd;
    void *buf;
    uint32_t len;
    uint8_t set;
    uint32_t used;
    uint32_t needed;
};

struct android_wifi_priv_cmd {
    char *buf;
    int used_len;
    int total_len;
};

static unsigned short htons_be16(unsigned short v) {
    return (unsigned short)((v << 8) | (v >> 8));
}

static int wlan0_priv_cmd(int sock, const char *cmd_str) {
    struct wifi_ifreq ifr;
    struct android_wifi_priv_cmd priv_cmd;
    char buf[128];

    memset(&ifr, 0, sizeof(ifr));
    memset(&priv_cmd, 0, sizeof(priv_cmd));
    memset(buf, 0, sizeof(buf));
    strncpy(ifr.ifr_name, "wlan0", WIFI_IFNAMSIZ - 1);
    strncpy(buf, cmd_str, sizeof(buf) - 1);
    priv_cmd.buf = buf;
    priv_cmd.used_len = (int)strlen(buf) + 1;
    priv_cmd.total_len = sizeof(buf);
    ifr.ifr_data = &priv_cmd;
    return ioctl(sock, WIFI_SIOCDEVPRIVATE + 1, &ifr);
}

static int wlan0_wlc_set_pm(int sock, int pm) {
    struct wifi_ifreq ifr;
    struct wl_ioctl_cmd ioc;
    int val = pm;

    memset(&ifr, 0, sizeof(ifr));
    memset(&ioc, 0, sizeof(ioc));
    strncpy(ifr.ifr_name, "wlan0", WIFI_IFNAMSIZ - 1);
    ioc.cmd = WLC_SET_PM;
    ioc.buf = &val;
    ioc.len = sizeof(val);
    ioc.set = 1;
    ifr.ifr_data = &ioc;
    return ioctl(sock, WIFI_SIOCDEVPRIVATE, &ifr);
}

static int wlan0_wlc_set_iovar(int sock, const char *name, int val) {
    struct wifi_ifreq ifr;
    struct wl_ioctl_cmd ioc;
    char buf[64];
    size_t nlen = strlen(name) + 1;

    if (nlen + sizeof(int) > sizeof(buf)) return -1;
    memset(&ifr, 0, sizeof(ifr));
    memset(&ioc, 0, sizeof(ioc));
    memset(buf, 0, sizeof(buf));
    memcpy(buf, name, nlen);
    memcpy(buf + nlen, &val, sizeof(int));
    strncpy(ifr.ifr_name, "wlan0", WIFI_IFNAMSIZ - 1);
    ioc.cmd = WLC_SET_VAR;
    ioc.buf = buf;
    ioc.len = (uint32_t)(nlen + sizeof(int));
    ioc.set = 1;
    ifr.ifr_data = &ioc;
    return ioctl(sock, WIFI_SIOCDEVPRIVATE, &ifr);
}

static void wlan0_ensure_allmulti(int sock) {
    struct wifi_ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "wlan0", WIFI_IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0 && !(ifr.ifr_flags & IFF_ALLMULTI)) {
        ifr.ifr_flags |= IFF_ALLMULTI;
        ioctl(sock, SIOCSIFFLAGS, &ifr);
    }
}

static int get_wlan0_info(int ctl_sock, int *ifidx, unsigned char mac[6], unsigned char ip[4]) {
    struct wifi_ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "wlan0", WIFI_IFNAMSIZ - 1);
    if (ioctl(ctl_sock, SIOCGIFINDEX, &ifr) < 0) return -1;
    *ifidx = ifr.ifr_ifindex;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "wlan0", WIFI_IFNAMSIZ - 1);
    if (ioctl(ctl_sock, SIOCGIFHWADDR, &ifr) < 0) return -1;
    memcpy(mac, ifr.ifr_raw + 2, 6);
    if ((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0) return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "wlan0", WIFI_IFNAMSIZ - 1);
    if (ioctl(ctl_sock, SIOCGIFADDR, &ifr) < 0) return -1;
    memcpy(ip, ifr.ifr_raw + 4, 4);
    if ((ip[0] | ip[1] | ip[2] | ip[3]) == 0) return -1;
    return 0;
}

static void wait_for_wlan0_ip(int max_wait_sec) {
    for (int elapsed = 0; elapsed < max_wait_sec; elapsed++) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock >= 0) {
            int ifidx = 0;
            unsigned char mac[6];
            unsigned char ip[4];
            int ok = get_wlan0_info(sock, &ifidx, mac, ip);
            close(sock);
            if (ok == 0) return;
        }
        sleep(1);
    }
}

static void send_arp_frame(int ps, int ifidx,
                           const unsigned char dst_mac[6],
                           const unsigned char src_mac[6],
                           const unsigned char src_ip[4],
                           unsigned short op,
                           const unsigned char tha[6],
                           const unsigned char tpa[4]) {
    struct sockaddr_ll sll;
    unsigned char pkt[42];

    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons_be16(ETH_P_ARP);
    sll.sll_ifindex = ifidx;
    sll.sll_halen = 6;
    memcpy(sll.sll_addr, dst_mac, 6);

    memcpy(pkt, dst_mac, 6);
    memcpy(pkt + 6, src_mac, 6);
    pkt[12] = 0x08; pkt[13] = 0x06; /* EtherType ARP */
    pkt[14] = 0x00; pkt[15] = 0x01; /* HTYPE Ethernet */
    pkt[16] = 0x08; pkt[17] = 0x00; /* PTYPE IPv4 */
    pkt[18] = 6;    pkt[19] = 4;    /* HLEN 6, PLEN 4 */
    pkt[20] = (unsigned char)((op >> 8) & 0xff);
    pkt[21] = (unsigned char)(op & 0xff);
    memcpy(pkt + 22, src_mac, 6);
    memcpy(pkt + 28, src_ip, 4);
    memcpy(pkt + 32, tha, 6);
    memcpy(pkt + 38, tpa, 4);
    sendto(ps, pkt, sizeof(pkt), 0, &sll, sizeof(sll));
}

static int parse_ipv4_token(const char *s, unsigned char out[4]) {
    unsigned int val = 0;
    int part = 0, digits = 0;
    for (; *s && *s != ' ' && *s != '\t'; s++) {
        if (*s >= '0' && *s <= '9') {
            val = val * 10U + (unsigned int)(*s - '0');
            if (val > 255U) return -1;
            digits++;
        } else if (*s == '.' && digits > 0 && part < 3) {
            out[part++] = (unsigned char)val;
            val = 0;
            digits = 0;
        } else {
            return -1;
        }
    }
    if (part != 3 || digits == 0) return -1;
    out[3] = (unsigned char)val;
    return 0;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_mac_token(const char *s, unsigned char out[6]) {
    for (int i = 0; i < 6; i++) {
        int hi = hex_nibble(s[i * 3]);
        int lo = hex_nibble(s[i * 3 + 1]);
        if (hi < 0 || lo < 0) return -1;
        if (i < 5 && s[i * 3 + 2] != ':') return -1;
        out[i] = (unsigned char)((hi << 4) | lo);
    }
    return 0;
}

/* Broadcast Gratuitous ARP + unicast ARP Request to known peers.
 * bcmdhd often drops inbound broadcast ARP requests on R1's wlan0;
 * proactive outbound ARP requests keep client ARP caches fresh.
 */
static void refresh_wlan0_arp(int ifidx, const unsigned char my_mac[6], const unsigned char my_ip[4]) {
    static const unsigned char bcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    static const unsigned char zero_mac[6] = {0, 0, 0, 0, 0, 0};

    int ps = socket(AF_PACKET, SOCK_RAW, htons_be16(ETH_P_ARP));
    if (ps < 0) return;

    send_arp_frame(ps, ifidx, bcast_mac, my_mac, my_ip, 1, zero_mac, my_ip);
    send_arp_frame(ps, ifidx, bcast_mac, my_mac, my_ip, 2, bcast_mac, my_ip);

    int afd = open("/proc/net/arp", O_RDONLY);
    if (afd >= 0) {
        char buf[4096];
        ssize_t n = read(afd, buf, sizeof(buf) - 1);
        close(afd);
        if (n > 0) {
            buf[n] = '\0';
            char *p = buf;
            while (*p) {
                char *line = p;
                while (*p && *p != '\n') p++;
                if (*p == '\n') *p++ = '\0';

                unsigned char peer_ip[4];
                if (parse_ipv4_token(line, peer_ip) != 0) continue;
                if (peer_ip[0] == my_ip[0] && peer_ip[1] == my_ip[1] &&
                    peer_ip[2] == my_ip[2] && peer_ip[3] == my_ip[3]) continue;

                const char *cols[6] = {0, 0, 0, 0, 0, 0};
                int col = 0;
                char *q = line;
                while (*q && col < 6) {
                    while (*q == ' ' || *q == '\t') q++;
                    if (!*q) break;
                    cols[col++] = q;
                    while (*q && *q != ' ' && *q != '\t') q++;
                }
                if (col < 6) continue;
                if (strncmp(cols[5], "wlan0", 5) != 0) continue;
                if (strncmp(cols[2], "0x0", 3) == 0 && (cols[2][3] == ' ' || cols[2][3] == '\t' || cols[2][3] == '\0')) {
                    continue;
                }
                unsigned char peer_mac[6];
                if (parse_mac_token(cols[3], peer_mac) != 0) continue;
                if ((peer_mac[0] | peer_mac[1] | peer_mac[2] |
                     peer_mac[3] | peer_mac[4] | peer_mac[5]) == 0) continue;

                send_arp_frame(ps, ifidx, peer_mac, my_mac, my_ip, 1, zero_mac, peer_ip);
            }
        }
    }
    close(ps);
}

/* --- Bluetooth Discoverable Keepalive (Android Binder) --- */

#define BINDER_PROT_READ       1
#define BINDER_MAP_PRIVATE     2
#define BINDER_MAP_FAILED      ((void *)-1)
#define BINDER_WRITE_READ      0xc0186201UL
#define BINDER_SET_MAX_THREADS 0x40046205UL
#define BINDER_TYPE_BINDER     0x73622a85U
#define BC_TRANSACTION         0x40286300U
#define BC_FREE_BUFFER         0x40046303U
#define BC_ACQUIRE             0x40046305U
#define BC_ENTER_LOOPER        0x0000630cU
#define BR_NOOP                0x0000720cU
#define BR_TRANSACTION_COMPLETE 0x00007206U
#define BR_REPLY               0x80287203U
#define BR_FAILED_REPLY        0x00007211U
#define BR_DEAD_REPLY          0x00007205U

struct r1_binder_write_read {
    size_t write_size;
    size_t write_consumed;
    unsigned long write_buffer;
    size_t read_size;
    size_t read_consumed;
    unsigned long read_buffer;
};

struct r1_binder_txn_data {
    union { uint32_t handle; void *ptr; } target;
    void *cookie;
    uint32_t code;
    uint32_t flags;
    int32_t sender_pid;
    uint32_t sender_euid;
    size_t data_size;
    size_t offsets_size;
    union {
        struct { const void *buffer; const void *offsets; } ptr;
        uint8_t buf[8];
    } data;
};

struct r1_flat_binder_obj {
    uint32_t type;
    uint32_t flags;
    union { void *binder; uint32_t handle; };
    void *cookie;
};

struct r1_parcel {
    uint8_t data[512];
    size_t pos;
    size_t offsets[8];
    size_t num_offsets;
};

static void r1_parcel_init(struct r1_parcel *p) {
    memset(p, 0, sizeof(*p));
}

static void r1_parcel_i32(struct r1_parcel *p, int32_t v) {
    memcpy(p->data + p->pos, &v, 4);
    p->pos += 4;
}

static void r1_parcel_str16(struct r1_parcel *p, const char *s) {
    size_t len = strlen(s);
    r1_parcel_i32(p, (int32_t)len);
    for (size_t i = 0; i < len; i++) {
        uint16_t ch = (uint8_t)s[i];
        memcpy(p->data + p->pos, &ch, 2);
        p->pos += 2;
    }
    uint16_t nul = 0;
    memcpy(p->data + p->pos, &nul, 2);
    p->pos += 2;
    while (p->pos & 3) p->data[p->pos++] = 0;
}

static void r1_parcel_iface(struct r1_parcel *p, const char *iface) {
    r1_parcel_i32(p, 0);
    r1_parcel_str16(p, iface);
}

static void r1_parcel_local_binder(struct r1_parcel *p, void *ptr) {
    struct r1_flat_binder_obj obj;
    memset(&obj, 0, sizeof(obj));
    obj.type = BINDER_TYPE_BINDER;
    obj.flags = 0x7f;
    obj.binder = ptr;
    obj.cookie = ptr;
    p->offsets[p->num_offsets++] = p->pos;
    memcpy(p->data + p->pos, &obj, sizeof(obj));
    p->pos += sizeof(obj);
}

static int r1_binder_transact(int fd, uint32_t handle, uint32_t code, struct r1_parcel *in,
                              uint8_t *reply_out, size_t *reply_len, uint32_t *reply_handle) {
    struct {
        uint32_t cmd;
        struct r1_binder_txn_data txn;
    } __attribute__((packed)) wbuf;
    memset(&wbuf, 0, sizeof(wbuf));
    wbuf.cmd = BC_TRANSACTION;
    wbuf.txn.target.handle = handle;
    wbuf.txn.code = code;
    wbuf.txn.flags = 0x10;
    wbuf.txn.data_size = in->pos;
    wbuf.txn.offsets_size = in->num_offsets * sizeof(size_t);
    wbuf.txn.data.ptr.buffer = in->data;
    wbuf.txn.data.ptr.offsets = in->num_offsets ? in->offsets : NULL;

    uint8_t rbuf[512];
    struct r1_binder_write_read bwr;
    memset(&bwr, 0, sizeof(bwr));
    bwr.write_size = sizeof(wbuf);
    bwr.write_buffer = (unsigned long)&wbuf;
    bwr.read_size = sizeof(rbuf);
    bwr.read_buffer = (unsigned long)rbuf;

    while (1) {
        if (ioctl(fd, BINDER_WRITE_READ, &bwr) < 0) return -1;
        size_t off = 0;
        while (off + 4 <= bwr.read_consumed) {
            uint32_t cmd;
            memcpy(&cmd, rbuf + off, 4);
            off += 4;
            if (cmd == BR_NOOP || cmd == BR_TRANSACTION_COMPLETE) continue;
            if (cmd == BR_FAILED_REPLY || cmd == BR_DEAD_REPLY) return -1;
            if (cmd == BR_REPLY) {
                struct r1_binder_txn_data tr;
                memcpy(&tr, rbuf + off, sizeof(tr));
                off += sizeof(tr);
                if (reply_out && reply_len) {
                    size_t n = tr.data_size < *reply_len ? tr.data_size : *reply_len;
                    memcpy(reply_out, tr.data.ptr.buffer, n);
                    *reply_len = n;
                }
                if (reply_handle && tr.offsets_size >= sizeof(size_t)) {
                    size_t obj_off = 0;
                    memcpy(&obj_off, tr.data.ptr.offsets, sizeof(size_t));
                    struct r1_flat_binder_obj obj;
                    memcpy(&obj, (const uint8_t *)tr.data.ptr.buffer + obj_off, sizeof(obj));
                    *reply_handle = obj.handle;
                    struct { uint32_t cmd; uint32_t handle; } __attribute__((packed)) abuf = { BC_ACQUIRE, obj.handle };
                    struct r1_binder_write_read abwr;
                    memset(&abwr, 0, sizeof(abwr));
                    abwr.write_size = sizeof(abuf);
                    abwr.write_buffer = (unsigned long)&abuf;
                    ioctl(fd, BINDER_WRITE_READ, &abwr);
                }
                struct { uint32_t cmd; const void *ptr; } __attribute__((packed)) fbuf = { BC_FREE_BUFFER, tr.data.ptr.buffer };
                struct r1_binder_write_read fbwr;
                memset(&fbwr, 0, sizeof(fbwr));
                fbwr.write_size = sizeof(fbuf);
                fbwr.write_buffer = (unsigned long)&fbuf;
                ioctl(fd, BINDER_WRITE_READ, &fbwr);
                return 0;
            }
        }
        bwr.write_size = 0;
        bwr.write_consumed = 0;
        bwr.read_consumed = 0;
    }
}

static void ensure_bt_discoverable(void) {
    static int bt_fd = -1;
    static void *bt_map = BINDER_MAP_FAILED;
    static uint32_t ibt_handle = 0;
    static int dummy_cb = 1;

    if (bt_fd < 0) {
        bt_fd = open("/dev/binder", O_RDWR);
        if (bt_fd < 0) return;
        bt_map = mmap(NULL, 128 * 1024, BINDER_PROT_READ, BINDER_MAP_PRIVATE, bt_fd, 0);
        if (bt_map == BINDER_MAP_FAILED) {
            close(bt_fd);
            bt_fd = -1;
            return;
        }
        uint32_t max_t = 0;
        ioctl(bt_fd, BINDER_SET_MAX_THREADS, &max_t);
        uint32_t enter = BC_ENTER_LOOPER;
        struct r1_binder_write_read ebwr;
        memset(&ebwr, 0, sizeof(ebwr));
        ebwr.write_size = sizeof(enter);
        ebwr.write_buffer = (unsigned long)&enter;
        ioctl(bt_fd, BINDER_WRITE_READ, &ebwr);
        ibt_handle = 0;
    }

    struct r1_parcel p;
    uint8_t reply[128];
    size_t rlen;

    if (ibt_handle == 0) {
        uint32_t mgr_handle = 0;
        rlen = sizeof(reply);
        r1_parcel_init(&p);
        r1_parcel_iface(&p, "android.os.IServiceManager");
        r1_parcel_str16(&p, "bluetooth_manager");
        if (r1_binder_transact(bt_fd, 0, 1, &p, reply, &rlen, &mgr_handle) < 0 || mgr_handle == 0) {
            goto reset_binder;
        }
        rlen = sizeof(reply);
        r1_parcel_init(&p);
        r1_parcel_iface(&p, "android.bluetooth.IBluetoothManager");
        r1_parcel_local_binder(&p, &dummy_cb);
        if (r1_binder_transact(bt_fd, mgr_handle, 1, &p, reply, &rlen, &ibt_handle) < 0 || ibt_handle == 0) {
            goto reset_binder;
        }
    }

    rlen = sizeof(reply);
    r1_parcel_init(&p);
    r1_parcel_iface(&p, "android.bluetooth.IBluetooth");
    if (r1_binder_transact(bt_fd, ibt_handle, 10, &p, reply, &rlen, NULL) < 0 || rlen < 8) {
        goto reset_binder;
    }
    int32_t exc = 0, mode = 0;
    memcpy(&exc, reply, 4);
    memcpy(&mode, reply + 4, 4);
    if (exc == 0 && mode != 23) {
        rlen = sizeof(reply);
        r1_parcel_init(&p);
        r1_parcel_iface(&p, "android.bluetooth.IBluetooth");
        r1_parcel_i32(&p, 23); // SCAN_MODE_CONNECTABLE_DISCOVERABLE
        r1_parcel_i32(&p, 0);  // infinite duration
        if (r1_binder_transact(bt_fd, ibt_handle, 11, &p, reply, &rlen, NULL) == 0) {
            klog("<4>[r1_uac] Bluetooth scan mode %d -> 23 (discoverable)\n", mode);
        }
    }
    return;

reset_binder:
    if (bt_map != BINDER_MAP_FAILED) munmap(bt_map, 128 * 1024);
    bt_map = BINDER_MAP_FAILED;
    if (bt_fd >= 0) close(bt_fd);
    bt_fd = -1;
    ibt_handle = 0;
}

static void *wifi_keepalive_thread(void *arg) {
    (void)arg;
    // Acquire partial wake lock so the kernel does not suspend networking
    write_sys("/sys/power/wake_lock", "r1_airplay_keepalive");

    // Wait for wlan0 and Bluetooth service to initialize after boot
    sleep(5);

    unsigned int tick = 0;
    while (1) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock >= 0) {
            wlan0_ensure_allmulti(sock);
            if (tick % 10 == 0) {
                wlan0_priv_cmd(sock, "SETSUSPENDMODE 0");
            }
            wlan0_priv_cmd(sock, "RXFILTER-STOP");
            wlan0_priv_cmd(sock, "PM 0");
            wlan0_priv_cmd(sock, "BTCOEXMODE 2");
            wlan0_wlc_set_pm(sock, 0);
            wlan0_wlc_set_iovar(sock, "mpc", 0);
            wlan0_wlc_set_iovar(sock, "arp_ol", 0);
            wlan0_wlc_set_iovar(sock, "bcn_li_dtim", 1);
            wlan0_wlc_set_iovar(sock, "bcn_li_bcn", 1);
            wlan0_wlc_set_iovar(sock, "btc_mode", 2);
            wlan0_wlc_set_iovar(sock, "allmulti", 1);

            int ifidx = 0;
            unsigned char my_mac[6];
            unsigned char my_ip[4];
            if (get_wlan0_info(sock, &ifidx, my_mac, my_ip) == 0) {
                refresh_wlan0_arp(ifidx, my_mac, my_ip);
            }
            close(sock);
        }
        if (tick >= 3 && (tick % 3 == 0)) {
            ensure_bt_discoverable();
        }
        tick++;
        sleep(10);
    }
    return NULL;
}

static void *nqptp_supervisor_thread(void *arg) {
    (void)arg;
    wait_for_wlan0_ip(30);
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
    wait_for_wlan0_ip(30);
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
    int last_uac_vol_seq = -1;

    while (1) {
        if (uac_fd < 0) {
            uac_fd = open("/dev/uac1_audio", O_RDONLY);
            if (uac_fd >= 0) {
                klog("<4>[r1_uac] bridge: /dev/uac1_audio opened (fd=%d)\n", uac_fd);
            }
        }
        if (uac_fd >= 0) {
            int vinfo = ioctl(uac_fd, 0x5501, 0UL);
            if (vinfo >= 0) {
                int seq = (vinfo >> 16) & 0x7fff;
                if (last_uac_vol_seq < 0) {
                    last_uac_vol_seq = seq;
                } else if (seq != last_uac_vol_seq) {
                    last_uac_vol_seq = seq;
                    s_usb_vol = vinfo & 0xff;
                    s_usb_muted = (vinfo >> 8) & 1;
                    if (s_mixer && !s_airplay_active && !s_bt_playing) {
                        apply_volume(s_mixer, s_usb_muted ? 0 : s_usb_vol);
                    }
                }
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
                    last_airplay_activity = time(NULL);
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
                                s_bt_playing = 0;
                                klog("<4>[r1_uac] AirPlay 2 ACTIVE @ 48000Hz (vol=%d)\n", s_vol);
                            } else {
                                klog("<3>[r1_uac] AirPlay 2 pcm_open failed\n");
                                if (s_pcm) { pcm_close(s_pcm); s_pcm = NULL; }
                                set_amp_state(s_mixer, 0);
                            }
                        }
                        if (s_pcm && s_active_rate == 48000 && !s_is_muted) {
                            if (pcm_write(s_pcm, ap2_frame_buf, (unsigned int)valid) != 0) {
                                klog("<3>[r1_uac] AirPlay 2 pcm_write failed, resetting PCM\n");
                                pcm_close(s_pcm);
                                s_pcm = NULL;
                                s_active_rate = 0;
                            }
                        }
                        pthread_mutex_unlock(&s_audio_lock);
                        last_airplay_activity = time(NULL);
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

        // Check AirPlay 2 idle timeout (2 seconds)
        now = time(NULL);
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
                        apply_volume(s_mixer, s_usb_muted ? 0 : s_usb_vol);

                        s_pcm = pcm_open(2, 0, PCM_OUT, &s_config_48k);
                        if (s_pcm && pcm_is_ready(s_pcm)) {
                            s_active_rate = 48000;
                            s_usb_playing = 1;
                            s_bt_playing = 0;
                            klog("<4>[r1_uac] USB playback ACTIVE @ 48000Hz (usb_vol=%d)\n", s_usb_vol);
                        } else {
                            klog("<3>[r1_uac] USB pcm_open failed\n");
                            if (s_pcm) { pcm_close(s_pcm); s_pcm = NULL; }
                            set_amp_state(s_mixer, 0);
                        }
                    }
                    if (s_pcm && s_active_rate == 48000 && !s_usb_muted) {
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

        // Unmute AK7755 DAC when Android mediaserver (e.g., Bluetooth A2DP Sink) opens card 2 while AirPlay/USB are idle
        if (!s_airplay_active && !s_usb_playing) {
            int ext_active = is_card2_externally_active();
            if (ext_active && !s_bt_playing) {
                pthread_mutex_lock(&s_audio_lock);
                if (!s_airplay_active && !s_usb_playing && !s_pcm) {
                    if (!s_mixer) s_mixer = mixer_open(2);
                    if (s_mixer && !s_dsp_initialized) init_harman_dsp(s_mixer);
                    set_amp_state(s_mixer, 1);
                    apply_volume(s_mixer, s_vol);
                    s_bt_playing = 1;
                    klog("<4>[r1_uac] Bluetooth/Android audio ACTIVE (Amps ON, vol=%d)\n", s_vol);
                }
                pthread_mutex_unlock(&s_audio_lock);
            } else if (!ext_active && s_bt_playing) {
                pthread_mutex_lock(&s_audio_lock);
                if (!s_airplay_active && !s_usb_playing && s_bt_playing) {
                    if (s_mixer) set_amp_state(s_mixer, 0);
                    s_bt_playing = 0;
                    klog("<4>[r1_uac] Bluetooth/Android audio IDLE (Amps OFF)\n");
                }
                pthread_mutex_unlock(&s_audio_lock);
            }
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

        // Start Wi-Fi keepalive thread (disables bcmdhd power-save & RX filter so AirPlay stays reachable)
        pthread_t wifi_tid;
        pthread_create(&wifi_tid, NULL, wifi_keepalive_thread, NULL);

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
