# TCommMQ:线程间基于事件的消息队列

> 带IO事件通知机制的、基于循环数组的线程间消息队列；
> 

### 特性
- “有数据可消费”为一个IO可读事件，比较适合EPOLL服务器的线程间数据交互
- 支持单消费者线程与单生产者线程
- 原理上支持多生产者线程与多消费者线程，但是出于易用性的考虑，暂时TODO

### USAGE
#### head file
```cpp
#include "tcomm_mq.h"
```
#### 1 create TComMQ
```cpp
TCommMQ tcommu(size = 838860800, timeout = -1);
//size(Bit): MQ大小
//timeout(ms): 丢弃那些在MQ里存在超过timeout(ms)的消息
```
#### 2 produce message
是非阻塞操作
```cpp
int ret = tcommu.produce(string_data, data_size);
//return code:
//QUEUE_SUCC：成功写入
//QUEUE_ERR_FULL：MQ已满
```
#### 3 consume message
是非阻塞操作
```cpp
char readbuffer[BUFFSIZE];
unsigned data_len;
int ret = tcommu.consume(readbuffer, BUFFSIZE, data_len);
//return code:
//QUEUE_SUCC：成功读取
//QUEUE_ERR_EMPTY：MQ为空
//QUEUE_ERR_...:各种内部错误，因为内存乱序（因为是理论上几乎不可能出现的错误，尚未想好对应的处理办法）
```
#### 4 consume with Multiplex IO

TComMQ对应的文件描述符fd =` tcommu.notifier()`，当数据到达MQ or MQ有数据可读，fd产生可读事件

```cpp
//example: epoll
int efd = epoll_create1(0);

//监控MQ的可读事件
struct epoll_event event;
event.events = EPOLLIN;
event.data.fd = tcommu.notifier();
epoll_ctl(efd, EPOLL_CTL_ADD, tcommu.notifier(), &event);

while (true)
{
	if (epoll_wait(efd, &revent, 1, 10) > 0)
	{
        //一直从MQ中拿数据
	    while (tcommu.consume(readbuffer, BUFFSIZE, data_len) == QUEUE_SUCC)
	    {
            //业务处理数据
	    }
	}
}
```

### 测试v1

> **100W data，length 100B**

| test id | push | pop | each delay |
| :-----: |:-----:|:-----:|:-----:|
|1|1860ms|1860ms|0.007ms|
|2|2048ms|2048ms|0.009ms|
|3|1963ms|1963ms|0.029ms|
|4|2145ms|2145ms|0.065ms|
|5|2106ms|2106ms|0.019ms|
|6|2071ms|2071ms|0.026ms|
|7|2165ms|2165ms|0.027ms|
|8|2091ms|2091ms|0.043ms|
|9|1496ms|1496ms|0.006ms|
|10|1429ms|1429ms|0.013ms|

> **1000W data，length 100B**

| test id | push | pop | each delay |
| :-----: |:-----:|:-----:|:-----:|
|1|11080ms|11080ms|0.012ms|
|2|10456ms|10456ms|0.015ms|
|3|11125ms|1963ms|0.027ms|

