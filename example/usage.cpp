#include <iostream>
#include <list>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <string>
#include <vector>
#include <fstream>
#include "tcomm_mq.h"

#define DATASCALE 5000000

static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

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

    std::vector<std::pair<std::string, unsigned long> > datapool(DATASCALE);
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
                if (recv_cnt == 0)
                    first_recv_time = getCurrentTimeInMillis();

                //datapool[recv_cnt].first = readbuffer;
		datapool[recv_cnt].second = getCurrentTimeInMillis();
                ++recv_cnt;
                if (DATASCALE == recv_cnt)
                    break;
            }
            if (DATASCALE == recv_cnt)
                break; 
        }
    }
    //persist data to test data content correctness
    unsigned long last_recv_time = getCurrentTimeInMillis();
    std::ofstream outfile;
    outfile.open("rect.txt", std::ofstream::out);
    for (std::vector<std::pair<std::string, unsigned long> >::iterator it = datapool.begin();
        it != datapool.end(); ++it)
        outfile << "x" << "+" << it->second << "\n";

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
        std::string data;
        int size = 100;
        data.resize(size);
        for (int i = 0;i < data.size(); ++i)
            data[i] = charset[::rand() % charset.length()];
        std::pair<std::string, unsigned long> mypair(data, 0);
        datapool.push_back(mypair);
    }

    unsigned send_cnt = 0;

    sleep(2);
    std::cout << "now send\n";
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
        outfile << 'x' << "+" << it->second << "\n";
    outfile.close();
    sleep(3);
}
