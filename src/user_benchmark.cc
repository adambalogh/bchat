#include <benchmark/benchmark.h>

#include "user.h"
#include "user.h"
#include "proto/message.pb.h"
#include "test_util.h"

static void Message(benchmark::State& state) {
  EmptyConn conn;
  UserRepo repo;
  User u{conn, repo};

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");

  auto bin = req.SerializeAsString();
  u.OnMessage(MakeBuf(bin));

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
  auto buf = MakeBuf(serialized);

  while (state.KeepRunning()) {
    u.OnMessage(buf);
  }
}

BENCHMARK(Message);

BENCHMARK_MAIN()
