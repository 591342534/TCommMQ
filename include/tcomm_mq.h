#ifndef __TCOMMMQ_HEADER__
#define __TCOMMMQ_HEADER__

#include "arrmq.h"
#include "errors.h"

class TCommMQ
{
public:
    TCommMQ(uint32_t mq_size = 838860800);//default size = 800MB
    ~TCommMQ();

    int produce(const void *data, unsigned data_len);
    int consume(void *buffer, unsigned buffer_size, unsigned &data_len);

    int notifier() const { return evfd; }
private:
    int evfd;   //eventfd
    ArrayMQ *mq;//mq
};

#endif
