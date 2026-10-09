#include <benchmark/benchmark.h>

#define BENCHMARK_LOGS 1
#include "LogBenchmark.h"

#define BENCHMARK_EVENTS 0
#include "EventBenchmark.h"

BENCHMARK_MAIN();