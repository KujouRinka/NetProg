#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
void error_handling(char *msg);

int main(int argc, char **argv) {
    int sock;
    struct sockaddr recv_adr;
    struct sockaddr_in *recv_ptr = (struct sockaddr_in *) &recv_adr;
    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(recv_ptr, 0, sizeof(recv_adr));
    recv_ptr->sin_family = AF_INET;
    recv_ptr->sin_addr.s_addr = inet_addr(argv[1]);
    recv_ptr->sin_port = htons(atoi(argv[2]));

    if (connect(sock, &recv_adr, sizeof(recv_adr)) == -1)
        error_handling("connect() error!");
    printf("writing....\n");
    write(sock, "123", strlen("123"));
    send(sock, "4", strlen("4"), MSG_OOB);
    write(sock, "567", strlen("567"));
    send(sock, "890", strlen("890"), MSG_OOB);
    close(sock);
    return 0;
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
