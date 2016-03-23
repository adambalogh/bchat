#pragma once

#include <assert.h>
#include <cstdio>
#include <unordered_map>
#include <thread>

#include <uv.h>

#include "server_base.h"
#include "conn_base.h"
#include "chat/server_context.h"
#include "chat/user.h"

#define DEFAULT_BACKLOG 128

namespace bchat {
namespace chat {

class Server : public ServerBase {
 public:
  Server(uv_loop_t *const loop, int port)
      : ServerBase(port, DEFAULT_BACKLOG, loop) {}

  void OnNewConnection(int status) override {
    if (status < 0) {
      fprintf(stderr, "New connection error %s\n", uv_strerror(status));
      return;
    }
    auto *conn = new conn::ConnBase<User>(loop_, context_);
    conn->Start(reinterpret_cast<uv_stream_t *>(&socket_));
  }

 private:
  ServerContext context_;
};
}
}
