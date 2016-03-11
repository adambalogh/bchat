#include "Conn.h"

ConnImpl::WriteReq::WriteReq(ConnImpl *const ConnImpl, MessagePtr msg)
    : conn_(ConnImpl), msg_(std::move(msg)) {
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

void inline ConnImpl::Close() {
  user_.OnDisconnect();
  uv_close(handle(), Delete);
}

void ConnImpl::Start(uv_stream_t *const server) {
  if (uv_accept(server, stream()) == 0) {
    uv_read_start(stream(), AllocBuffer, OnRead);
  } else {
    Close();
  }
}

void ConnImpl::AllocBuffer(size_t suggested_size, uv_buf_t *buf) {
  *buf = parser_.GetBuf();
}

void ConnImpl::OnRead(ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
    }
    Close();
  } else if (nread > 0) {
    parser_.Sink(nread, [this](uv_buf_t buf) { user_.OnMessage(buf); });
  }
}

void ConnImpl::Send(MessagePtr msg) {
  auto req = new WriteReq(this, std::move(msg));
  uv_write(req->req(), stream(), req->bufs(), req->buf_count(),
           OnWriteFinished);
}

void ConnImpl::OnWriteFinished(WriteReq *req, int status) { delete req; }
