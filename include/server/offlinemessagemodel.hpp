#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <string>
#include <vector>
//提供离线消息表的操作接口方法
//model类的目的是不让业务层直接操作sql语句或者mysql API的调用

class offlineMsgModel{
public:
    //存储用户的离线消息    
    void insert(int userid, std::string msg);
    
    //删除存储的用户离线消息
    void remove(int useid);    
    
    //查询用户的离线消息
    std::vector<std::string> query(int userid);
    
private:


};
    
#endif