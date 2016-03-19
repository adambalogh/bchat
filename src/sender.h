#pragma once

#include <array>
#include <inttypes.h>
#include <memory>
#include <vector>

#include "uv.h"

namespace bchat {

typedef std::vector<uint8_t> Message;
typedef std::unique_ptr<Message> MessagePtr;

class Sender {
 public:
  virtual ~Sender() {}

  virtual void Send(MessagePtr msg) = 0;
};

class SenderImpl : public Sender {
 private:
  class WriteReq {
   public:
    WriteReq(SenderImpl &sender, MessagePtr msg);

    SenderImpl *sender() { return &sender_; }
    uv_buf_t *bufs() { return buf_; }
    size_t buf_count() const { return 2; }
    uv_write_t *req() { return &req_; }

   private:
    uv_write_t req_;
    SenderImpl &sender_;
    const MessagePtr msg_;
    std::array<uint8_t, 4> length_;
    uv_buf_t buf_[2];
  };

 public:
  SenderImpl(uv_stream_t *stream);

  void Send(MessagePtr msg) override;

  void OnWriteFinished(WriteReq *req, int status);

 private:
  uv_stream_t *const stream_;

  static inline void OnWriteFinished(uv_write_t *uv_req, int status) {
    WriteReq *req = reinterpret_cast<WriteReq *>(uv_req);
    req->sender()->OnWriteFinished(req, status);
  }
};
}
