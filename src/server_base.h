#include <assert.h>
#include <cstdio>
#include <unordered_map>
#include <thread>

#include <uv.h>

namespace bchat {

class ServerBase {
 public:
  virtual ~ServerBase() {}

  virtual void OnNewConnection(int status) = 0;

  ServerBase(int port, int backlog, uv_loop_t *const loop)
      : port_(port), backlog_(backlog), loop_(loop) {
    socket_.data = this;

    uv_tcp_init(loop_, &socket_);
    uv_ip4_addr("0.0.0.0", port_, &addr_);
    uv_tcp_bind(&socket_, (const struct sockaddr *)&addr_, 0);
  }

  int Start() {
    int rc = uv_listen(stream(), backlog_, OnNewConnection);
    if (rc) {
      fprintf(stderr, "Listen error %s\n", uv_strerror(rc));
      return -1;
    }
    return uv_run(loop_, UV_RUN_DEFAULT);
  }

  static void OnNewConnection(uv_stream_t *server, int status) {
    reinterpret_cast<ServerBase *>(server->data)->OnNewConnection(status);
  }

 protected:
  uv_handle_t *handle() { return reinterpret_cast<uv_handle_t *>(&socket_); }
  uv_stream_t *stream() { return reinterpret_cast<uv_stream_t *>(&socket_); }

  int port_;
  int backlog_;

  uv_tcp_t socket_;
  uv_loop_t *const loop_;
  struct sockaddr_in addr_;
};
}
