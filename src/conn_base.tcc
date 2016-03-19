template <typename A>
template <typename... T>
ConnBase<A>::ConnBase(uv_loop_t *const loop, T &&... args)
    : sender_(stream()), app_(sender_, std::forward<T>(args)...) {
  socket_.data = this;
  assert(socket_.data == (void *)this);

  uv_tcp_init(loop, &socket_);
}

template <typename A>
void ConnBase<A>::Close() {
  app_.OnDisconnect();
  uv_close(handle(), Delete);
}

template <typename A>
void ConnBase<A>::Start(uv_stream_t *const server) {
  if (uv_accept(server, stream()) == 0) {
    uv_read_start(stream(), AllocBuffer, OnRead);
  } else {
    Close();
  }
  app_.OnConnect();
}

template <typename A>
void ConnBase<A>::AllocBuffer(size_t suggested_size, uv_buf_t *buf) {
  *buf = parser_.GetBuf();
}

template <typename A>
void ConnBase<A>::OnRead(ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
    }
    Close();
  } else if (nread > 0) {
    parser_.Sink(nread, [this](uv_buf_t buf) { app_.OnMessage(buf); });
  }
}
