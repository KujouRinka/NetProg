#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>

using namespace std;

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

int setNonBlocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

void addFd(int epollFd, int fd, bool enableET) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    if (enableET)
        event.events |= EPOLLET;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

void LT(struct epoll_event *events, int number, int epollFd, int listenFd) {
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i) {
        int sockFd = events[i].data.fd;
        if (sockFd == listenFd) {
            struct sockaddr_in clientAddress;
            socklen_t clientAddrLength = sizeof(clientAddress);
            int connFd = accept(listenFd, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddrLength);
            addFd(epollFd, connFd, false);
        } else if (events[i].events & EPOLLIN) {
            cout << "event trigger once" << endl;
            memset(buf, 0, BUFFER_SIZE);
            int ret = recv(sockFd, buf, BUFFER_SIZE - 1, 0);
            if (ret == -1) {
                close(sockFd);
                continue;
            }
            cout << "get " << ret << " bytes of content: " << buf << endl;
        } else {
            cout << "something else happened" << endl;
        }
    }
}

void ET(struct epoll_event *events, int number, int epollFd, int listenFd) {
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i) {
        int sockFd = events[i].data.fd;
        if (sockFd == listenFd) {
            struct sockaddr_in clientAddress;
            socklen_t clientAddrLength = sizeof(clientAddress);
            int connFd = accept(listenFd, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddrLength);
            addFd(epollFd, connFd, true);
        } else if (events[i].events & EPOLLIN) {
            cout << "trigger once" << endl;
            while (true) {
                memset(buf, 0, BUFFER_SIZE);
                int ret = recv(sockFd, buf, BUFFER_SIZE - 1, 0);
                if (ret == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        cout << "read later" << endl;
                        break;
                    }
                    close(sockFd);
                    break;
                } else if (ret == 0) {
                    close(sockFd);
                } else {
                    cout << "get " << ret << " bytes of content: " << buf << endl;
                }
            }
        } else {
            cout << "something else happened" << endl;
        }
    }
}

int main() {
    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(atoi("8888"));

    int listenFd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        cout << "socket error: " << strerror(errno) << endl;
        exit(0);
    }

    ret = bind(listenFd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address));
    if (ret == -1) {
        cout << "bind error: " << strerror(errno) << endl;
        exit(0);
    }

    ret = listen(listenFd, 5);
    if (ret == -1) {
        cout << "listen error: " << strerror(errno) << endl;
        exit(0);
    }

    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollFd = epoll_create(5);
    if (epollFd == -1) {
        cout << "epoll_create error: " << strerror(errno) << endl;
        exit(0);
    }

    addFd(epollFd, listenFd, true);
    while (true) {
        ret = epoll_wait(epollFd, events, MAX_EVENT_NUMBER, -1);
        if (ret == -1) {
            cout << "epoll_wait error: " << strerror(errno) << endl;
            break;
        }
        LT(events, ret, epollFd, listenFd);
        // ET(events, ret, epollFd, listenFd);
    }

    close(listenFd);
    return 0;
}
