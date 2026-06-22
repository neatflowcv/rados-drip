#include <benchmark/benchmark.h>

#include "simple/simple.h"

namespace {

constexpr int kRightHandSide = 7;

void BM_Add(benchmark::State& state) {
  const auto lhs = static_cast<int>(state.range(0));
  for ([[maybe_unused]] auto iteration : state) {
    benchmark::DoNotOptimize(simple::Add(lhs, kRightHandSide));
  }
}

// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
BENCHMARK(BM_Add)->Arg(1)->Arg(1024);

}  // namespace
