#include "friendmodel.hpp"
#include "db.h"
// 添加好友关系
void FriendModel::insert(int userid, int friendid){
    //sql语句
    char sql[1024] = {0}; 
    sprintf(sql, "insert into friend(userid, friendid) values('%d', '%d')", userid, friendid);
    //MySQL对象
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
// 实际业务好友列表数据可能保存在客户端 需要在上次下线时将更新的数据存在用户本地
// 返回用户好友列表
std::vector<User> FriendModel::query(int userid){
    //sql语句
    char sql[1024] = {0}; 
    //在friend和user表中进行联合查询 指定user的friends的详细信息
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = '%d'", userid);

    //MySQL对象
    MySQL mysql;
    std::vector<User> friends;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            //把userid用户所有的离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                friends.push_back(user);
            }            
            mysql_free_result(res);
        }
    }
    return friends;
}