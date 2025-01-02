#include <cstring>
#include <iostream>
#include <thread>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "groupuser.hpp"
#include "json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

using json = nlohmann::json;

//记录信息 当用户想要查看相关信息 直接在客户端执行
//记录当前系统登录的用户信息
User g_currentUser;
//记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表信息
std::vector<Group> g_currentUserGroupList;
//显示当前登录成功用户的基本信息
void showCurrentUserData();
//控制MainMenu界面程序布尔值
bool isMainMenuRunning = false;

//接收线程 多线程防止输入阻塞接收
void readTaskHandler(int clientfdl);
//获取系统时间(聊天信息需要添加时间信息)
std::string getCurrentTime();
//主聊天界面程序
void mainMenu(int clientfd);

//聊天客户端程序实现, main线程作为发送线程, 子线程作为接收线程  
int main(int argc, char** argv){
    if(argc < 3){
        std::cerr << "command invalid! example: ./Chatclient 127.0.0.1 8080" << std::endl;
        std::exit(-1);
    }
    
    //解析通过命令行参数传递的ip和port
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]); 

    //创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1){
        std::cerr << "socket create error" << std::endl;
        exit(-1);
    }

    //填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    
    //client和server进行连接 //因为server是个IPv4的协议所以需要强转成通用的sockaddr类型
    if(connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)) == 1){
        std::cerr << "connect server error" << std::endl;
        close(clientfd);
        exit(-1);
    }

    //main线程用于接收用户输入, 负责发送数据
    for(;;){
        //显示首页面菜单 登录 注册 退出
        std::cout << "========================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << "choice:";
        int choice = 0;
        std::cin >> choice;
        std::cin.get(); //读掉缓冲区残留的回车
        
        switch (choice) {

            case 1:{ //login业务
                int id = 0;
                char pwd[50] = {0};
                std::cout << "userid:";
                std::cin >> id;
                std::cin.get();
                std::cout << "userpassword:";
                std::cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                std::string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);        
                if(len == -1){
                    std::cerr << "send login msg error:" << request << std::endl;
                }
                else{
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if(len == -1){
                        std::cerr << "recv login response error" << std::endl;
                    }
                    else{
                        json responsejs = json::parse(buffer);
                        if(responsejs["errno"].get<int>() != 0){ //登录失败
                            std::cerr << responsejs["errmsg"] << std::endl;
                        }
                        else{ //登录成功
                            //记录当前用户的id和name
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);

                            //记录当前用户的好友列表信息
                            if(responsejs.contains("friends")){ //当前用户是否拥有好友
                                //每次登陆显示数据前 清除一下上次登录的数据
                                g_currentUserFriendList.clear();
                                std::vector<std::string> friends = responsejs["friends"];
                                for(std::string& buddy : friends){
                                    json js = json::parse(buddy);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }
                            
                            //记录当前用户的群组列表信息
                            if(responsejs.contains("groups")){
                                //清理记录
                                g_currentUserGroupList.clear();

                                std::vector<std::string> groupinfos = responsejs["groups"];
                                for(std::string& groupinfo : groupinfos){
                                    json grpjs = json::parse(groupinfo);
                                    Group group;
                                    group.setId(grpjs["id"].get<int>());
                                    group.setName(grpjs["groupname"]);
                                    group.setDesc(grpjs["groupdesc"]);
                                    std::vector<std::string> userinfos = grpjs["users"];
                                    for(std::string& userinfo : userinfos){
                                        json userjs = json::parse(userinfo);
                                        GroupUser user;
                                        user.setId(userjs["id"].get<int>());
                                        user.setName(userjs["name"]);
                                        user.setState(userjs["state"]);
                                        user.setRole(userjs["role"]);
                                        group.getUsers().push_back(user);

                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            //显示登录用户的基本信息 
                            showCurrentUserData();
                            
                            //显示当前用户的离线消息 个人聊天信息或者群组消息
                            if(responsejs.contains("offlinemsg")){
                                std::vector<std::string> msgs = responsejs["offlinemsg"];
                                for(std::string& msg : msgs){ //从离线消息中判断是个人离线消息还是群组离线消息
                                    json js = json::parse(msg);
                                    int msgtype = js["msgid"].get<int>();
                                    std::cout << "type: " << msgtype;
                                    if(msgtype == ONE_CHAT_MSG){
                                        std::cout << "personal message: " << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>() << " said: " 
                                                  << js["msg"].get<std::string>() << std::endl;
                                    }
                                    else if(msgtype == GROUP_CHAT_MSG){
                                        std::cout << "group [id:" << js["groupid"] << "]" << "message: " << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>() << " said: " 
                                                  << js["msg"].get<std::string>() << std::endl;
                                    }
                                }
                            }

                            //登录成功, 启动接收线程负责接收数据 该线程只启动一次 防止recv函数阻塞
                            static bool readThreadInitialized = false;
                            if(!readThreadInitialized){
                                std::thread readTask(readTaskHandler, clientfd);
                                readThreadInitialized = true;
                                readTask.detach(); //不分离 如果主线程没有调用join方法 就会造成tcp资源泄露 //分离子线程 资源在线程结束后会自动被回收
                            }
                            //进入聊天主菜单界面
                            isMainMenuRunning = true;
                            mainMenu(clientfd);
                            
                        }
                    }
                }
            }
            break;
            case 2: //register业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                std::cout << "username:";
                std::cin.getline(name, 50); //最多50个字符
                std::cout << "userpassword:";
                std::cin.getline(pwd, 50);
                json js;

                //确保刷入输出流 否则不会打印出错误消息
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                std::string request = js.dump();
                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0); //(+1写在里面是指针运算)
                if(len == -1){
                    std::cerr << "send reg msg error: " << request << std::endl;
                }
                else{
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0); //阻塞等待 //send 和 recv函数是基于socket的所以一个socket发送数据后 recv表示的是这个接口在等待数据
                    if(len == -1){
                        std::cerr << "recv reg response error" << std::endl;
                    }
                    else{
                        json responsejs = json::parse(buffer);
                        if(responsejs["errno"].get<int>() != 0 ){
                            std::cerr << name << "invalid username, register error!" << std::endl;
                        }
                        else{ //注册成功
                            std::cout << name << " register success, userid is " << responsejs["id"] << ", keep it as identity" << std::endl;
                        }
                    }
                }
            }
            break; 
            case 3:{ //quit业务
                close(clientfd);
                exit(0);
            }
            default:
                std::cerr << "invalid input!" << std::endl;
                break;
        }

    }


}

//接收线程
void readTaskHandler(int clientfd){ //不存在竞态条件
    for(;;){
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024,0 );
        if(len == -1 || len == 0){
            close(clientfd);
            exit(-1);
        }
        //通过ChatServer接收其他用户发送的消息 反序列化得到js对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(msgtype == ONE_CHAT_MSG){
            std::cout << "personal message: " << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>() << " said: " 
                      << js["msg"].get<std::string>() << std::endl;
            continue;
        }
        if(msgtype == GROUP_CHAT_MSG){
            std::cout << "group [id:" << js["groupid"] << "]" << "message: " << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>() << " said: " 
                      << js["msg"].get<std::string>() << std::endl;
            continue;
        }
    }
}

void showCurrentUserData(){
    std::cout << "===================================login user============================" << std::endl;
    std::cout << "current login user => id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << std::endl;
    std::cout << "-----------------------------------friend list---------------------------" << std::endl;
    if(!g_currentUserFriendList.empty()){
        for(User& user : g_currentUserFriendList){
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "-----------------------------------group list-----------------------------" << std::endl;
    if(!g_currentUserGroupList.empty()){
        for(Group& group : g_currentUserGroupList){
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for(GroupUser& user : group.getUsers()){
                std::cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << std::endl;

            }
        }
    }
    std::cout << "=========================================" << std::endl;
}

//"help" command handler
void help(int fd = 0, std::string str = "");
//"chat" command handler
void chat(int, std::string);
//"addfriend" command handler
void addfriend(int, std::string);
//"creategroup" command handler
void creategroup(int, std::string);
//"addgroup" command handler
void addgroup(int, std::string);
//"groupchat" command handler
void groupchat(int, std::string);
//"logout" command handler
void logout(int, std::string);

//系统支持的客户端命令列表 //表驱动设计
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "show all supported commands, command format:help"},
    {"chat", "one-to-one chat, command format:chat:friendid:message"},
    {"addfriend", "add friend, command format:addfriend:friendid"},
    {"creategroup", "create group, command format:creategroup:groupname:groupdesc"},
    {"addgroup", "join group, command format:addgroup:groupid"},
    {"groupchat", "group chat, command format:groupchat:groupid:message"},
    {"logout", "quit, command format:logout"}
};

//注册系统支持的客户端命令处理
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}
};

void mainMenu(int clientfd){
    help();

    char buffer[1024] = {0};
    while(isMainMenuRunning){
        std::cin.getline(buffer, 1024);
        std::string commandBuf(buffer);
        std::string command; //存储命令
        int idx = commandBuf.find(":");
        if(idx == -1){
            command = commandBuf;
        }
        else {
            command = commandBuf.substr(0, idx);
        }
        //查找command是否在映射表里
        auto it =   commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()){
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }
        
        //调用响应命令的事件处理回调, mainMenu对修改封闭, parse通过在handler内部实现, 添加新功能不需要修改mainMenu函数
        it->second(clientfd, commandBuf.substr(idx+1, commandBuf.size() - idx)); //调用命令处理方法
    }
}

//"help" command handler
void help(int, std::string){
    std::cout << "show command list >>> " << std::endl;
    for(auto& p : commandMap){
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

//"addfriend" command handler
void addfriend(int clientfd, std::string commandArgs){
    int friendid = atoi(commandArgs.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len == -1){
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
}

//"chat" command handler
void chat(int clientfd, std::string msg){
    int idx = msg.find(":");
    if(idx == -1){
        std::cerr << "chat command invalid" << std::endl;
        return;
    }
    
    int friendid = std::stoi(msg.substr(0, idx));
    std::string message = msg.substr(idx + 1, msg.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();
    
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len == -1){
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}

//"creategroup" command handler
void creategroup(int clientfd, std::string commandArgs){
    int idx = commandArgs.find(":");
    if(idx == -1){
        std::cerr << "creategroup command invalid" << std::endl;
        return;
    } 
    
    std::string groupname = commandArgs.substr(0, idx);
    std::string groupdesc = commandArgs.substr(idx+1, commandArgs.size()-idx);
    std::cout << groupname << groupdesc;
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1){
        std::cerr << "send creategroup msg error" << buffer << std::endl;
    }
}
//"addgroup" command handler
void addgroup(int clientfd, std::string commandArgs){
    int groupid = atoi(commandArgs.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len == -1){
        std::cerr << "send addgorup msg error -> " << buffer << std::endl;
    }
}
//"groupchat" command handler
void groupchat(int clientfd, std::string commandArgs){
    int idx = commandArgs.find(":"); 
    if(idx == -1){
        std::cerr << "chat command invalid" << std::endl;
        return;
    }

    int groupid = std::stoi(commandArgs.substr(0, idx));
    std::string message = commandArgs.substr(idx+1, commandArgs.size()-idx);
    
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1){
        std::cerr << "send groupchat msg error" << buffer << std::endl;
    }

}
//"logout" command handler
void logout(int clientfd, std::string){
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len == -1){
        std::cerr << "send logout msg error -> " << buffer << std::endl;
    }else{
        isMainMenuRunning = false;
    }
}

//获取系统时间 (聊天信息需要添加时间信息)
std::string getCurrentTime(){
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); 
    struct tm* ptm = localtime(&tt);;
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}