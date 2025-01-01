#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/Callbacks.h>
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "groupmodel.hpp"
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;
using json = nlohmann::json;

//处理消息事件的回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;

// 聊天服务器业务类
//
class ChatService{
public:
    //线程安全的懒汉式模式获取单例接口函数
    static ChatService* getChatService();
    //由网络层派发的处理器回调
    //处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time); //register是关键字
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);                                
    //添加好友业务
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //注销业务
    void logout(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //服务器异常, 业务重置方法
    void reset();

private:
    ChatService();

    //存储消息ID和其对应的业务处理方法
    std::unordered_map<int, MsgHandler> _msgHandlerMap;
    
    //数据操作类对象
    //model 给业务层提供的都是对象
    UserModel _userModel; //服务只依赖model类 并不直接涉及数据库相关的操作 数据库相关的操作封装在了model里
    offlineMsgModel _offlineMsgModel;

    //存储在线用户的连接 //会随着用户的上线和下线不断改变 需要保证线程安全
    std::unordered_map<int, TcpConnectionPtr> _userConnectionMap;
     //互斥锁对象 保证_userConnectionMap insert delete 操作的线程安全
    std::mutex _connMutex;
    FriendModel _friendModel;
    GroupModel _groupModel; 
};

#endif