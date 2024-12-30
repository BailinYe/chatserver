#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include <vector>

//维护好友信息操作的接口方法
class FriendModel{
public:
    //添加好友关系
    void insert(int userid, int friendid);
    //实际业务好友列表数据可能保存在客户端 需要在上次下线时将更新的数据存在用户本地
    //返回用户好友列表 
    std::vector<User> query(int userid);

};

#endif