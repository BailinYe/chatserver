#if !defined(PUBLIC_H)
#define PUBLIC_H

/*
 * server 和 client 的公共头文件
*/

enum EnMsgType{
    LOGIN_MSG = 1, //如果收到的messge是LOGIN_MSG类型 会自动调用login方法
    LOGIN_MSG_ACK, //登录响应消息
    REG_MSG, //如果收到的messge是REG_MSG类型 会自动调用reg方法
    REG_MSG_ACK, //注册确认响应消息
    ONE_CHAT_MSG, //聊天消息
    ADD_FRIEND_MSG, //添加好友消息
                    
    CREATE_GROUP_MSG, //创建群组
    ADD_GROUP_MSG, //加入群组
    GROUP_CHAT_MSG, //群聊天
    
    LOGOUT_MSG, //注销消息
};

#endif // PUBLIC_H
