#ifndef __ARRMQ_HEADER__
#define __ARRMQ_HEADER__

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

inline uint64_t getCurrentMillis()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    uint64_t ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}

/*
    each message format
    (^BE=|TS|LENGTH|DATA|=ND$)
*/

#define BIG_ENDIAN_VALUE 0
#define LITTLE_ENDIAN_VALUE 1
#define BEGIN_BOUND_VALUE 0x3d45425e//little-endian of: "^BE="
#define END_BOUND_VALUE 0x24444e3d//little-endian of: "=ND$"
#define BOUND_VALUE_LEN 4
#define MSG_HEAD_LEN (12 + BOUND_VALUE_LEN) // BOUND_VALUE_LEN + uint64_t + uint32_t

class ArrayMQ
{
public:
    ArrayMQ(uint32_t mq_size);
    virtual ~ArrayMQ();

    /*
     enqueue: push data to shmmq
     data: write content
     data_len: write content size
    */
    int enqueue(const void *data, unsigned data_len);

    /*
     dequeue: pop data from shmmq
     buffer: read buffer
     buffer_size: read buffer size
     data_len: data length (return)
     send_t: data's send timestamp
    */
    int dequeue(void *buffer, unsigned buffer_size, unsigned &data_len, uint64_t &send_ts);

private:
    void init();

    char endian_solution;//machine endian

    /* mq format

      block_ptr = |data block|

      data block is a ring queue
      head_addr is queue's head
      tail_addr is queue's tail
    */
    char *block_ptr;//data block head ptr
    uint32_t block_size;//data block size

    uint32_t head_addr;//mq head
    uint32_t tail_addr;//mq tail
};

#endif