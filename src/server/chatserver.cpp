#include "chatserver.hpp"
#include "chatservice.hpp"
#include <functional>
#include <sys/socket.h>
#include <string>
#include "json.hpp"
using namespace std::placeholders;
using json = nlohmann::json;

//初始化chatserver对象
ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr,
                     const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop{loop} 
{
  // 注册连接事件回调函数
  _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

  // 注册读写事件回调函数
  _server.setMessageCallback(
      std::bind(&ChatServer::onMessage, this, _1, _2, _3));

  // 设置线程数量
  _server.setThreadNum(4);
}

//开始服务
void ChatServer::Start(){
    _server.start();
}

// 上报连接相关的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn){
    //the client disconnected
    if(!conn->connected()){
        ChatService::getChatService()->clientCloseException(conn);
        conn->shutdown(); //release the source of fd
    }
}
// 上报信息相关的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time){
    std::string buf = buffer->retrieveAllAsString();  
    //数据的反序列化
    json js = json::parse(buf);
    //将网络模块和业务模块解耦
    //解耦的两种方式 1.通过面向接口编程 2.通过回调函数
    //通过js["msgid"] 获取=>业务handler
    auto msgHandler = ChatService::getChatService()->getHandler(js["msgid"].get<int>()); //把json键对应的值强转成对应的实际类型  
    //通过消息拿到对应类型(e.x.login)的处理器后 传参进行回调bind所绑定的方法                                                                                         
    msgHandler(conn, js, time);
}
