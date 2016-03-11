#pragma once

#include <assert.h>
#include <cstdio>
#include <iostream>

#include "uv.h"

#include "parser.h"
#include "user.h"

class Conn {
 public:
  typedef Parser::Message Message;
  typedef Parser::MessagePtr MessagePtr;

  virtual ~Conn() {}

  virtual void OnRead(ssize_t nread, const uv_buf_t *buf) = 0;
  virtual void Send(const MessagePtr msg) = 0;
};

class ConnImpl : public Conn {
 private:
  class WriteReq {
   public:
    WriteReq(ConnImpl *const conn, MessagePtr msg);

    ConnImpl *conn() { return conn_; }
    uv_buf_t *bufs() { return buf_; }
    size_t buf_count() const { return 2; }
    uv_write_t *req() { return &req_; }

   private:
    uv_write_t req_;
    ConnImpl *const conn_;
    const MessagePtr msg_;
    std::array<uint8_t, 4> length_;
    uv_buf_t buf_[2];
  };

 public:
  ConnImpl(uv_loop_t *const loop, UserRepo &user_repo)
      : user_(*this, user_repo) {
    socket_.data = this;
    assert(socket_.data == (void *)this);

    uv_tcp_init(loop, &socket_);
  }

  void inline Close();

  void Start(uv_stream_t *const server);

  void AllocBuffer(size_t suggested_size, uv_buf_t *buf);

  void OnRead(ssize_t nread, const uv_buf_t *buf) override;

  void Send(const MessagePtr msg) override;

  void OnWriteFinished(WriteReq *req, int status);

 private:
  uv_handle_t *handle() { return reinterpret_cast<uv_handle_t *>(&socket_); }
  uv_stream_t *stream() { return reinterpret_cast<uv_stream_t *>(&socket_); }

  uv_tcp_t socket_;

  Parser parser_;

  User user_;

  // C-style function adapters for libuv

 public:
  static inline void Delete(uv_handle_t *handle) {
    delete reinterpret_cast<ConnImpl *>(handle->data);
  }

  static inline void AllocBuffer(uv_handle_t *handle, size_t suggested_size,
                                 uv_buf_t *buf) {
    reinterpret_cast<ConnImpl *>(handle->data)
        ->AllocBuffer(suggested_size, buf);
  }

  static inline void OnRead(uv_stream_t *client, ssize_t nread,
                            const uv_buf_t *buf) {
    reinterpret_cast<ConnImpl *>(client->data)->OnRead(nread, buf);
  }

  static inline void OnWriteFinished(uv_write_t *uv_req, int status) {
    WriteReq *req = reinterpret_cast<WriteReq *>(uv_req);
    req->conn()->OnWriteFinished(req, status);
  }
};
