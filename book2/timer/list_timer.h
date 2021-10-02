//
// Created by kujou on 10/2/2021.
//

#ifndef CSERVER_LIST_TIMER_H
#define CSERVER_LIST_TIMER_H

#include <iostream>
#include <list>
#include <algorithm>
#include <ctime>
#include <arpa/inet.h>

#define BUFFER_SIZE 64

class UtilTimer;

struct ClientData {
    struct sockaddr_in address;
    int sock_fd;
    char buf[BUFFER_SIZE];
    UtilTimer *timer;
};

class UtilTimer {
public:
    time_t expire;
    void (*cbFunc)(struct ClientData *);
    struct ClientData *user_data;
    bool operator<(const UtilTimer rhs) const {
        return expire < rhs.expire;
    }
};

class SortTimeList {
public:
    SortTimeList() : timer_list() {}

    void addTimer(UtilTimer *timer) {
        time_t this_time = timer->expire;
        auto it = std::find_if(timer_list.begin(), timer_list.end(),
                               [this_time](const UtilTimer *ut) {
                                   return ut->expire > this_time;
                               });
        timer_list.insert(it, timer);
    }

    void adjustTimer(UtilTimer *timer) {
        auto it = std::find(timer_list.begin(), timer_list.end(), timer);
        adjustTimer(it);
    }

    void adjustTimer(std::list<UtilTimer *>::iterator it) {
        if (*it == nullptr)
            return;
        auto this_it = it;
        auto tmp_it = ++it;
        if (tmp_it == timer_list.end() || ((*this_it)->expire < (*tmp_it)->expire))
            return;
        UtilTimer *tp = *this_it;
        timer_list.erase(this_it);
        addTimer(tp);
    }

    void deleteTimer(UtilTimer *timer) {
        auto it = std::find(timer_list.begin(), timer_list.end(), timer);
        deleteTimer(it);
    }

    void deleteTimer(std::list<UtilTimer *>::iterator it) {
        if (*it == nullptr)
            return;
        delete *it;
        timer_list.erase(it);
    }

    void tick() {
        if (timer_list.empty())
            return;
        std::cout << "time tick" << std::endl;
        time_t cur = time(nullptr);
        for (auto it = timer_list.begin(); it != timer_list.end(); ++it) {
            if (cur < (*it)->expire)
                break;
            (*it)->cbFunc((*it)->user_data);
            delete (*it);
            timer_list.erase(it);
        }
    }

    ~SortTimeList() {
        for (auto p: timer_list)
            delete p;
    }
private:
    std::list<UtilTimer *> timer_list;
};

#endif //CSERVER_LIST_TIMER_H
