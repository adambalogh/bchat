#pragma once

#include <assert.h>
#include <unordered_map>

#include "proto/message.pb.h"

class User;
class Conn;

class UserRepo {
 public:
  void RegisterUser(const std::string& name, User* user) {
    assert(user != nullptr);
    users_[name] = user;
  }

  User& GetUser(const std::string& name) { return *users_.at(name); }

  void RemoveUser(const std::string& name) {
    auto user = users_.find(name);
    if (user != users_.end()) {
      users_.erase(user);
    }
  }

 private:
  std::unordered_map<std::string, User*> users_;
};

// User contains the chat's application logic.
//
// Each User is owned by a unique Conn.
//
class User {
 public:
  User(Conn& conn, UserRepo& user_repo);

  void OnMessage(std::unique_ptr<std::vector<uint8_t>> msg);

  void SendMessage(proto::Message msg);

  void OnDisconnect();

 private:
  const std::string name_;

  Conn& conn_;

  UserRepo& user_repo_;

  static int count;
};
