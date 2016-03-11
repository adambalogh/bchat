#include <benchmark/benchmark.h>

#include "user.h"
#include "user.h"
#include "proto/message.pb.h"
#include "test_util.h"

class EmptyConn : public Conn {
 public:
  virtual void OnRead(ssize_t nread, const uv_buf_t* buf) {}
  virtual void Send(const MessagePtr msg) { benchmark::DoNotOptimize(msg); }
};

static void Message(benchmark::State& state) {
  EmptyConn conn;
  UserRepo repo;
  User u{conn, repo};

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");

  u.OnMessage(MakeMsg(req.SerializeAsString()));

  req.Clear();
  req.set_type(req.Message);
  req.mutable_message()->set_recipient("Bela");
  req.mutable_message()->set_content(
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus eget "
      "lectus nec arcu convallis tempus nec vel velit. Nunc eu libero dolor. "
      "Sed ultrices vehicula sem eu molestie. Aenean et tellus at tortor "
      "efficitur consectetur eu vitae dui. Quisque maximus lectus libero, "
      "vitae ornare libero pretium nec.");

  const auto serialized = req.SerializeAsString();

  while (state.KeepRunning()) {
    // TODO Dynamic memory allocation adds at least 50% to the benchmark times,
    // get rid of it somehow
    u.OnMessage(MakeMsg(serialized));
  }
}

BENCHMARK(Message);

BENCHMARK_MAIN()
