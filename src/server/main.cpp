#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <signal.h>
#include "../../include/server/chatServer.h"
#include "../../include/server/chatService.h"

// 处理服务器 crtl + c结束后,重置usr状态信息
void resetHandler(int) {
  ChatService::GetInstance()->reset();
  exit(0);
}

int main() {
  signal(SIGINT, resetHandler);
  EventLoop loop;
  InetAddress addr("127.0.0.1", 6000);
  ChatServer server(&loop, addr, "ChatServer");
  server.start();
  loop.loop();
  return 0;
}