#ifndef __ERRORS_HEADER__
#define __ERRORS_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

enum RETCODE
{
    QUEUE_SUCC,
    QUEUE_ERR_FULL,       //full, can't enqueue more
    QUEUE_ERR_EMPTY,      //empty, can't dequeue more
    QUEUE_ERR_CHECKSEN,   //sentinel check wrong.
    QUEUE_ERR_MEMESS,     //mem is fuck up
    QUEUE_ERR_OTFBUFF,    //user buffer overflow
    QUEUE_ERR_TIMEOUT,    //no data arrive in mq within the specified time
//  QUEUE_ERR_OVLIMIT,    //message size > length limit
    SYS_ERR               //system-call error
};

static const char* errmsgs[] = 
{
    "success",
    "no enough space for mq",
    "mq empty",
    "sentinel check error",
    "mem is messed up",
    "user buffer overflow",
    "timeout",
//  "message size over length limit",
    "system-call error"
};

#define TELL_ERROR(format, ...) fprintf(stderr, "ERROR: %s\n", format, ## __VA_ARGS__)

#define TELL_SYS_ERROR fprintf(stderr, "ERROR: %s\n", strerror(errno))

inline void exit_if(int condition, const char *format, ...)
{
    va_list arglist;
    if (condition)
    {
        va_start(arglist, format);
        fprintf(stderr, "EXIT BECAUSE OF ERROR: ");
        vfprintf(stderr, format, arglist);
        fprintf(stderr, ": %s\n", strerror(errno));
        va_end(arglist);

        exit(EXIT_FAILURE);   
    }
}

#endif
