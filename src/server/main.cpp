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

int main(int argc, char **argv) {
  if (argc < 3) {
    cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
    exit(-1);
  }

  // 解析通过命令行参数传递的ip和port
  char *ip = argv[1];
  uint16_t port = atoi(argv[2]);

  signal(SIGINT, resetHandler);

  EventLoop loop;
  InetAddress addr(ip, port);
  ChatServer server(&loop, addr, "ChatServer");

  server.start();
  loop.loop();
}