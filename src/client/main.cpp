#include <cstring>
#include <iostream>
#include <thread>
#include <string>
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

//接收线程 多线程防止输入阻塞接收
void readTaskHandler(int clientfdl);
//获取系统时间(聊天信息需要添加时间信息)
std::string getCurrentTime();
//主聊天界面程序
void mainMenu();

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
                                std::vector<std::string> groupinfos = responsejs["groups"];
                                for(std::string& groupinfo : groupinfos){
                                    json grpjs = json::parse(groupinfo);
                                    Group group;
                                    group.setId(grpjs["id"].get<int>());
                                    group.setName(grpjs["groupname"]);
                                    group.setDesc(grpjs["groupdesc"]);

                                    std::vector<std::string> userinfos = grpjs["users"];
                                    for(std::string& userinfo : userinfos){
                                        GroupUser user;
                                        json js = json::parse(userinfo);
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        user.setRole(js["role"]);
                                        group.getUsers().push_back(user);

                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            //显示登录用户的基本信息 
                            showCurrentUserData();

                            //登录成功, 启动接收线程负责接收数据
                            std::thread readTask(readTaskHandler, clientfd);
                            readTask.detach(); //不分离 如果主线程没有调用join方法 就会造成tcp资源泄露 //分离子线程 资源在线程结束后会自动被回收
                            //进入聊天主菜单界面
                            mainMenu();
                            
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
                std::cout << user.getId() << " " << user.getName() << " " << user.getState() << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "=========================================" << std::endl;
}

void readTaskHandler(int clientfdl){

}

void mainMenu(){

}