#include "chatserver.hpp"
#include <iostream>
#include <muduo/net/InetAddress.h>

int main(){
    EventLoop loop;
    InetAddress addr("127.0.0.1", 8080);
    ChatServer server(&loop, addr, "ChatServer");
    
    server.Start();
    loop.loop();

    getchar();
    return 0;
}