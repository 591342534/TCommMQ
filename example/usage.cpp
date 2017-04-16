#include <iostream>
#include <list>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include "tcomm_mq.h"

static string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

struct dataform
{
    std::string context;
    unsigned sendtime;
    unsigned recvtime;

    dataform(): sendtime(0), recvtime(0)
    {
        int size = ::rand() % 50;
        context.resize(size);
        for (int i = 0;i < context.size(); ++i)
            context[i] = charset[::rand() % charset.length()];
    }

    bool encode(char* buffer, unsigned buffer_size, unsigned& data_size)
    {
        //sendtime|recvtime|context_len|context
        if (context.size() + sizeof(unsigned) * 3 > buffer_size)
            return false;
        data_size = context.size() + sizeof(unsigned) * 3;
        *(unsigned)buffer = sendtime;
        *((unsigned)buffer + 1) = recvtime;
        *((unsigned)buffer + 2) = context.size();
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
        sendtime = *(unsigned)buffer;
        recvtime = *((unsigned)buffer + 1);
        unsigned str_size = *((unsigned)buffer + 2);
        buffer += sizeof(unsigned) * 3;
        if (data_size != str_size + sizeof(unsigned) * 3)
            return false;
        context.assign(buffer, buffer + str_size);
        return true;
    }
}

long int getCurrentTimeInMillis()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}

TCommMQ tcomm;

void* thd_do(void* args)
{
    #define BUFFSIZE 3072
    char readbuffer[BUFFSIZE];
    unsigned data_len;

    tcomm.consume(readbuffer, BUFFSIZE, data_len);
    return NULL;
}

int main()
{
    ::srand(::time(NULL));
    #define DATASCALE 1000000
    std::list<dataform> datapool;
    pthread_t tid;
    pthread_create(&tid, NULL, thd_do, NULL);
    pthread_detach(tid);

    for (int i = 0;i < DATASCALE; ++i)
    {
        dataform data;
        datapool.push_back(data);
    }

    for (std::list<dataform>::iterator it = datapool.begin();
        it != datapool.end(); ++it)
    {
        it.
    }
}
