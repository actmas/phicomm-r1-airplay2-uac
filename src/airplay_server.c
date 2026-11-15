#include <stddef.h>
#include <stdint.h>

/* Shairport functions in /system/lib/libshairport.so */
extern int set_airplay_hostname(const char *name);
extern int set_airplay_hwAddr(const char *hwaddr);
extern int set_airplay_port(const char *port);
extern int start_airplay(void);
extern int stop_airplay(void);
extern void *audio_dummy;

typedef struct {
    void (*help)(void);
    const char *name;
    int (*init)(int argc, char **argv);
    void (*deinit)(void);
    void (*start)(int sample_rate);
    void (*play)(short *buf, int samples);
    void (*stop)(void);
    void (*volume)(double vol);
} audio_output_t;

/* TinyALSA definitions */
enum pcm_format {
    PCM_FORMAT_INVALID = -1,
    PCM_FORMAT_S16_LE = 0,
    PCM_FORMAT_S32_LE = 1,
    PCM_FORMAT_S8 = 2,
    PCM_FORMAT_S24_LE = 3,
    PCM_FORMAT_S24_3LE = 4,
    PCM_FORMAT_MAX,
};

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
int mixer_ctl_set_value(struct mixer_ctl *ctl, unsigned int id, int value);
int mixer_ctl_set_enum_by_string(struct mixer_ctl *ctl, const char *string);

#define PCM_OUT 0x00000000

/* Socket API */
#define AF_INET 2
#define SOCK_DGRAM 2
#define IPPROTO_UDP 17
#define IPPROTO_IP 0
#define IP_MULTICAST_TTL 33
#define IP_ADD_MEMBERSHIP 35
#define SO_REUSEADDR 2
#define SOL_SOCKET 1

#define SIOCGIFADDR   0x8915
#define SIOCGIFHWADDR 0x8927

struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct ip_mreq {
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
};

struct ifreq {
    char ifr_name[16];
    union {
        struct sockaddr_in ifr_addr;
        struct {
            short sa_family;
            unsigned char sa_data[14];
        } ifr_hwaddr;
        short ifr_flags;
        char ifr_data[24];
    };
};

/* Poll & Input API */
struct pollfd {
    int fd;
    short events;
    short revents;
};
#define POLLIN 0x0001
int poll(struct pollfd *fds, unsigned long nfds, int timeout);

struct input_event {
    struct {
        long tv_sec;
        long tv_usec;
    } time;
    unsigned short type;
    unsigned short code;
    int value;
};
#define EV_KEY    0x01
#define EV_ABS    0x03
#define ABS_X     0x00
#define BTN_TOUCH 0x14a

/* Standard Libc functions */
int printf(const char *fmt, ...);
int snprintf(char *str, size_t size, const char *format, ...);
void exit(int status);
int sleep(unsigned int seconds);
int usleep(unsigned long usec);
int socket(int domain, int type, int protocol);
int bind(int sockfd, const void *addr, unsigned int addrlen);
int setsockopt(int sockfd, int level, int optname, const void *optval, unsigned int optlen);
long sendto(int sockfd, const void *buf, size_t len, int flags, const void *dest_addr, unsigned int addrlen);
long recvfrom(int sockfd, void *buf, size_t len, int flags, void *src_addr, unsigned int *addrlen);
int close(int fd);
int open(const char *pathname, int flags, ...);
int getpid(void);
long read(int fd, void *buf, size_t count);
long write(int fd, const void *buf, size_t count);
int unlink(const char *pathname);
int pthread_create(void *thread, const void *attr, void *(*start_routine)(void *), void *arg);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
int system(const char *command);
long time(long *tloc);
int ioctl(int fd, unsigned long request, ...);
int fork(void);
int setsid(void);
int dup2(int oldfd, int newfd);

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0100
#define O_TRUNC  01000

/* State */
static struct pcm *s_pcm = NULL;
static struct mixer *s_mixer = NULL;
static volatile int s_airplay_active = 0;
static volatile unsigned long s_total_samples = 0;
static int s_vol = 165; // Factory Harman/Kardon default volume
static int s_is_muted = 0;
static int s_touching = 0;
static int s_last_pos = -1;

/* Multi-color breathing ambient light: 5-color cycle */
static const int s_breathe_colors[] = {
    511, // Deep Blue
    514, // Aqua Cyan
    510, // Sunset Orange
    515, // Passion Red
    512  // Ambient White
};
#define NUM_BREATHE_COLORS 5
static int s_color_idx = 0;
static long s_last_color_time = 0;

static void update_playback_light(int is_playing) {
    if (!is_playing || s_is_muted) {
        if (s_last_color_time != 0) {
            system("/system/bin/lights_test show 504 >/dev/null 2>&1 &");
            system("/system/bin/lights_test show 505 >/dev/null 2>&1 &");
            s_last_color_time = 0;
        }
        return;
    }

    long now = time(NULL);
    if (s_last_color_time == 0 || (now - s_last_color_time) >= 4) {
        int color_id = s_breathe_colors[s_color_idx];
        s_color_idx = (s_color_idx + 1) % NUM_BREATHE_COLORS;
        s_last_color_time = now;

        char cmd[64];
        snprintf(cmd, sizeof(cmd), "/system/bin/lights_test show %d >/dev/null 2>&1 &", color_id);
        system(cmd);
    }
}

static void set_amps(int on) {
    if (!s_mixer) s_mixer = mixer_open(2);
    if (!s_mixer) return;

    if (on) {
        // Line Out PGA (12/15) - Factory Harman/Kardon gain
        struct mixer_ctl *lo1 = mixer_get_ctl(s_mixer, 2);
        if (lo1) mixer_ctl_set_value(lo1, 0, 12);
        struct mixer_ctl *lo2 = mixer_get_ctl(s_mixer, 3);
        if (lo2) mixer_ctl_set_value(lo2, 0, 12);

        // Volume (AK7755 digital attenuation 0..231)
        struct mixer_ctl *vl = mixer_get_ctl(s_mixer, 10);
        if (vl) mixer_ctl_set_value(vl, 0, s_vol);
        struct mixer_ctl *vr = mixer_get_ctl(s_mixer, 11);
        if (vr) mixer_ctl_set_value(vr, 0, s_vol);

        // DAC Mute Off (0) if not muted
        struct mixer_ctl *mute = mixer_get_ctl(s_mixer, 14);
        if (mute) mixer_ctl_set_value(mute, 0, s_is_muted ? 1 : 0);

        // LineOut Amps 1 & 2 On
        struct mixer_ctl *a1 = mixer_get_ctl(s_mixer, 69);
        if (a1) mixer_ctl_set_enum_by_string(a1, "On");
        struct mixer_ctl *a2 = mixer_get_ctl(s_mixer, 70);
        if (a2) mixer_ctl_set_enum_by_string(a2, "On");

        update_playback_light(1);
    } else {
        struct mixer_ctl *a1 = mixer_get_ctl(s_mixer, 69);
        if (a1) mixer_ctl_set_enum_by_string(a1, "Off");
        struct mixer_ctl *a2 = mixer_get_ctl(s_mixer, 70);
        if (a2) mixer_ctl_set_enum_by_string(a2, "Off");

        struct mixer_ctl *mute = mixer_get_ctl(s_mixer, 14);
        if (mute) mixer_ctl_set_value(mute, 0, 1);

        update_playback_light(0);
    }
}

static void *touch_listener_thread(void *arg) {
    (void)arg;
    int touch_fd = -1;
    int key_fd = -1;

    for (int retry = 0; retry < 60; retry++) {
        if (touch_fd < 0) touch_fd = open("/dev/input/event1", O_RDONLY);
        if (key_fd < 0) key_fd = open("/dev/input/event0", O_RDONLY);
        if (touch_fd >= 0 && key_fd >= 0) break;
        sleep(1);
    }

    struct pollfd pfds[2];

    while (1) {
        pfds[0].fd = touch_fd;
        pfds[0].events = (touch_fd >= 0) ? POLLIN : 0;
        pfds[0].revents = 0;

        pfds[1].fd = key_fd;
        pfds[1].events = (key_fd >= 0) ? POLLIN : 0;
        pfds[1].revents = 0;

        int ret = poll(pfds, 2, 500);
        if (ret <= 0) continue;

        if (touch_fd >= 0 && (pfds[0].revents & POLLIN)) {
            struct input_event ev;
            while (read(touch_fd, &ev, sizeof(ev)) == (long)sizeof(ev)) {
                if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                    s_touching = (ev.value == 1);
                    s_last_pos = -1;
                } else if (ev.type == EV_ABS && ev.code == ABS_X) {
                    int cur_pos = ev.value & 0xFF;
                    if (s_touching) {
                        if (s_last_pos >= 0) {
                            int diff = cur_pos - s_last_pos;
                            if (diff > 128) diff -= 256;
                            else if (diff < -128) diff += 256;

                            if (diff >= -50 && diff <= 50) {
                                s_vol += diff;
                                if (s_vol > 231) s_vol = 231;
                                if (s_vol < 0) s_vol = 0;

                                if (s_is_muted && diff > 0) {
                                    s_is_muted = 0;
                                    if (s_mixer) {
                                        struct mixer_ctl *m = mixer_get_ctl(s_mixer, 14);
                                        if (m) mixer_ctl_set_value(m, 0, 0);
                                    }
                                    if (s_airplay_active) update_playback_light(1);
                                }

                                if (s_mixer) {
                                    struct mixer_ctl *vl = mixer_get_ctl(s_mixer, 10);
                                    if (vl) mixer_ctl_set_value(vl, 0, s_vol);
                                    struct mixer_ctl *vr = mixer_get_ctl(s_mixer, 11);
                                    if (vr) mixer_ctl_set_value(vr, 0, s_vol);
                                }
                            }
                        }
                        s_last_pos = cur_pos;
                    }
                }
            }
        }

        if (key_fd >= 0 && (pfds[1].revents & POLLIN)) {
            struct input_event ev;
            while (read(key_fd, &ev, sizeof(ev)) == (long)sizeof(ev)) {
                if (ev.type == EV_KEY && ev.value == 1) { // Center button press
                    s_is_muted = !s_is_muted;
                    if (s_mixer) {
                        struct mixer_ctl *m = mixer_get_ctl(s_mixer, 14);
                        if (m) mixer_ctl_set_value(m, 0, s_is_muted ? 1 : 0);
                    }
                    if (s_airplay_active) update_playback_light(!s_is_muted);
                }
            }
        }
    }
    return NULL;
}

static void my_output_help(void) {}
static int my_output_init(int argc, char **argv) {
    (void)argc; (void)argv;
    return 0;
}
static void my_output_deinit(void) {}

static void my_output_start(int sample_rate) {
    printf("[*** AirPlay ***] Audio Stream STARTING @ %d Hz\n", sample_rate);
    s_airplay_active = 1;
    s_total_samples = 0;

    // Signal status file for dual-mode USB DAC coexistence
    int fd = open("/data/local/tmp/airplay_playing", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        write(fd, "1\n", 2);
        close(fd);
    }

    // Short wait to ensure USB DAC yields Card 2 if playing
    usleep(50000);

    struct pcm_config config;
    config.channels = 2;
    config.rate = (unsigned int)sample_rate;
    config.period_size = 1024;
    config.period_count = 4;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 1024;
    config.stop_threshold = 1024 * 4;
    config.silence_threshold = 0;
    config.avail_min = 0;

    set_amps(1);

    s_pcm = pcm_open(2, 0, PCM_OUT, &config);
    if (s_pcm && pcm_is_ready(s_pcm)) {
        printf("[*** AirPlay ***] TinyALSA Card 2 opened successfully @ %d Hz!\n", sample_rate);
    } else {
        printf("[-] TinyALSA pcm_open failed: %s\n", s_pcm ? pcm_get_error(s_pcm) : "null");
        if (s_pcm) { pcm_close(s_pcm); s_pcm = NULL; }
    }
}

static void my_output_play(short *buf, int samples) {
    if (s_pcm) {
        pcm_write(s_pcm, buf, (unsigned int)(samples * 4));
    }
    s_total_samples += (unsigned long)samples;
    if (s_total_samples % 44100 < (unsigned long)samples) {
        printf("[*** AirPlay ***] Streaming: %lu frames (%.1f s)\n",
               s_total_samples, (double)s_total_samples / 44100.0);
    }
    update_playback_light(1);
}

static void my_output_stop(void) {
    printf("[*** AirPlay ***] Audio Stream STOPPED (total frames = %lu)\n", s_total_samples);
    s_airplay_active = 0;
    if (s_pcm) {
        pcm_close(s_pcm);
        s_pcm = NULL;
    }
    unlink("/data/local/tmp/airplay_playing");
    set_amps(0);
}

static void my_output_volume(double vol) {
    printf("[*** AirPlay ***] Volume command: %f dB\n", vol);
    if (vol < -30.0) vol = -30.0;
    if (vol > 0.0) vol = 0.0;
    s_vol = (int)((vol + 30.0) / 30.0 * 231.0);
    if (s_vol < 0) s_vol = 0;
    if (s_vol > 231) s_vol = 231;

    if (s_mixer) {
        struct mixer_ctl *vl = mixer_get_ctl(s_mixer, 10);
        if (vl) mixer_ctl_set_value(vl, 0, s_vol);
        struct mixer_ctl *vr = mixer_get_ctl(s_mixer, 11);
        if (vr) mixer_ctl_set_value(vr, 0, s_vol);
    }
}

static audio_output_t s_my_audio_output = {
    .help = my_output_help,
    .name = "r1_audio",
    .init = my_output_init,
    .deinit = my_output_deinit,
    .start = my_output_start,
    .play = my_output_play,
    .stop = my_output_stop,
    .volume = my_output_volume,
};

/* --- Dynamic Network Interface Info --- */
static char s_mac_str[16] = "FC7C02C7A931";
static char s_mac_colons[24] = "FC:7C:02:C7:A9:31";
static uint32_t s_ip_be = 0;

static int resolve_wlan0(void) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "wlan0", sizeof(ifr.ifr_name) - 1);

    if (ioctl(s, SIOCGIFADDR, &ifr) == 0) {
        s_ip_be = ifr.ifr_addr.sin_addr.s_addr;
    }

    if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
        unsigned char *m = (unsigned char *)ifr.ifr_hwaddr.sa_data;
        snprintf(s_mac_str, sizeof(s_mac_str), "%02X%02X%02X%02X%02X%02X",
                 m[0], m[1], m[2], m[3], m[4], m[5]);
        snprintf(s_mac_colons, sizeof(s_mac_colons), "%02X:%02X:%02X:%02X:%02X:%02X",
                 m[0], m[1], m[2], m[3], m[4], m[5]);
    }
    close(s);
    return (s_ip_be != 0) ? 0 : -1;
}

/* --- mDNS Broadcaster --- */
static unsigned short htons(unsigned short x) {
    return (unsigned short)(((x & 0xFF) << 8) | ((x >> 8) & 0xFF));
}

static unsigned int htonl(unsigned int x) {
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
}

static void append_dns_name(uint8_t *dst, int *offset, const char *name) {
    const char *p = name;
    while (*p) {
        const char *next = p;
        while (*next && *next != '.') next++;
        int len = next - p;
        dst[(*offset)++] = (uint8_t)len;
        memcpy(&dst[*offset], p, (size_t)len);
        *offset += len;
        if (*next == '.') next++;
        p = next;
    }
    dst[(*offset)++] = 0x00;
}

static void append_txt_item(uint8_t *dst, int *offset, const char *str) {
    int len = (int)strlen(str);
    dst[(*offset)++] = (uint8_t)len;
    memcpy(&dst[*offset], str, (size_t)len);
    *offset += len;
}

static int build_mdns_response(uint8_t *buf, const char *mac, const char *mac_colons,
                               const char *hostname, uint32_t ip_be) {
    int off = 0;

    // Header: Answers=7 (RAOP PTR/SRV/TXT, AirPlay PTR/SRV/TXT, A)
    uint16_t hdr[6] = { 0, htons(0x8400), 0, htons(7), 0, 0 };
    memcpy(&buf[off], hdr, 12);
    off += 12;

    char raop_srv[128];
    snprintf(raop_srv, sizeof(raop_srv), "%s@%s._raop._tcp.local", mac, hostname);

    char airplay_srv[128];
    snprintf(airplay_srv, sizeof(airplay_srv), "%s._airplay._tcp.local", hostname);

    char host_local[64];
    snprintf(host_local, sizeof(host_local), "%s.local", hostname);

    uint8_t ptr_type_class[8] = { 0x00, 12, 0x80, 0x01, 0x00, 0x00, 0x11, 0x94 }; // PTR, TTL=4500
    uint8_t srv_type_class[8] = { 0x00, 33, 0x80, 0x01, 0x00, 0x00, 0x00, 120 };    // SRV, TTL=120
    uint8_t txt_type_class[8] = { 0x00, 16, 0x80, 0x01, 0x00, 0x00, 0x11, 0x94 }; // TXT, TTL=4500

    // 1. PTR Record: _raop._tcp.local -> raop_srv
    append_dns_name(buf, &off, "_raop._tcp.local");
    memcpy(&buf[off], ptr_type_class, 8);
    off += 8;
    int rdata_len_pos = off;
    off += 2;
    int rdata_start = off;
    append_dns_name(buf, &off, raop_srv);
    uint16_t ptr_len = htons((uint16_t)(off - rdata_start));
    memcpy(&buf[rdata_len_pos], &ptr_len, 2);

    // 2. SRV Record: raop_srv -> port 5000, target host_local
    append_dns_name(buf, &off, raop_srv);
    memcpy(&buf[off], srv_type_class, 8);
    off += 8;
    rdata_len_pos = off;
    off += 2;
    rdata_start = off;
    uint16_t srv_prio_weight_port[3] = { 0, 0, htons(5000) };
    memcpy(&buf[off], srv_prio_weight_port, 6);
    off += 6;
    append_dns_name(buf, &off, host_local);
    uint16_t srv_len = htons((uint16_t)(off - rdata_start));
    memcpy(&buf[rdata_len_pos], &srv_len, 2);

    // 3. TXT Record: raop_srv -> Lossless ALAC audio attributes
    append_dns_name(buf, &off, raop_srv);
    memcpy(&buf[off], txt_type_class, 8);
    off += 8;
    rdata_len_pos = off;
    off += 2;
    rdata_start = off;
    append_txt_item(buf, &off, "txtvers=1");
    append_txt_item(buf, &off, "ch=2");
    append_txt_item(buf, &off, "cn=0,1");
    append_txt_item(buf, &off, "et=0,1");
    append_txt_item(buf, &off, "ek=1");
    append_txt_item(buf, &off, "sv=false");
    append_txt_item(buf, &off, "da=true");
    append_txt_item(buf, &off, "sr=44100");
    append_txt_item(buf, &off, "ss=16");
    append_txt_item(buf, &off, "vn=65537");
    append_txt_item(buf, &off, "tp=TCP,UDP");
    append_txt_item(buf, &off, "md=0,1,2");
    append_txt_item(buf, &off, "pw=false");
    append_txt_item(buf, &off, "vs=105.1");
    append_txt_item(buf, &off, "am=Phicomm,R1");
    append_txt_item(buf, &off, "fv=4.3.7");
    append_txt_item(buf, &off, "sf=0x4");
    uint16_t txt_len = htons((uint16_t)(off - rdata_start));
    memcpy(&buf[rdata_len_pos], &txt_len, 2);

    // 4. PTR Record: _airplay._tcp.local -> airplay_srv
    append_dns_name(buf, &off, "_airplay._tcp.local");
    memcpy(&buf[off], ptr_type_class, 8);
    off += 8;
    rdata_len_pos = off;
    off += 2;
    rdata_start = off;
    append_dns_name(buf, &off, airplay_srv);
    ptr_len = htons((uint16_t)(off - rdata_start));
    memcpy(&buf[rdata_len_pos], &ptr_len, 2);

    // 5. SRV Record: airplay_srv -> port 5000, target host_local
    append_dns_name(buf, &off, airplay_srv);
    memcpy(&buf[off], srv_type_class, 8);
    off += 8;
    rdata_len_pos = off;
    off += 2;
    rdata_start = off;
    memcpy(&buf[off], srv_prio_weight_port, 6);
    off += 6;
    append_dns_name(buf, &off, host_local);
    srv_len = htons((uint16_t)(off - rdata_start));
    memcpy(&buf[rdata_len_pos], &srv_len, 2);

    // 6. TXT Record: airplay_srv -> Modern AirPlay attributes
    append_dns_name(buf, &off, airplay_srv);
    memcpy(&buf[off], txt_type_class, 8);
    off += 8;
    rdata_len_pos = off;
    off += 2;
    rdata_start = off;
    char devid_txt[32];
    snprintf(devid_txt, sizeof(devid_txt), "deviceid=%s", mac_colons);
    append_txt_item(buf, &off, devid_txt);
    append_txt_item(buf, &off, "features=0x5A7FFFF7,0x1E");
    append_txt_item(buf, &off, "flags=0x4");
    append_txt_item(buf, &off, "model=Phicomm,R1");
    append_txt_item(buf, &off, "srcvers=220.68");
    append_txt_item(buf, &off, "vv=2");
    txt_len = htons((uint16_t)(off - rdata_start));
    memcpy(&buf[rdata_len_pos], &txt_len, 2);

    // 7. A Record: host_local -> IP address
    append_dns_name(buf, &off, host_local);
    uint8_t a_type_class_ttl_len[10] = {
        0x00, 1, 0x80, 0x01, 0x00, 0x00, 0x00, 120, 0x00, 4
    };
    memcpy(&buf[off], a_type_class_ttl_len, 10);
    off += 10;
    memcpy(&buf[off], &ip_be, 4);
    off += 4;

    return off;
}

static void *mdns_responder_thread(void *arg) {
    (void)arg;

    // Loop until wlan0 has a valid IPv4 address
    while (s_ip_be == 0) {
        resolve_wlan0();
        if (s_ip_be == 0) sleep(1);
    }
    printf("[+] wlan0 online: MAC=%s IP=0x%08x\n", s_mac_str, s_ip_be);

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) return NULL;

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ttl = 255;
    setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(5353);
    srv_addr.sin_addr.s_addr = htonl(0x00000000);

    if (bind(fd, (void *)&srv_addr, sizeof(srv_addr)) < 0) {
        close(fd);
        return NULL;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = htonl(0xe00000fb);
    mreq.imr_interface.s_addr = htonl(0x00000000);
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    struct sockaddr_in dst_addr;
    memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(5353);
    dst_addr.sin_addr.s_addr = htonl(0xe00000fb);

    uint8_t packet[1024];
    int pkt_len = build_mdns_response(packet, s_mac_str, s_mac_colons, "Phicomm-R1", s_ip_be);

    // Initial announcement (3 bursts)
    for (int i = 0; i < 3; i++) {
        sendto(fd, packet, (size_t)pkt_len, 0, (void *)&dst_addr, sizeof(dst_addr));
        usleep(150000);
    }
    printf("[+] mDNS service advertised: Phicomm-R1 (%s)\n", s_mac_str);

    uint8_t rx_buf[1500];
    int counter = 0;
    while (1) {
        struct sockaddr_in from;
        unsigned int from_len = sizeof(from);
        long n = recvfrom(fd, rx_buf, sizeof(rx_buf), 0, (void *)&from, &from_len);
        if (n > 12) {
            uint16_t flags = (uint16_t)((rx_buf[2] << 8) | rx_buf[3]);
            if ((flags & 0x8000) == 0) { // DNS Query
                sendto(fd, packet, (size_t)pkt_len, 0, (void *)&dst_addr, sizeof(dst_addr));
            }
        }
        counter++;
        if (counter >= 120) {
            sendto(fd, packet, (size_t)pkt_len, 0, (void *)&dst_addr, sizeof(dst_addr));
            counter = 0;
        }
    }

    close(fd);
    return NULL;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("========================================================\n");
    printf("    Phicomm R1 AirPlay Full Lossless Audio Receiver    \n");
    printf("========================================================\n");

    // Write PID file for supervisor
    int pfd = open("/data/local/tmp/airplay.pid", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (pfd >= 0) {
        char pbuf[16];
        snprintf(pbuf, sizeof(pbuf), "%d\n", getpid());
        write(pfd, pbuf, strlen(pbuf));
        close(pfd);
    }

    // Wait for network interface
    for (int i = 0; i < 30; i++) {
        if (resolve_wlan0() == 0) break;
        sleep(1);
    }

    // Hook custom audio output into Shairport
    void **audio_outputs = (void **)((char *)&audio_dummy - 16);
    audio_outputs[0] = &s_my_audio_output;

    set_airplay_hostname("Phicomm-R1");
    set_airplay_hwAddr(s_mac_str);
    set_airplay_port("5000");

    unsigned long tid = 0;
    pthread_create(&tid, NULL, mdns_responder_thread, NULL);

    unsigned long touch_tid = 0;
    pthread_create(&touch_tid, NULL, touch_listener_thread, NULL);

    int ret = start_airplay();
    printf("[+] start_airplay() listening on 0.0.0.0:5000 (ret=%d)\n", ret);

    while (1) {
        sleep(10);
    }

    exit(0);
}
