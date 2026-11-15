int system(const char *cmd);
int printf(const char *fmt, ...);

int main(void) {
    int ret = system("/system/bin/sh /data/local/tmp/test_exec.sh");
    printf("[*] system() returned: %d\n", ret);
    return 0;
}
