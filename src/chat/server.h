#pragma once

#include <assert.h>
#include <cstdio>
#include <unordered_map>
#include <thread>

#include <uv.h>

#include "server_base.h"
#include "conn_base.h"
#include "chat/user.h"

#define DEFAULT_PORT 7002
#define DEFAULT_BACKLOG 128

namespace bchat {
namespace chat {

class Server : public ServerBase {
 public:
  Server(uv_loop_t *const loop)
      : ServerBase(DEFAULT_PORT, DEFAULT_BACKLOG, loop) {}

  void OnNewConnection(int status) override {
    if (status < 0) {
      fprintf(stderr, "New connection error %s\n", uv_strerror(status));
      return;
    }
    auto *conn = new conn::ConnBase<User>(loop_, users_);
    conn->Start(reinterpret_cast<uv_stream_t *>(&socket_));
  }

 private:
  UserRepo users_;
};
}
}
