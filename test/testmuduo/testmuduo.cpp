#include <muduo/base/Timestamp.h>
#include <iostream>

/*
Muduo:
TcpServer : 服务器程序
TcpClient : 客户端程序

区分网络 IO 和 业务代码区分
              我们只需要关注：用户的连接和断开 用户的可读写事件
*/
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>

using namespace muduo::net;
using namespace muduo;

/*
1.组合tcp对象
2.创建 eventloop指针
3.明确构造函数，如何构造ChatServer
4.服务器类构造函数内，注册应该的回调事件
5.设置线程数量
*/
class ChatServer {
 public:
  ChatServer(EventLoop *loop, const InetAddress &listen_addr, const std::string &name_arg)
      : server_(loop, listen_addr, name_arg), loop_(loop) {
    // 给服务器注册用户连接和断开回调
    server_.setConnectionCallback([this](const TcpConnectionPtr &ptr) { OnConnect(ptr); });

    // 给服务器注册读写事件回调
    server_.setMessageCallback(
        [this](const TcpConnectionPtr &ptr, Buffer *buf, Timestamp t) { OnMessage(ptr, buf, t); });

    // 设置线程数量 1个 io线程 ，三个 worker线程
    server_.setThreadNum(10);
  }

  // 开启事件循环
  void start() { server_.start(); }

 private:
  // 处理用户的连接创建和断开 epoll listenfd accept
  void OnConnect(const TcpConnectionPtr &con) {
    if (con->connected()) {
      std::cout << con->peerAddress().toIpPort() << "->" << con->localAddress().toIpPort() << " state: online"
                << "\n";
    } else {
      std::cout << con->peerAddress().toIpPort() << "->" << con->localAddress().toIpPort() << " state: offline"
                << "\n";
      con->shutdown();  // close(fd)
    }
  }

  //处理用户读写事件
  void OnMessage(const TcpConnectionPtr &con, Buffer *buf, Timestamp t) {
    string s_buf = buf->retrieveAllAsString();
    std::cout << "recv data : " << s_buf << " time : " << t.toString() << "\n";
    con->send(s_buf);
  }
  TcpServer server_;
  EventLoop *loop_;  // epoll
};
int main() {
  EventLoop loop;
  InetAddress addr("127.0.0.1", 6000);
  ChatServer server(&loop, addr, "Server");
  server.start();  // listen
  loop.loop();     // epoll_wait 以阻塞方式等待新用户连接
  return 0;
}
