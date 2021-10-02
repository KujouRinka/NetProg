#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

struct fds {
    int epollFd;
    int sockFd;
};

int setNonBlocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

void addFd(int epollFd, int fd, bool oneshot) {
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd;
    if (oneshot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

void resetOneShot(int epollFd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
}

void *worker(void *arg) {
    int sockFd = reinterpret_cast<fds *>(arg)->sockFd;
    int epollFd = reinterpret_cast<fds *>(arg)->epollFd;
    cout << "start new thread to receive data on fd: " << sockFd << endl;
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);
    while (true) {
        int ret = recv(sockFd, buf, BUFFER_SIZE, 0);
        if (ret == 0) {
            close(sockFd);
            cout << "forever closed the connection" << endl;
            break;
        } else if (ret == -1) {
            if (errno == EAGAIN) {
                resetOneShot(epollFd, sockFd);
                cout << "read later" << endl;
                break;
            }
        } else {
            cout << "get content: " << buf << endl;
            sleep(5);
        }
    }
    cout << "end thread receiving data on fd: " << sockFd << endl;
    return nullptr;
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
    addFd(epollFd, listenFd, false);
    while (true) {
        int ret = epoll_wait(epollFd, events, MAX_EVENT_NUMBER, -1);
        if (ret == -1) {
            cout << "epoll_wait error: " << strerror(errno) << endl;
            break;
        }
        for (int i = 0; i < ret; ++i) {
            int sockFd = events[i].data.fd;
            if (sockFd == listenFd) {
                struct sockaddr_in clientAddress;
                socklen_t clientAddrLength = sizeof(clientAddress);
                int connFd = accept(listenFd, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddrLength);
                if (connFd == -1) {
                    cout << "accept error: " << strerror(errno) << endl;
                    continue;
                }
                addFd(epollFd, connFd, true);
            } else if (events[i].events & EPOLLIN) {
                pthread_t thread;
                fds fdsForNewWorker;
                fdsForNewWorker.epollFd = epollFd;
                fdsForNewWorker.sockFd = sockFd;
                pthread_create(&thread, nullptr, worker, reinterpret_cast<void *>(&fdsForNewWorker));
            } else {
                cout << "something else happened" << endl;
            }
        }
    }

    close(listenFd);
    return 0;
}
