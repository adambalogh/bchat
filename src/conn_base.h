#pragma once

#include <assert.h>
#include <cstdio>
#include <iostream>
#include <vector>

#include "uv.h"

#include "parser.h"
#include "sender.h"

namespace bchat {
namespace conn {

// ConnBase contains the communication protocol.
//
// Each message received is passed to A, which contains the application logic.
// A must implement the following interface:
//
//   class A {
//   public:
//    void OnConnect();
//    void OnDisconnect();
//    void OnMessage(uv_buf_t buf);
//  };
//
template <typename A>
class ConnBase {
 public:
  template <typename... T>
  ConnBase(uv_loop_t *const loop, T &&...);

  void Start(uv_stream_t *const server);

 private:
  void Close();

  void AllocBuffer(size_t suggested_size, uv_buf_t *buf);

  void OnRead(ssize_t nread, const uv_buf_t *buf);

  uv_handle_t *handle() { return reinterpret_cast<uv_handle_t *>(&socket_); }
  uv_stream_t *stream() { return reinterpret_cast<uv_stream_t *>(&socket_); }

 private:
  uv_tcp_t socket_;

  SenderImpl sender_;

  Parser parser_;

  // application logic
  A app_;

 public:
  // C-style functions for libuv

  static inline void Delete(uv_handle_t *handle) {
    delete reinterpret_cast<ConnBase *>(handle->data);
  }

  static inline void AllocBuffer(uv_handle_t *handle, size_t suggested_size,
                                 uv_buf_t *buf) {
    reinterpret_cast<ConnBase *>(handle->data)
        ->AllocBuffer(suggested_size, buf);
  }

  static inline void OnRead(uv_stream_t *client, ssize_t nread,
                            const uv_buf_t *buf) {
    reinterpret_cast<ConnBase *>(client->data)->OnRead(nread, buf);
  }
};

// Implementation
#include "conn_base.tcc"
}
}
