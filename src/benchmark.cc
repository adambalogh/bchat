#include <benchmark/benchmark.h>

#include "parser.h"

static void Sink(benchmark::State& state) {
  Parser p;

  uint8_t size = 100;
  uint8_t buf[]{0, 0, 0, size};
  uint8_t msg[100];

  std::vector<Parser::MessagePtr> messages;

  while (state.KeepRunning()) {
    p.Sink(buf, 4);
    p.Sink(msg, size);
    p.HasMessages();
    messages = p.GetMessages();
  }
  assert(messages[0]->size() == size);
}

BENCHMARK(Sink);

BENCHMARK_MAIN()
