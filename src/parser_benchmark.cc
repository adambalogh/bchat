#include <benchmark/benchmark.h>

#include "parser.h"
#include "test_util.h"

void handle(uv_buf_t msg) {}

static void Sink(benchmark::State& state) {
  Parser p;

  Arr length{0, 0, 0, 100};
  while (state.KeepRunning()) {
    SetBuf(p.GetBuf(), length);
    p.Sink(4, handle);
    p.Sink(100, handle);
  }
}

BENCHMARK(Sink);

BENCHMARK_MAIN()
