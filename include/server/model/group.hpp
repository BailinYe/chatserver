#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>

class Group{
public:
    Group(int id = -1, std::string name = "", std::string desc = "")
        :id{id}, name{name}, desc{desc}
    {
        
    }
    Group(const Group& group)
        :id{group.id}, name{group.name}, desc{group.desc}, users{group.users}
    {

    }
    
    void setId(int id){this->id = id;}
    void setName(std::string name){this->name = name;}
    void setDesc(std::string desc){this->desc = desc;}

    int getId(){return this->id;}
    std::string getName(){return this->name;}
    std::string getDesc(){return this->desc;}
    std::vector<GroupUser>& getUsers(){return this->users;}

private:
    int id;
    std::string name;
    std::string desc;
    std::vector<GroupUser> users; //存储组内成员 给业务层使用

};

#endif