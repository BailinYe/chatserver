#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

//chatserver主类
class ChatServer{
public:
    //Tcpserver 没有默认构造函数
    //初始化 chatserver 对象
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);

    //start the service
    void Start();

private:
    //报告连接相关信息回调
    void onConnection(const TcpConnectionPtr&);
    //上报读写信息相关信息回调
    void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);

    TcpServer _server; //组合对象 实现类功能的类对象
    EventLoop* _loop; //目的是在合适的事件退出事件循环
};


#endif