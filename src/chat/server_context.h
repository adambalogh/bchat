#pragma once

#include "chat/user.h"
#include "chat/message_storage.h"

namespace bchat {
namespace chat {

class User;

class UserRepo {
 public:
  bool Register(const std::string& name, User* user) {
    assert(user != nullptr);
    if (Contains(name)) {
      return false;
    }
    users_[name] = user;
    return true;
  }

  User& Get(const std::string& name) { return *users_.at(name); }

  bool Contains(const std::string& name) const {
    return users_.count(name) == 1;
  }

  void Remove(const std::string& name) {
    auto user = users_.find(name);
    if (user != users_.end()) {
      users_.erase(user);
    }
  }

 private:
  std::unordered_map<std::string, User*> users_;
};

class ServerContext {
 public:
  const UserRepo& online_users() const { return online_users_; }
  UserRepo& online_users() { return online_users_; }

 private:
  UserRepo online_users_;
};
}
}
