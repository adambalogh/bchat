#include <assert.h>
#include <chrono>
#include <array>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <thread>

#include <uv.h>

#include "parser.h"

#define DEFAULT_PORT 7002
#define DEFAULT_BACKLOG 128

class Connection;

class WriteReq {
 public:
  WriteReq(Connection *const conn, uv_buf_t buf)
      : conn_(conn), buf_(std::move(buf)) {
    assert((void *)this == &req_);
  }

  ~WriteReq() { free(buf_.base); }

  Connection *conn() { return conn_; }
  uv_buf_t *buf() { return &buf_; }
  uv_write_t *req() { return &req_; }

 private:
  uv_write_t req_;
  Connection *const conn_;
  uv_buf_t buf_;
};

class Connection final {
 public:
  Connection(uv_loop_t *const loop) {
    uv_tcp_init(loop, &socket_);
    assert((void *)&socket_ == (void *)this);
  }

  ~Connection() {
    std::cout << "Deleted" << std::endl;
    exit(0);
  }

  void inline Close() { uv_close((uv_handle_t *)this, Delete); }

  void Start(uv_stream_t *const server) {
    if (uv_accept(server, (uv_stream_t *)this) == 0) {
      uv_read_start(stream(), AllocBuffer, OnRead);
    } else {
      Close();
    }
  }

  void AllocBuffer(size_t suggested_size, uv_buf_t *buf) {
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
  }

  void OnRead(ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
      if (nread != UV_EOF) {
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
      }
      Close();
    } else if (nread > 0) {
      count_ += nread;
      printf("Read: %d\n", count_);

      parser_.Sink((uint8_t *)buf->base, nread);
      while (parser_.HasNext()) {
        auto msg = parser_.GetNext();
        std::string msg_str{(char *)msg.data(), msg.size()};
        printf("%s\n", msg_str.c_str());
      }
    }
  }

  void OnWriteFinished(WriteReq *req, int status) { delete req; }

  // C-style function adapters for libuv

  static inline void Delete(uv_handle_t *h) {
    delete reinterpret_cast<Connection *>(h);
  }

  static inline void AllocBuffer(uv_handle_t *handle, size_t suggested_size,
                                 uv_buf_t *buf) {
    reinterpret_cast<Connection *>(handle)->AllocBuffer(suggested_size, buf);
  }

  static inline void OnRead(uv_stream_t *client, ssize_t nread,
                            const uv_buf_t *buf) {
    reinterpret_cast<Connection *>(client)->OnRead(nread, buf);
  }

  static inline void OnWriteFinished(uv_write_t *uv_req, int status) {
    WriteReq *req = reinterpret_cast<WriteReq *>(uv_req);
    req->conn()->OnWriteFinished(req, status);
  }

 private:
  uv_handle_t *handle() { return reinterpret_cast<uv_handle_t *>(this); }
  uv_stream_t *stream() { return reinterpret_cast<uv_stream_t *>(this); }

  // socket must be the first member, so that we can cast the this pointer to
  // uv_handle_t*, a la C inheritance
  uv_tcp_t socket_;

  Parser parser_;

  int count_{0};
};

class Server {
 public:
  Server(uv_loop_t *const loop) : loop_(loop) {
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
    auto *client = new Connection(loop_);
    client->Start(reinterpret_cast<uv_stream_t *>(&socket_));
  }

  static void OnNewConnection(uv_stream_t *server, int status) {
    reinterpret_cast<Server *>(server)->OnNewConnection(status);
  }

 private:
  uv_tcp_t socket_;
  uv_loop_t *const loop_;
  struct sockaddr_in addr_;
};

int main() {
  Server s{uv_default_loop()};
  s.Start();
  std::cout << uv_strerror(uv_loop_close(uv_default_loop())) << std::endl;
}
