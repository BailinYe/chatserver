#include "usermodel.hpp"
#include "user.hpp"
#include <cstddef>
#include <mysql/mysql.h>
#include "db.h"
#include <iostream>
#include <string>

//分离数据库层和业务层的ORM层model 涉及相关的数据库操作和MYSQL API调用
//User表添加user的方法
bool UserModel::insert(User& user){
    //sql语句
    char sql[1024] = {0}; 
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    //MySQL对象
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            //获取成功插入的用户生成的的主键数据ID
            //id即为用户号
            user.setId(mysql_insert_id(mysql.getConnection()));
            
            return true; 
        }
        std::cout << "didn't update";
    }
    
    std::cout << "didn't connect";
    return false;
}

//根据用户id查询用户信息
User UserModel::query(int id){
    //sql语句
    char sql[1024] = {0}; 
    sprintf(sql, "select * from user where id = %d", id);
    //MySQL对象
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row = mysql_fetch_row(res); 
            if(row != nullptr){
               User user;
               user.setId(std::atoi(row[0])); 
               user.setName(row[1]);
               user.setPassword(row[2]);
               user.setState(row[3]);
               mysql_free_result(res);
               return user;
            }
        }
    }
    return User();
}

//更新用户状态信息
bool UserModel::updateState(User user){
    //sql语句
    char sql[1024] = {0}; 
    sprintf(sql, "update user set state = '%s' where id = '%d'",
            user.getState().c_str(), user.getId());
    //MySQL对象
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return true;
        }
    }
    return false;
}

void UserModel::resetState(){
    //sql语句
    char sql[1024] = {"update user set state = 'offline' where state = 'online'"};
    //MySQL对象
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}