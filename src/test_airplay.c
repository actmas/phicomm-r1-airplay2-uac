#include <stddef.h>

extern int set_airplay_hostname(const char *name);
extern int set_airplay_hwAddr(const char *hwaddr);
extern int set_airplay_port(const char *port);
extern int start_airplay(void);
extern int stop_airplay(void);
extern int mdns_register(void);

extern void *audio_dummy;
extern void *audio_pipe;
extern void *audio_androidtrack;

int printf(const char *fmt, ...);
int sleep(unsigned int sec);
void exit(int status);

int main(int argc, char **argv) {
    printf("[*] audio_dummy at: %p\n", &audio_dummy);

    void **audio_outputs = (void **)((char *)&audio_dummy - 16);
    audio_outputs[0] = &audio_dummy;
    printf("[*] audio_outputs[0] redirected to dummy: %p\n", audio_outputs[0]);

    printf("[*] Setting hostname...\n");
    set_airplay_hostname("Phicomm-R1");

    printf("[*] Setting hwAddr...\n");
    set_airplay_hwAddr("FC7C02C7A931");

    printf("[*] Setting port...\n");
    set_airplay_port("5000");

    printf("[*] Calling start_airplay()...\n");
    int ret = start_airplay();
    printf("[*] start_airplay returned: %d\n", ret);

    printf("[*] Skipping mdns_register to avoid buggy getifaddrs...\n");
    // int mret = mdns_register();
    // printf("[*] mdns_register returned: %d\n", mret);

    printf("[*] Sleeping for 60 seconds to observe...\n");
    sleep(60);

    exit(0);
}
