#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
using namespace muduo::net;
using namespace muduo;
#include <iostream>

class ChatServer {
 public:
  ChatServer(EventLoop *loop, const InetAddress &listen_addr, const std::string &name_arg);

  void start();

 private:
  // 处理用户的连接创建和断开 epoll listenfd accept
  void OnConnection(const TcpConnectionPtr &con);

  //处理用户读写事件
  void OnMessage(const TcpConnectionPtr &con, Buffer *buf, Timestamp t);

  TcpServer server_;

  EventLoop *loop_;  // epoll
};
#endif