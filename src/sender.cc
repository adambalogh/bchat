#include "sender.h"

#include <assert.h>

#include "uv.h"

namespace bchat {

SenderImpl::SenderImpl(uv_stream_t *const stream) : stream_(stream) {}

SenderImpl::WriteReq::WriteReq(SenderImpl &sender, MessagePtr msg)
    : sender_(sender), msg_(std::move(msg)) {
  req_.data = this;
  assert(req_.data == (void *)this);

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

void SenderImpl::Send(MessagePtr msg) {
  auto req = new WriteReq(*this, std::move(msg));
  uv_write(req->req(), stream_, req->bufs(), req->buf_count(), OnWriteFinished);
}

void SenderImpl::OnWriteFinished(WriteReq *req, int status) {
  if (status < 0) {
    printf("Write error %s\n", uv_err_name(status));
  }
  delete req;
}
}
