#include "offlinemessagemodel.hpp"
#include "db.h"
#include <cstdio>
#include <mysql/mysql.h>

void offlineMsgModel::insert(int userid, std::string msg){
    //1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage(userid, message) values('%d', '%s')", userid, msg.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

std::vector<std::string> offlineMsgModel::query(int userid){
    //sql语句
    char sql[1024] = {0}; 
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    //MySQL对象
    MySQL mysql;
    std::vector<std::string> rows;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            //把userid用户所有的离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                rows.push_back(row[0]);
            }            
            mysql_free_result(res);
        }
    }
    return rows;
}

void offlineMsgModel::remove(int userid){
    //1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = '%d'", userid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}