#include "groupmodel.hpp"
#include "groupuser.hpp"
#include "db.h"
#include <mysql/mysql.h>

//通过封装对业务层所需的数据进行处理 并以对象的形式进行返回
//提供给业务层对象信息 比如Group User 而不是字段信息 或者sql语句

//创建群组
bool GroupModel::createGroup(Group& group){
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s','%s')", 
    group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            //创建群组后 将group的id设为将返回的群组id
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

//加入群组
void GroupModel::addGroup(int groupid, int userid, std::string role){
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser(groupid, userid, grouprole) values('%d', '%d','%s')", 
    groupid, userid, role.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }

}

//查询用户所在群组信息和群组成员信息 通过封装提供业务层所需的所有数据
std::vector<Group> GroupModel::queryGroups(int userid){
    /*
     * 1.根据userid在groupuser表中查询该用户所在的全部群组
    */
    
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join \
            groupuser b on a.id = b.groupid where b.userid = '%d'", userid);
    
    std::vector<Group> groups;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
                MYSQL_ROW row;
                //查出userid所有的群组信息
                while((row = mysql_fetch_row(res)) != nullptr){
                    Group group;
                    group.setId(atoi(row[0]));
                    group.setName(row[1]);
                    group.setDesc(row[2]);
                    groups.push_back(group);
                }
                mysql_free_result(res);
        }
    }
    
    //查询所有群组的用户信息
    for(Group& group : groups){
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a inner join \
        groupuser b on b.userid = a.userid where b.groupid = '%d'", group.getId());

        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user); //groups并不真正存储用户信息 角色信息在函数运行后才从数据库中返回 类表示结构 动态获取
            }
            mysql_free_result(res);
        }
    }
    return groups;
    //返回包含了该用户所在的所有群组信息和群组的所有成员信息
}

//返回指定群聊的除发送者之外的所有用户id
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid){
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = '%d' and userid != '%d'", groupid, userid);

    std::vector<int> ids;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                ids.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return ids;
}