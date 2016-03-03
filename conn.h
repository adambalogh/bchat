#pragma once

#include <assert.h>
#include <cstdio>
#include <iostream>

#include "uv.h"

#include "parser.h"

class Conn {
 private:
  class WriteReq {
   public:
    WriteReq(Conn *const conn, std::shared_ptr<std::vector<uint8_t>> msg)
        : conn_(conn), msg_(std::move(msg)) {
      assert((void *)this == &req_);

      const auto size = msg_->size();
      length_[0] = (size >> 24) & 0xFF;
      length_[1] = (size >> 16) & 0xFF;
      length_[2] = (size >> 8) & 0xFF;
      length_[3] = size & 0xFF;

      buf_[0].base = reinterpret_cast<char *>(length_.data());
      buf_[0].len = length_.size();

      buf_[1].base = reinterpret_cast<char *>(msg_->data());
      buf_[1].len = msg_->size();
    }

    Conn *conn() { return conn_; }
    uv_buf_t *bufs() { return buf_; }
    size_t buf_count() const { return 2; }
    uv_write_t *req() { return &req_; }

   private:
    uv_write_t req_;

    Conn *const conn_;

    const std::shared_ptr<std::vector<uint8_t>> msg_;

    std::array<uint8_t, 4> length_;

    uv_buf_t buf_[2];
  };

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
    buf->base = reinterpret_cast<char *>(buffer_.data());
    buf->len = buffer_.size();
  }

  void OnRead(ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
      if (nread != UV_EOF) {
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
      }
      Close();
    } else if (nread > 0) {
      parser_.Sink((uint8_t *)buf->base, nread);
      if (parser_.HasMessages()) {
        auto messages = parser_.GetMessages();
        for (auto &msg : messages) {
          Send(msg);
        }
      }
    }
  }

  void Send(const std::shared_ptr<std::vector<uint8_t>> &msg) {
    auto req = new WriteReq(this, msg);
    uv_write(req->req(), stream(), req->bufs(), req->buf_count(),
             OnWriteFinished);
  }

  void OnWriteFinished(WriteReq *req, int status) { delete req; }

 private:
  uv_handle_t *handle() { return reinterpret_cast<uv_handle_t *>(this); }
  uv_stream_t *stream() { return reinterpret_cast<uv_stream_t *>(this); }

  // socket must be the first member, so that we can cast the this pointer to
  // uv_handle_t*, a la C inheritance
  uv_tcp_t socket_;

  Parser parser_;

  std::array<uint8_t, 20000> buffer_;

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
