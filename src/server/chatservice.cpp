#include "chatservice.hpp"
#include "friendmodel.hpp"
#include "groupuser.hpp"
#include "public.hpp" 
#include <functional>
#include <muduo/net/Callbacks.h>
#include <mutex>
#include <string>
#include <vector>
#include <muduo/base/Logging.h>
#include <iostream>

using namespace std::placeholders;
using namespace muduo;
//线程安全的懒汉式模式获取单例接口函数
ChatService* ChatService::getChatService(){
    static ChatService service;
    return &service;
}

//注册消息以及对应的handler回调操作
ChatService::ChatService(){
    //因为是成员对象 所以需要通过bind方法绑定当前对象
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});     
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});     
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});

    //连接redis服务器
    if(_redis.connect()){
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

//业务代码应该只处理对象
// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int id = js["id"].get<int>(); //json返回默认为string类型
    std::string password = js["password"]; 

    //TODO: 需要做代码长度限制 防止用户密码为空 
    User user = _userModel.query(id);
    //主键从0开始 id为-1表示用户不存在
    if(user.getId() != -1 && user.getPassword() == password){
        if(user.getState() == "online"){
            //该用户已经登陆, 不允许重复登录
            // 登录失败
            json response;
            // 序列化响应消息
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is online, do not login again";
            conn->send(response.dump());
        } else {
            // 登录成功
            // 记录用户连接信息
            { //缩小锁的粒度
                std::lock_guard<std::mutex> lock(_connMutex) ; 
                _userConnectionMap.insert({user.getId(),conn}); //临界代码区
            }
            //id用户登录成功以后, 向redis订阅channel(id)
            _redis.subscribe(id);
            
            // 更新用户状态信息 offline => online
            user.setState("online");
            _userModel.updateState(user);
            json response;
            // 序列化响应消息
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            //查询该用户是否有离线消息
            std::vector<std::string> rows = _offlineMsgModel.query(user.getId());
            if(!rows.empty()){
                //用户有离线消息
                //将容器赋给json对象
                response["offlinemsg"] = rows;
                //读取该用户的离线消息后 把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(user.getId());
            }
            //查询该用户好友信息并返回
            std::vector<User> friends = _friendModel.query(user.getId());
            if(!friends.empty()){
                std::vector<std::string> friendsInfo;
                for(User& user : friends){
                    //对好友信息进行序列化
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    friendsInfo.push_back(js.dump());
                }
                response["friends"] = friendsInfo;
            }
            //查询用户群组信息
            std::vector<Group> groups = _groupModel.queryGroups(user.getId()); //角色信息通过每次调取该方法 从数据库表中获取角色信息
            if(!groups.empty()){
                std::vector<std::string> groupvec;
                for(Group& group : groups){
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();
                    std::vector<std::string> uservec;
                    for(GroupUser& user : group.getUsers()){
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        uservec.push_back(js.dump());
                    }
                    groupjs["users"] = uservec;
                    groupvec.push_back(groupjs.dump()); 
                }
                response["groups"] = groupvec;
            }
            conn->send(response.dump());
        }
    }else{
        //登录失败 用户不存在或者登录密码错误
        json response;
        //序列化响应消息
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is wrong";
        conn->send(response.dump());
    }
}
// 处理注册业务  // register是关键字
// 何时调用和此类无关
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time){
    std::string name = js["name"];
    std::string password = js["password"];
    User user; //只操作数据对象
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);
    if(state){
        //注册成功
        json response;
        //序列化响应消息
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());

    }else{
        //注册失败
        json response;
        //序列化响应消息
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }

}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志, msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid); //(使用find避免自动添加)
    if(it == _msgHandlerMap.end()){
        //使用muduo库日志模块
        //返回一个默认的处理器 空操作 //防止处理 网络模块导致的异常
        return [=](auto a, auto b, auto c){
            LOG_ERROR << "msgid: " << msgid << "can not find the handler!"; 
        };
    }                                          
    return _msgHandlerMap[msgid];
}

//logout command handler
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(userid);
        if(it != _userConnectionMap.end()){
            _userConnectionMap.erase(it); 
        }
    }

    //用户注销, 相当于下线, 在redis中取消订阅通道
    _redis.unsubscribe(userid);

    if (userid != -1) {
        // 更新用户状态信息
        User user;
        user.setId(userid);
        user.setState("offline");
        _userModel.updateState(user); // 根据user对象的状态信息更新表
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn){
    User user;
    {
      // 保证异常退出的用户不会因为_userConnectionMap对在线用户造成线程安全问题
      std::lock_guard<std::mutex> lock(_connMutex);
      for (auto it = _userConnectionMap.begin(); it != _userConnectionMap.end();
           ++it) {
        if (it->second == conn) {
          // 从_userConnectionMap 删除用户连接信息
          user.setId(it->first);
          _userConnectionMap.erase(it);
          break;
        }
      }
    }

    //用户注销, 相当于下线, 在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    if (user.getId() != -1) {
        // 更新用户状态信息
        user.setState("offline");
        _userModel.updateState(user); // 根据user对象的状态信息更新表
    }
}

//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int toid = js["toid"].get<int>();    
    {
        //线程安全 因为可能有其它线程在修改_userConnectionMap这个变量
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(toid);
        if(it != _userConnectionMap.end()){
          // toid在线 转发消息 服务器主动推送消息给id为toid的用户 两个用户在同一台服务器上
          it->second->send(js.dump());
          return;
        }
    }

    //查询toid是否在线
    User user = _userModel.query(toid);
    if(user.getState() == "online"){
        //目标用户在线但与源用户不在同一台主机上
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线 存储离线消息
    _offlineMsgModel.insert(toid, js.dump());    
}

//添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid, friendid);

}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group)){
        //存储群组创建人信息
        _groupModel.addGroup(group.getId(), userid, "creator");
    }
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(groupid, userid, "normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> userids = _groupModel.queryGroupUsers(userid, groupid);

    std::lock_guard<std::mutex> lock(_connMutex);
    for(int id : userids){
        auto it = _userConnectionMap.find(id);
        if(it != _userConnectionMap.end()){
            //转发群消息
            it->second->send(js.dump());
        }
        else{
            //查询toid是否在线在其它服务器
            User user = _userModel.query(id);
            if(user.getState() == "online"){
                _redis.publish(id, js.dump());
            }else{
                //存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

void ChatService::reset(){
    //把全部在线用户强制下线
    _userModel.resetState();
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, std::string msg){

    std::lock_guard<std::mutex> lock(_connMutex);
    auto it = _userConnectionMap.find(userid);
    if(it != _userConnectionMap.end()){
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);

}