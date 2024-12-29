#include "db.h"
#include <muduo/base/Logging.h>

// 数据库配置信息 //默认情况下mysql只监听127.0.0.1 
static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "Oskar030906";
static std::string dbname = "chat";

//大写MYSQL会有命名冲突
// 初始化数据库连接 并没有实际连接 只是分配空间
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        // C/C++默认编码为ASCII 设置中文
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }else{
        LOG_INFO << "connect mysql fail!";
    }
    return p;
}
// 更新操作
bool MySQL::update(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES *MySQL::query(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
//获取连接
MYSQL* MySQL::getConnection(){
    return _conn;
}