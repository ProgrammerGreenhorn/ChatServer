#ifndef CHAT_SERVICE_H
#define CHAT_SERVICE_H
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <functional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <unordered_map>
#include "./model/offlineMessageModel.h"
#include "./model/userModel.h"
#include "model/friendModel.h"
#include "model/groupModel.h"
using namespace muduo::net;
using namespace muduo;
using json = nlohmann::json;
// msg id对应的处理事件回调
using MsgHandler = std::function<void(const TcpConnectionPtr &, json, Timestamp)>;
// 聊天服务器业务类
class ChatService {
 public:
  //单例模式
  static ChatService *GetInstance();

  // 登录
  void Login(const TcpConnectionPtr &con, json js, Timestamp t);

  // 注册
  void Reg(const TcpConnectionPtr &con, json js, Timestamp t);

  // 一对一聊天
  void OneChat(const TcpConnectionPtr &con, json js, Timestamp t);

  // 添加好友业务
  void AddFriend(const TcpConnectionPtr &con, json js, Timestamp t);

  // 创建群组
  void CreateGroup(const TcpConnectionPtr &con, json js, Timestamp t);

  // 加入群组
  void AddGroup(const TcpConnectionPtr &con, json js, Timestamp t);

  // 群组聊天
  void GroupChat(const TcpConnectionPtr &con, json js, Timestamp t);

  void LoginOut(const TcpConnectionPtr &con, json js, Timestamp t);

  // 获取消息对应的处理器
  MsgHandler GetHandler(int msg_id);

  // 处理客户端异常退出
  void ClientCloseException(const TcpConnectionPtr &con);

  // 服务器异常，业务重置
  void reset();

 private:
  ChatService();

  // 消息id对应的业务处理方法
  std::unordered_map<int, MsgHandler> msg_handler_map_;

  // 保护usr_connections的互斥锁
  std::mutex mutex_;

  // 存储在线用户的tcp连接
  std::unordered_map<int, TcpConnectionPtr> usr_connections_;

  /* 数据操作类对象 */
  UserModel user_model_;

  OfflineMsgModel offline_msg_model_;

  FriendModel friend_model_;

  GroupModel group_model_;
  /* end of 数据操作类对象 */
};

#endif