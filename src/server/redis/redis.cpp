#include "redis.hpp"
#include <cstdlib>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <iostream>
#include <thread>

Redis::Redis()
    : _publish_context(nullptr), _subscribe_context(nullptr)
{

}

Redis::~Redis()
{
    if(_publish_context != nullptr){
        redisFree(_publish_context);
    } 
    if(_subscribe_context != nullptr){
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    //负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(_publish_context == nullptr){
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }
    //负责subscribe订阅消息的上下文
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if(_subscribe_context == nullptr){
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    //在单独的线程中(因为subscribe是阻塞操作), 监听通道上的事件, 有消息给业务层进行上报
    std::thread t([&](){
        observer_channel_message();
    });
    t.detach();
    
    std::cout << "connect redis-server success" << std::endl;
        
    return true;
}

//向redis指定的通道channel发布消息
bool Redis::publish(int channel, std::string message){
    //rediscommand 相当于redisAppenCommand, redisBufferWrite 和 redisGetReply
    redisReply* reply = (redisReply*) redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(reply == nullptr){
        std::cerr << "publish command failed" << std::endl;
        return false;
    } 
    freeReplyObject(reply);
    return true;
}

//向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel){
    //一个ChatServer只有一个I/O线程和三个worker线程 不能每次订阅都导致一个线程阻塞等待通道消息 所以将订阅和接收操作拆分开来 
    //SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息, 这里只做订阅通道, 不接受通道消息
    //通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    //只负责发送命令, 不阻塞接收redis server响应消息, 否则和notifyMsg线程抢占响应资源
    if(redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR){
        std::cerr << "subscribe command failed" << std::endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区, 直到缓冲区数据发送完毕(done被置为1) 
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR){
            std::cerr << "subscribe command failed!" << std::endl;
            return false;
        }
    }
    //省略redisReply 从而不会阻塞等待redis server给我们的响应

    return true; 
}

//向redis指定的通道subscribe订阅消息
bool Redis::unsubscribe(int channel){
    //一个ChatServer只有一个I/O线程和三个worker线程 不能每次订阅都导致一个线程阻塞等待通道消息 所以将订阅和接收操作拆分开来 
    //SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息, 这里只做订阅通道, 不接受通道消息
    //通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    //只负责发送命令, 不阻塞接收redis server响应消息, 否则和notifyMsg线程抢占响应资源
    if(redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR){
        std::cerr << "unsubscribe command failed" << std::endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区, 直到缓冲区数据发送完毕(done被置为1) 
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR){
            std::cerr << "unsubscribe command failed!" << std::endl;
            return false;
        }
    }
    //省略redisReply 从而不会阻塞等待redis server给我们的响应

    return true; 
}

//在独立的线程中接收订阅通道的消息
void Redis::observer_channel_message()
{
    redisReply* reply = nullptr;
    //阻塞等待订阅频道的消息
    while(redisGetReply(this->_subscribe_context, (void **)& reply) == REDIS_OK){
        //订阅收到的消息是一个带三元素的数组
        if(reply != nullptr && reply->element[1] != nullptr && reply->element[1]->str != nullptr
            && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<" << std::endl;
}

void Redis::init_notify_handler(std::function<void(int, std::string)> fn){
    this->_notify_message_handler = fn;
}