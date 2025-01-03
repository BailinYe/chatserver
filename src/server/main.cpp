#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <muduo/net/InetAddress.h>
#include <iostream>

//处理服务器ctrl-c退出后, 将用户全部强制下线
void resetHandler(int){
    ChatService::getChatService()->reset();
    exit(0);
}


int main(int argc, char** argv){

    if(argc < 3){
        std::cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << std::endl;
        std::exit(-1);
    }
    
    //解析通过命令行参数传递的ip和port
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resetHandler); //SIGINT - signal interrupt

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");
    
    server.Start();
    loop.loop();

    getchar();
    return 0;
}