#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char **argv) {
    if (argc != 3)
        exit(1);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));

    int read_cnt;
    char buffer[BUF_SIZE];
    FILE *fp = fopen("pic.png", "wb");
    while (read_cnt = read(sock, buffer, BUF_SIZE))
        fwrite(buffer, 1, read_cnt, fp);

    puts("Received file data");
    write(sock, "Thx", 4);
    fclose(fp);
    close(sock);

    return 0;
}
