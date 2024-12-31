#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"
#include <string>

//群组用户类, 多了角色信息, 从User类继承属性
class GroupUser : public User{
public:
    void setRole(std::string role){this->role = role;}
    std::string getRole(){return this->role;}

private:
    std::string role;
    //通过公共继承set方法进行私有变量的设置
};



#endif