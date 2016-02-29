#pragma once

#include <assert.h>
#include <cstdio>
#include <iostream>

#include "uv.h"

#include "parser.h"

class Conn;

class WriteReq {
 public:
  WriteReq(Conn *const conn, uv_buf_t buf) : conn_(conn), buf_(std::move(buf)) {
    assert((void *)this == &req_);
  }

  ~WriteReq() { free(buf_.base); }

  Conn *conn() { return conn_; }
  uv_buf_t *buf() { return &buf_; }
  uv_write_t *req() { return &req_; }

 private:
  uv_write_t req_;
  Conn *const conn_;
  uv_buf_t buf_;
};

class Conn final {
 public:
  Conn(uv_loop_t *const loop) {
    assert((void *)&socket_ == (void *)this);
    uv_tcp_init(loop, &socket_);
  }

  ~Conn() { std::cout << "Deleted" << std::endl; }

  void inline Close() { uv_close((uv_handle_t *)this, Delete); }

  void Start(uv_stream_t *const server) {
    if (uv_accept(server, (uv_stream_t *)this) == 0) {
      uv_read_start(stream(), AllocBuffer, OnRead);
    } else {
      Close();
    }
  }

  void AllocBuffer(size_t suggested_size, uv_buf_t *buf) {
    buf->base = new char[2000];
    buf->len = 2000;
  }

  void OnRead(ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
      if (nread != UV_EOF) {
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
      }
      Close();
    } else if (nread > 0) {
      // TODO if nread is << buf->len, we are wasting a lot of space in parser
      parser_.Sink((uint8_t *)buf->base, nread);
      while (parser_.HasNext()) {
        auto msg = parser_.GetNext();
        std::string msg_str{(char *)msg.data(), msg.size()};
        printf("%s\n", msg_str.c_str());
      }
    }
  }

  void OnWriteFinished(WriteReq *req, int status) { delete req; }

 private:
  uv_handle_t *handle() { return reinterpret_cast<uv_handle_t *>(this); }
  uv_stream_t *stream() { return reinterpret_cast<uv_stream_t *>(this); }

  // socket must be the first member, so that we can cast the this pointer to
  // uv_handle_t*, a la C inheritance
  uv_tcp_t socket_;

  Parser parser_;

  // C-style function adapters for libuv

 public:
  static inline void Delete(uv_handle_t *h) {
    delete reinterpret_cast<Conn *>(h);
  }

  static inline void AllocBuffer(uv_handle_t *handle, size_t suggested_size,
                                 uv_buf_t *buf) {
    reinterpret_cast<Conn *>(handle)->AllocBuffer(suggested_size, buf);
  }

  static inline void OnRead(uv_stream_t *client, ssize_t nread,
                            const uv_buf_t *buf) {
    reinterpret_cast<Conn *>(client)->OnRead(nread, buf);
  }

  static inline void OnWriteFinished(uv_write_t *uv_req, int status) {
    WriteReq *req = reinterpret_cast<WriteReq *>(uv_req);
    req->conn()->OnWriteFinished(req, status);
  }
};
