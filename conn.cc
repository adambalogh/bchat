#include "conn.h"

Conn::WriteReq::WriteReq(Conn *const conn, MessagePtr msg)
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

void inline Conn::Close() {
  user_.OnDisconnect();
  uv_close((uv_handle_t *)this, Delete);
}

void Conn::Start(uv_stream_t *const server) {
  if (uv_accept(server, (uv_stream_t *)this) == 0) {
    uv_read_start(stream(), AllocBuffer, OnRead);
  } else {
    Close();
  }
}

void Conn::AllocBuffer(size_t suggested_size, uv_buf_t *buf) {
  buf->base = reinterpret_cast<char *>(buffer_.data());
  buf->len = buffer_.size();
}

void Conn::OnRead(ssize_t nread, const uv_buf_t *buf) {
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
        user_.OnMessage(std::move(msg));
      }
    }
  }
}

void Conn::Send(MessagePtr msg) {
  auto req = new WriteReq(this, std::move(msg));
  uv_write(req->req(), stream(), req->bufs(), req->buf_count(),
           OnWriteFinished);
}

void Conn::OnWriteFinished(WriteReq *req, int status) { delete req; }
