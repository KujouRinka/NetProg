#include <iostream>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <csignal>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "list_timer.h"

using namespace std;

constexpr uint32_t FD_LIMIT = 0xffff;
constexpr int MAX_EVENT_NUMBER = 1024;
constexpr int TIMESLOT = 5;

static int pipe_fd[2];
static SortTimeList timer_list;
static int epoll_fd = 0;

int setNonBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addFd(int epoll_fd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

void sigHandler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipe_fd[1], &msg, 1, 0);
    errno = save_errno;
}

void addSig(int sig) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigHandler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    if (sigaction(sig, &sa, nullptr) == -1) {
        cout << "register signal error" << endl;
        exit(0);
    }
}

void timer_handler() {
    timer_list.tick();
    alarm(TIMESLOT);
}

void cbFunc(struct ClientData *user_data) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, user_data->sock_fd, nullptr);
    if (user_data == nullptr) {
        cout << "user data null" << endl;
        exit(0);
    }
    close(user_data->sock_fd);
    cout << "close fd " << user_data->sock_fd << endl;
}

int main() {
    int ret = 0;

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listen_fd != -1);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8889);

    ret = bind(listen_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address));
    assert(ret != -1);

    ret = listen(listen_fd, 5);
    assert(ret != -1);

    struct epoll_event events[MAX_EVENT_NUMBER];
    epoll_fd = epoll_create(5);
    assert(epoll_fd != -1);
    addFd(epoll_fd, listen_fd);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipe_fd);
    assert(ret != -1);
    setNonBlocking(pipe_fd[1]);
    addFd(epoll_fd, pipe_fd[0]);

    addSig(SIGALRM);
    addSig(SIGTERM);
    addSig(SIGINT);
    bool stop_server = false;

    struct ClientData *users = new ClientData[FD_LIMIT];
    bool timeout = false;
    alarm(TIMESLOT);

    while (!stop_server) {
        int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            cout << "epoll fail" << endl;
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sock_fd = events[i].data.fd;
            if (sock_fd == listen_fd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int conn_fd = accept(listen_fd, reinterpret_cast<struct sockaddr *>(&client_address),
                                     &client_addrlength);
                addFd(epoll_fd, conn_fd);
                users[conn_fd].address = client_address;
                users[conn_fd].sock_fd = conn_fd;

                struct UtilTimer *timer = new UtilTimer;
                timer->user_data = &users[conn_fd];
                timer->cbFunc = cbFunc;
                time_t cur = time(nullptr);
                timer->expire = cur + 3 * TIMESLOT;
                users[conn_fd].timer = timer;
                timer_list.addTimer(timer);
            } else if ((sock_fd == pipe_fd[0]) && (events[i].events & EPOLLIN)) {
                int sig;
                char signals[1024];
                ret = recv(pipe_fd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                            case SIGALRM:
                                timeout = true;
                                break;
                            case SIGTERM:
                            case SIGINT:
                                stop_server = true;
                                break;
                            default:
                                cout << "unsupported signal" << endl;
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                memset(users[sock_fd].buf, 0, BUFFER_SIZE);
                ret = recv(sock_fd, users[sock_fd].buf, BUFFER_SIZE - 1, 0);
                cout << "get " << ret << " bytes of client data " << users[sock_fd].buf << " from " << sock_fd << endl;
                struct UtilTimer *timer = users[sock_fd].timer;
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        cbFunc(&users[sock_fd]);
                        if (timer)
                            timer_list.deleteTimer(timer);
                    }
                } else if (ret == 0) {
                    cbFunc(&users[sock_fd]);
                    if (timer)
                        timer_list.deleteTimer(timer);
                } else {
                    if (timer) {
                        time_t cur = time(nullptr);
                        timer->expire = cur + 3 * TIMESLOT;
                        cout << "adjust timer once" << endl;
                        timer_list.adjustTimer(timer);
                    }
                }
            } else {
                cout << "something else happened" << endl;
            }

            if (timeout) {
                timer_handler();
                timeout = false;
            }
        }
    }

    close(listen_fd);
    close(pipe_fd[1]);
    close(pipe_fd[0]);
    delete[]users;
    cout << "close server safely" << endl;

    return 0;
}
