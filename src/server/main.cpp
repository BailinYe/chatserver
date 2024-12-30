#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <muduo/net/InetAddress.h>

//处理服务器ctrl-c退出后, 将用户全部强制下线
void resetHandler(int){
    ChatService::getChatService()->reset();
    exit(0);
}


int main(){

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr("127.0.0.1", 8080);
    ChatServer server(&loop, addr, "ChatServer");
    
    server.Start();
    loop.loop();

    getchar();
    return 0;
}