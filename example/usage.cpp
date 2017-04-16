#include <iostream>
#include <list>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <string>
#include <fstream>
#include "tcomm_mq.h"

#define DATASCALE 10000000

static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

struct dataform
{
    std::string context;
    unsigned sendtime;
    unsigned recvtime;

    dataform(bool create = true): sendtime(0), recvtime(0)
    {
        if (create)
        {
            int size = ::rand() % 50;
            context.resize(size);
            for (int i = 0;i < context.size(); ++i)
                context[i] = charset[::rand() % charset.length()];
        }
    }

    bool encode(char* buffer, unsigned buffer_size, unsigned& data_size)
    {
        //sendtime|recvtime|context_len|context
        if (context.size() + sizeof(unsigned) * 3 > buffer_size)
            return false;
        data_size = context.size() + sizeof(unsigned) * 3;
        *(unsigned *)buffer = sendtime;
        *((unsigned *)buffer + 1) = recvtime;
        *((unsigned *)buffer + 2) = context.size();
        buffer += sizeof(unsigned) * 3;
        memcpy(buffer, context.c_str(), context.size());
        return true;
    }

    bool decode(char* buffer, unsigned data_size)
    {
        //sendtime|recvtime|context_len|context
        if (data_size < sizeof(unsigned) * 3)
            return false;
        data_size = context.size() + sizeof(unsigned) * 3;
        sendtime = *(unsigned *)buffer;
        recvtime = *((unsigned *)buffer + 1);
        unsigned str_size = *((unsigned *)buffer + 2);
        buffer += sizeof(unsigned) * 3;
        if (data_size != str_size + sizeof(unsigned) * 3)
            return false;
        context.assign(buffer, buffer + str_size);
        return true;
    }
};

long int getCurrentTimeInMillis()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}

TCommMQ tcommu;

void* thd_do(void* args)
{
    int efd = epoll_create1(0);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = tcommu.notifier();
    epoll_ctl(efd, EPOLL_CTL_ADD, tcommu.notifier(), &event);

    std::list<std::pair<std::string, unsigned long> > datapool;
    #define BUFFSIZE 3072
    char readbuffer[BUFFSIZE];
    unsigned data_len;
    unsigned long first_recv_time;

    unsigned recv_cnt = 0;

    struct epoll_event revent;
    while (true)
    {
        if (epoll_wait(efd, &revent, 1, 10) > 0)
        {
            while (tcommu.consume(readbuffer, BUFFSIZE, data_len) == QUEUE_SUCC)
            {
                if (datapool.size() == 0)
                    first_recv_time = getCurrentTimeInMillis();

                std::string recvdata(readbuffer, readbuffer + data_len);
                std::pair<std::string, unsigned long> mypair(recvdata, getCurrentTimeInMillis());
                datapool.push_back(mypair);
                ++recv_cnt;
                if (DATASCALE == datapool.size())
                    break;
            }
            if (DATASCALE == datapool.size())
                break; 
        }
    }
    //persist data to test data content correctness
    unsigned long last_recv_time = getCurrentTimeInMillis();
    std::ofstream outfile;
    outfile.open("rect.txt", std::ofstream::out);
    for (std::list<std::pair<std::string, unsigned long> >::iterator it = datapool.begin();
        it != datapool.end(); ++it)
        outfile << it->first << "+" << it->second << "\n";

    std::cout << "recv in " << recv_cnt << " data in " << last_recv_time - first_recv_time <<" ms\n";
    outfile.close();
    return NULL;
}

int main()
{
    ::srand(::time(NULL));
    std::list<std::pair<std::string, unsigned long> > datapool;
    pthread_t tid;
    pthread_create(&tid, NULL, thd_do, NULL);
    pthread_detach(tid);

    for (int i = 0;i < DATASCALE; ++i)
    {
        std::pair<std::string, unsigned long> mypair("leechanx hello !", 0);
        datapool.push_back(mypair);
    }

    unsigned send_cnt = 0;

    unsigned long first_snd_time = getCurrentTimeInMillis();

    for (std::list<std::pair<std::string, unsigned long> >::iterator it = datapool.begin();
        it != datapool.end(); ++it)
    {
        it->second = getCurrentTimeInMillis();
        if (tcommu.produce(it->first.c_str(), it->first.size()) == QUEUE_SUCC)
            ++send_cnt;
    }
    unsigned long last_snd_time = getCurrentTimeInMillis();

    std::cout << "send out " << send_cnt << " data in " << last_snd_time - first_snd_time <<" ms\n";
    std::ofstream outfile;
    outfile.open("sendt.txt", std::ofstream::out);
    for (std::list<std::pair<std::string, unsigned long> >::iterator it = datapool.begin();
        it != datapool.end(); ++it)
        outfile << it->first << "+" << it->second << "\n";
    outfile.close();
    sleep(10000);
}
