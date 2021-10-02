#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

int setNonBlocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

int unblockConnect(const char *ip, int port, int time) {
    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    int sockFd = socket(PF_INET, SOCK_STREAM, 0);
    int fdOpt = setNonBlocking(sockFd);
    ret = connect(sockFd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address));
    if (ret == 0) {
        cout << "connect with server immediately\n" << endl;
        fcntl(sockFd, F_SETFL, fdOpt);
        return sockFd;
    } else if (errno != EINPROGRESS) {
        cout << "unblock connect not support" << endl;
        return -1;
    }
    fd_set writeFds;
    struct timeval timeout;

    FD_SET(sockFd, &writeFds);
    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(sockFd + 1, nullptr, &writeFds, nullptr, &timeout);
    if (ret <= 0) {
        cout << "connection time out" << endl;
        close(sockFd);
        return -1;
    }
    if (!FD_ISSET(sockFd, &writeFds)) {
        cout << "no events on sockFd found" << endl;
        close(sockFd);
        return -1;
    }
    int error = 0;
    socklen_t length = sizeof(error);
    if (getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        cout << "get socket option failed" << endl;
        close(sockFd);
        return -1;
    }
    if (error != 0) {
        cout << "connection failed after select with the error: " << strerror(error) << endl;
        close(sockFd);
        return -1;
    }
    cout << "connection ready after select with the socket: " << sockFd << endl;
    fcntl(sockFd, F_SETFL, fdOpt);
    return sockFd;
}

int main() {
    int sockFd = unblockConnect("127.0.0.1", 8888, 10);
    if (sockFd < 0) {
        cout << "bad socket" << endl;
        return 0;
    }
    close(sockFd);
    return 0;
}
