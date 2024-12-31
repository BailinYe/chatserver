#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>

//维护群组信息的接口方法
class GroupModel{
public:
    //创建群组
    bool createGroup(Group& group);
    //加入群组
    void addGroup(int groupid, int userid, std::string role);
    //查询用户所在群组信息 用户登录后显示群组信息
    std::vector<Group> queryGroups(int userid);
    //群聊消息 除了发送消息的用户自己 查询并返回他所在group的其它群成员用户id 
    std::vector<int> queryGroupUsers(int userid, int groupid);
};

#endif