#include <unistd.h>
#include <sys/eventfd.h>
#include "tcomm_mq.h"

TCommMQ::TCommMQ(uint32_t mq_size)
{
    mq = new ArrayMQ(mq_size);
    exit_if(mq == NULL, "no space to new ArrayMQ");

    evfd = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK | EFD_CLOEXEC);
    exit_if(evfd == -1, "eventfd create error");
}

TCommMQ::~TCommMQ()
{
    delete mq;
    close(evfd);
}

int TCommMQ::produce(const void *data, unsigned data_len)
{
    int ret = mq->enqueue(data, data_len);
    if (ret == QUEUE_SUCC)
    {
        unsigned long long number = 1;
        write(evfd, &number, sizeof(unsigned long long));
    }
}

int TCommMQ::consume(void *buffer, unsigned buffer_size, unsigned &data_len)
{
    int ret = mq->dequeue(buffer, buffer_size, data_len);
    if (ret != QUEUE_ERR_EMPTY)
    {
        unsigned long long number;
        read(evfd, &number, sizeof(unsigned long long));
    }
    return ret;
}
