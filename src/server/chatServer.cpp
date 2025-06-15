#include "../../include/server/chatServer.h"
#include <muduo/base/Logging.h>
#include <exception>
#include <nlohmann/json.hpp>
#include <string>
#include "../../include/server/chatService.h"

using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listen_addr, const std::string &name_arg)
    : server_(loop, listen_addr, name_arg), loop_(loop) {
  // 给服务器注册用户连接和断开回调
  // 发生事件时，muduo调用我们注册的这个回调，并把封装好的ptr传过来
  server_.setConnectionCallback([this](const TcpConnectionPtr &ptr) { OnConnection(ptr); });

  // 给服务器注册读写事件回调
  server_.setMessageCallback([this](const TcpConnectionPtr &ptr, Buffer *buf, Timestamp t) { OnMessage(ptr, buf, t); });

  // 设置线程数量 1个 io线程 ，三个 worker线程
  server_.setThreadNum(10);
}

// 开启事件循环
void ChatServer::start() { server_.start(); }

// 处理用户的连接创建和断开 epoll listenfd accept
void ChatServer::OnConnection(const TcpConnectionPtr &con) {
  if (!con->connected()) {
    LOG_INFO << con->peerAddress().toIpPort() << " state offline";
    ChatService::GetInstance()->ClientCloseException(con);
    con->shutdown();
  }
}

// 处理用户读写事件
void ChatServer::OnMessage(const TcpConnectionPtr &con, Buffer *buf, Timestamp t) {
  string s_buf = buf->retrieveAllAsString();
  // 反序列化
  try {
    json js = json::parse(s_buf);
    // 通过 js["msgid"] 获取业务处理器handler -> con js time
    // 达到目的：解耦网络模块的代码和业务模块的代码
    auto chat_service = ChatService::GetInstance();
    // 要转换int
    auto msg_handler = chat_service->GetHandler(js["msgid"].get<int>());
    // 调用消息绑定的事件处理器
    msg_handler(con, js, t);
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();
  }
}