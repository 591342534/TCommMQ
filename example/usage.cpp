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

#define DATASCALE 10000//00

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

    std::list<dataform> datapool;
    #define BUFFSIZE 3072
    char readbuffer[BUFFSIZE];
    unsigned data_len;

    struct epoll_event revent;
    while (true)
    {
        if (epoll_wait(efd, &revent, 1, 10) > 0)
        {
            while (tcommu.consume(readbuffer, BUFFSIZE, data_len) == QUEUE_SUCC)
            {
                dataform data;
                data.decode(readbuffer, data_len);
                data.recvtime = getCurrentTimeInMillis();
                datapool.push_back(data);
//                std::cout << datapool.size() << " current size\n";
                if (DATASCALE == datapool.size())
                    break;
            }
            if (DATASCALE == datapool.size())
                break; 
        }
    }
    //persist data to test data content correctness
    unsigned long total_time = 0;
    std::ofstream outfile;
    outfile.open("rect.txt", std::ofstream::out);
    for (std::list<dataform>::iterator it = datapool.begin();
        it != datapool.end(); ++it)
    {
        total_time += it->recvtime - it->sendtime;
        outfile << it->context << "\n";
    }
    std::cout << "each data delay " << total_time / DATASCALE << "ms\n";
    outfile.close();
    return NULL;
}

int main()
{
    ::srand(::time(NULL));
    std::list<dataform> datapool;
    pthread_t tid;
    pthread_create(&tid, NULL, thd_do, NULL);
    pthread_detach(tid);

    for (int i = 0;i < DATASCALE; ++i)
    {
        dataform data;
        datapool.push_back(data);
    }

    unsigned send_cnt = 0;

    unsigned long first_snd_time = getCurrentTimeInMillis();

    for (std::list<dataform>::iterator it = datapool.begin();
        it != datapool.end(); ++it)
    {
        it->sendtime = getCurrentTimeInMillis();
        char sendbuffer[1024];
        unsigned datalen;
        if (it->encode(sendbuffer, 1024, datalen))
            if (tcommu.produce(sendbuffer, datalen) == QUEUE_SUCC)
                ++send_cnt;
    }

    unsigned long last_snd_time = getCurrentTimeInMillis();

    std::cout << "send out " << send_cnt << " data in " << last_snd_time - first_snd_time <<" ms\n";
    std::ofstream outfile;
    outfile.open("sendt.txt", std::ofstream::out);
    for (std::list<dataform>::iterator it = datapool.begin();
        it != datapool.end(); ++it)
        outfile << it->context << "\n";
    outfile.close();
    sleep(10000);
}
