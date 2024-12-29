#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <functional>
#include <iostream>
#include <string>
using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;

// 基于muduo网路库的服务端开发程序
// 流程
// 1.组合TcpServer对象
// 2.创建事件循环指针
// 3.明确TcpServer构造函数需要的参数 确定ChatServer的构造参数
// 4.在服务器类的构造函数当中, 注册处理连接和处理读写事件的回调函数
// 5.设置合适的服务端线程数量, muduo库会自动分配I/O线程和worker线程
// 固定模板 很少需要改动

class ChatServer
{
public:
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
            : _server(loop, listenAddr, nameArg)
            , _loop(loop)
            {
                //使用网络库函数给服务器注册用户创建回调 网络库会帮我们监听 
                //注意 连接或者断开onConnection都会被回调
                _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
                //使用库函数对用户读写事件注册回调
                _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3)); //占位符表示回调传给函数的参数
                //设置服务器端的线程数量 1个I/O线程 3个worker线程
                _server.setThreadNum(4); //最好设置和服务器核数相同数量的线程
            }

    //开启事件循环
    void Start(){
        _server.start();
    }
        
private:
    //处理用户的连接和断开 
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:on" << std::endl;
        }
        else{
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:off" << std::endl;
            conn->shutdown(); //close(fd)
            //_loop.quit();
        }
    }
    void onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp time)
    {
        std::string buftext = buffer->retrieveAllAsString();
        std::cout << "Receive data: " << buftext << "Time: " << time.toString() << std::endl;
        conn->send(buftext);
    }
    TcpServer _server;
    EventLoop* _loop; 
};

int main(){

    EventLoop loop;
    InetAddress addr("192.168.0.100", 6000);
    ChatServer server(&loop, addr, "CharServer");
    
    server.Start(); //启动服务 把listenfd 通过 epoll_ctl 添加到 epoll上
    loop.loop(); //epoll_wait 以阻塞方式等待用户连接 或者已连接用户的读写事件

}