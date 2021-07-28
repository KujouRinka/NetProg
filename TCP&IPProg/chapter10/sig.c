#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

void timeout(int sig) {
    if (sig == SIGALRM)
        puts("time out");
    alarm(2);
}

void keycontrol(int sig) {
    if (sig == SIGINT)
        puts("ctrl+C pressed");
}

int main(int argc, char **argv) {
    signal(SIGALRM, timeout);
    signal(SIGINT, keycontrol);
    alarm(2);

    for (int i = 0; i < 3; ++i) {
        puts("wait...");
        sleep(100);
    }

    return 0;
}
