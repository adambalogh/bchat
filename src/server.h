#include <assert.h>
#include <cstdio>
#include <unordered_map>
#include <thread>

#include <uv.h>

#include "conn.h"

#define DEFAULT_PORT 7002
#define DEFAULT_BACKLOG 128

class Server {
 public:
  Server(uv_loop_t *const loop) : loop_(loop) {
    assert((void *)this == &socket_);

    uv_tcp_init(loop_, &socket_);
    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr_);
    uv_tcp_bind(&socket_, (const struct sockaddr *)&addr_, 0);
  }

  int Start() {
    int r = uv_listen((uv_stream_t *)this, DEFAULT_BACKLOG, OnNewConnection);
    if (r) {
      fprintf(stderr, "Listen error %s\n", uv_strerror(r));
      return -1;
    }
    return uv_run(loop_, UV_RUN_DEFAULT);
  }

  void OnNewConnection(int status) {
    if (status < 0) {
      fprintf(stderr, "New connection error %s\n", uv_strerror(status));
      return;
    }
    auto *client = new Conn(loop_, users_);
    client->Start(reinterpret_cast<uv_stream_t *>(&socket_));
  }

  static void OnNewConnection(uv_stream_t *server, int status) {
    reinterpret_cast<Server *>(server)->OnNewConnection(status);
  }

 private:
  uv_tcp_t socket_;
  uv_loop_t *const loop_;
  struct sockaddr_in addr_;

  UserRepo users_;
};
