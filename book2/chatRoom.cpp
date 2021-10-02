#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <cerrno>

using namespace std;

#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 0xffff

struct clientData {
    struct sockaddr_in address;
    char *writeBuf;
    char buf[BUFFER_SIZE];
};

int setNonBlock(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

int main() {
    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8888);

    int listenFd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        cout << "socket error" << endl;
        exit(0);
    }

    ret = bind(listenFd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address));
    if (ret == -1) {
        cout << "bind error" << endl;
        close(listenFd);
        exit(0);
    }

    ret = listen(listenFd, 5);
    if (ret == -1) {
        cout << "listen error" << endl;
        close(listenFd);
        exit(0);
    }

    clientData *users = new clientData[FD_LIMIT];
    pollfd fds[USER_LIMIT + 1];
    int user_counter = 0;

    for (int i = 1; i <= USER_LIMIT; ++i) {
        fds[i].fd = -1;
        fds[i].events = 0;
    }

    fds[0].fd = listenFd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while (true) {
        ret = poll(fds, user_counter + 1, -1);
        if (ret < 0) {
            cout << "poll failure" << endl;
            break;
        }

        for (int i = 0; i < user_counter + 1; ++i) {
            if ((fds[i].fd == listenFd) && (fds[i].revents & POLLIN)) {
                struct sockaddr_in clientAddress;
                memset(&clientAddress, 0, sizeof(clientAddress));
                socklen_t clientAddrLength = sizeof(clientAddress);
                int connFd = accept(listenFd, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddrLength);
                if (connFd == -1) {
                    cout << "errno is: " << endl;
                    continue;
                }
                if (user_counter >= USER_LIMIT) {
                    const char *info = "too many users";
                    cout << info << endl;
                    send(connFd, info, strlen(info), 0);
                    close(connFd);
                    continue;
                }
                ++user_counter;
                users[connFd].address = clientAddress;
                setNonBlock(connFd);
                fds[user_counter].fd = connFd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                cout << "comes a new users, now have " << user_counter << " users" << endl;
            } else if (fds[i].revents & POLLERR) {
                cout << "get an error from " << fds[i].fd << endl;
                char errors[100] = {0};
                socklen_t length = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0) {
                    cout << "get socket option failed" << endl;
                }
                continue;
            } else if (fds[i].revents & POLLRDHUP) {
                // users[fds[i].fd] = users[fds[user_counter].fd];
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                --i;
                --user_counter;
                cout << "a client left" << endl;
            } else if (fds[i].revents & POLLIN) {
                int connFd = fds[i].fd;
                memset(users[connFd].buf, 0, BUFFER_SIZE);
                ret = recv(connFd, users[connFd].buf, BUFFER_SIZE - 1, 0);
                cout << "get " << ret << " bytes of client data " << users[connFd].buf << " from " << connFd << endl;
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        close(connFd);
                        // users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        --i;
                        --user_counter;
                    }
                } else if (ret == 0) {

                } else {
                    for (int j = 1; j <= user_counter; ++j) {
                        if (fds[j].fd == connFd)
                            continue;
                        fds[j].events &= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].writeBuf = users[connFd].buf;
                    }
                }
            } else if (fds[i].revents & POLLOUT) {
                int connFd = fds[i].fd;
                if (!users[connFd].writeBuf)
                    continue;
                ret = send(connFd, users[connFd].writeBuf, strlen(users[connFd].writeBuf), 0);
                users[connFd].writeBuf = nullptr;
                fds[i].events &= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete[]users;
    close(listenFd);
    return 0;
}
