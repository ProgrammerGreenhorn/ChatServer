#include "../../include/server/chatService.h"
#include <muduo/base/Logging.h>
#include <mutex>
#include <vector>
#include "../../include/public.h"
using namespace muduo;
ChatService *ChatService::GetInstance() {
  static ChatService service;
  return &service;
}

// 注册消息对应的回调函数
ChatService::ChatService() {
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::LOGIN_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { Login(con, js, t); });
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::REG_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { Reg(con, js, t); });
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::LOGINOUT_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { LoginOut(con, js, t); });
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::ONE_CHAT_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { OneChat(con, js, t); });
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::ADD_FRIEND_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { AddFriend(con, js, t); });
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::CREATE_GROUP_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { CreateGroup(con, js, t); });
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::ADD_GROUP_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { AddGroup(con, js, t); });
  msg_handler_map_.emplace(static_cast<int>(EnMsgType::GROUP_CHAT_MSG),
                           [this](const TcpConnectionPtr &con, json js, Timestamp t) { GroupChat(con, js, t); });
}

MsgHandler ChatService::GetHandler(int msg_id) {
  auto it = msg_handler_map_.find(msg_id);
  if (it != msg_handler_map_.end()) {
    return it->second;
  }
  // 如果没有找到对应的处理器，返回一个默认的处理器
  // 这个处理器会打印错误信息
  return [msg_id](const TcpConnectionPtr &con, json js, Timestamp t) {
    LOG_ERROR << "msg id : " << msg_id << " can not find handle";
  };
}

void ChatService::ClientCloseException(const TcpConnectionPtr &con) {
  User user;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto &[k, v] : usr_connections_) {
      if (v == con) {
        user = user_model_.query(k);
        // 删除连接信息
        usr_connections_.erase(k);
        break;
      }
    }
  }

  // 更新状态
  if (user.getId() != -1) {
    user.setState("offline");
    user_model_.updateState(user);
  }
}

void ChatService::reset() { user_model_.resetState(); }

// /ORM 业务操作的都是对象 DAO，而不用操作sql语句
// 登录 登录业务
void ChatService::Login(const TcpConnectionPtr &con, json js, Timestamp t) {
  int id = js["id"].get<int>();
  string pwd = js["password"];
  User user = user_model_.query(id);
  if (user.getId() == id && user.getPwd() == pwd) {
    if (user.getState() == "online") {
      json response;
      response["msgid"] = static_cast<int>(EnMsgType::LOGIN_MSG_ACK);
      response["errno"] = 2;
      response["errmsg"] = "the account is using, input another";
      con->send(response.dump());
    } else {
      // 登录成功 更新状态
      user.setState("online");

      // 记录连接信息 互斥
      {
        std::unique_lock<std::mutex> lock(mutex_);
        usr_connections_[id] = con;
      }

      user_model_.updateState(user);
      json response;
      response["msgid"] = static_cast<int>(EnMsgType::LOGIN_MSG_ACK);
      response["errno"] = 0;
      response["id"] = user.getId();
      response["name"] = user.getName();
      // 查询是否有离线消息
      auto offline_msg = offline_msg_model_.query(id);
      if (!offline_msg.empty()) {
        response["offlinemsg"] = offline_msg;
        // 移除
        offline_msg_model_.remove(id);
      }

      // 返回好友信息
      auto usr_friend = friend_model_.query(id);
      if (!usr_friend.empty()) {
        vector<string> vec2;
        for (auto &f : usr_friend) {
          json js;
          js["id"] = f.getId();
          js["name"] = f.getName();
          js["state"] = f.getState();
          vec2.push_back((js.dump()));
        }
        response["friends"] = vec2;
      }

      // 返回群组信息
      auto group_vec = group_model_.queryGroups(id);
      if (!group_vec.empty()) {
        // group : [{groupid : [xx,xx,xx]}]
        vector<string> group_v;
        for (auto &group : group_vec) {
          json grp_json;
          grp_json["id"] = group.getId();
          grp_json["groupname"] = group.getName();
          grp_json["groupdesc"] = group.getDesc();
          vector<string> user_v;
          for (auto &user : group.getUsers()) {
            json user_json;
            user_json["id"] = user.getId();
            user_json["name"] = user.getName();
            user_json["state"] = user.getState();
            user_json["role"] = user.getRole();
            user_v.push_back(user_json.dump());
          }
          grp_json["users"] = user_v;
          group_v.push_back(grp_json.dump());
        }
        response["groups"] = group_v;
      }
      con->send(response.dump());
    }
  } else {
    json response;
    response["msgid"] = static_cast<int>(EnMsgType::LOGIN_MSG_ACK);
    response["errno"] = 1;
    response["errmsg"] = "worng usr or password";
    con->send(response.dump());
  }
}

// 注册 name password，返回id
void ChatService::Reg(const TcpConnectionPtr &con, json js, Timestamp t) {
  string name = js["name"];
  string pwd = js["password"];
  User user;
  user.setName(name);
  user.setPwd(pwd);
  bool state = user_model_.insert(user);
  if (state) {
    // 注册成功
    json response;
    response["msgid"] = static_cast<int>(EnMsgType::REG_MSG_ACK);
    response["errno"] = 0;
    response["id"] = user.getId();
    con->send(response.dump());
  } else {
    json response;
    response["msgid"] = static_cast<int>(EnMsgType::REG_MSG_ACK);
    response["errno"] = 0;
    response["id"] = user.getId();
    con->send(response.dump());
  }
}

void ChatService::OneChat(const TcpConnectionPtr &con, json js, Timestamp t) {
  int toid = js["toid"].get<int>();
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (usr_connections_.count(toid) != 0U) {
      // 在线 转发消息给toid用户
      usr_connections_[toid]->send(js.dump());
      return;
    }
  }
  // 存储离线信息
  offline_msg_model_.insert(toid, js.dump());
}

// 加好友
void ChatService::AddFriend(const TcpConnectionPtr &con, json js, Timestamp t) {
  int id = js["id"].get<int>();
  int friend_id = js["friendid"].get<int>();
  friend_model_.insert(id, friend_id);
}

// 创建群组
void ChatService::CreateGroup(const TcpConnectionPtr &con, json js, Timestamp t) {
  int usr_id = js["id"].get<int>();
  string name = js["groupname"];
  string desc = js["groupdesc"];
  Group group(-1, name, desc);
  if (group_model_.createGroup(group)) {
    // 存入创建者信息
    group_model_.addGroup(usr_id, group.getId(), "creator");
  }
}

// 加入群组
void ChatService::AddGroup(const TcpConnectionPtr &con, json js, Timestamp t) {
  int usr_id = js["id"].get<int>();
  int group_id = js["groupid"].get<int>();
  group_model_.addGroup(usr_id, group_id, "normal");
}

// 群聊天
void ChatService::GroupChat(const TcpConnectionPtr &con, json js, Timestamp t) {
  int usr_id = js["id"].get<int>();
  int group_id = js["groupid"].get<int>();
  vector<int> usr_id_vec = group_model_.queryGroupUsers(usr_id, group_id);
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto id : usr_id_vec) {
    if (usr_connections_.count(id) != 0) {
      // 转发群消息
      usr_connections_[id]->send(js.dump());
    } else {
      // 存储离线消息
      offline_msg_model_.insert(id, js.dump());
    }
  }
}

// 下线
void ChatService::LoginOut(const TcpConnectionPtr &con, json js, Timestamp t) {
  int id = js["id"].get<int>();
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (usr_connections_.count(id) != 0) {
      usr_connections_.erase(id);
    }
  }
  User user;
  user.setId(id);
  user.setState("offline");
  user_model_.updateState(user);

  json response;
  response["msgid"] = static_cast<int>(EnMsgType::LOGINOUT_MSG);
  con->send(response.dump());
}