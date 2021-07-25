#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char **argv) {
    if (argc != 2)
        exit(1);

    int server_sock, client_sock;
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));
    bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr));

    listen(server_sock, 5);

    socklen_t client_addr_len = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_addr_len);

    char buffer[BUF_SIZE];
    FILE *fp = fopen("Astesia.png", "rb");
    int read_cnt;

    while (1) {
        read_cnt = fread(buffer, 1, BUF_SIZE, fp);
        write(client_sock, buffer, read_cnt);
        if (read_cnt < BUF_SIZE)
            break;
    }
    shutdown(client_sock, SHUT_WR);
    read(client_sock, buffer, BUF_SIZE);
    printf("Message: %s\n", buffer);

    fclose(fp);
    close(client_sock);
    close(server_sock);

    return 0;
}
