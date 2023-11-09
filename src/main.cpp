#include "benchmark/benchmark.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/iostream.hpp>

static void hpx_parallel_mmul_bench(benchmark::State &s) {
  using element_type = int;

  // Define matrix sizes
  std::size_t const rowsA = s.range(0);
  std::size_t const colsA = s.range(0);
  std::size_t const rowsB = colsA;
  std::size_t const colsB = s.range(0);
  std::size_t const rowsR = rowsA;
  std::size_t const colsR = colsB;

  // Initialize matrices A and B
  std::vector<int> A(rowsA * colsA);
  std::vector<int> B(rowsB * colsB);
  std::vector<int> R(rowsR * colsR);

  // Create our random number generators
  std::mt19937 rng;
  rng.seed(std::random_device()());
  std::uniform_real_distribution<float> dist(-10, 10);
  auto generator = std::bind(dist, rng);

  hpx::ranges::generate(A, generator);
  hpx::ranges::generate(B, generator);

  std::chrono::duration<double, std::micro> sleep_duration{
      static_cast<double>(benchmark::kMillisecond)};

  // Perform matrix multiplication
  for (auto _ : s) {
    hpx::experimental::for_loop(hpx::execution::par, 0, rowsA, [&](auto i) {
      hpx::experimental::for_loop(0, colsB, [&](auto j) {
        R[i * colsR + j] = 0;
        hpx::experimental::for_loop(0, rowsB, [&](auto k) {
          R[i * colsR + j] += A[i * colsA + k] * B[k * colsB + j];
        });
      });
    });
  }
}

// Parallel implementation
void parallel_mmul(const float *A, const float *B, float *C, std::size_t N,
                   std::size_t start_row, std::size_t end_row) {
  // For each row assigned to this thread...
  for (std::size_t row = start_row; row < end_row; row++)
    // For each column...
    for (std::size_t col = 0; col < N; col++)
      // For each element in the row-col pair...
      for (std::size_t idx = 0; idx < N; idx++)
        // Accumulate the partial results
        C[row * N + col] += A[row * N + idx] * B[idx * N + col];
}

// Parallel MMul benchmark
static void parallel_mmul_bench(benchmark::State &s) {
  // Number Dimensions of our matrix
  std::size_t N = s.range(0);

  // Create our random number generators
  std::mt19937 rng;
  rng.seed(std::random_device()());
  std::uniform_real_distribution<float> dist(-10, 10);

  // Create input matrices
  float *A = new float[N * N];
  float *B = new float[N * N];
  float *C = new float[N * N];

  // Initialize them with random values (and C to 0)
  std::generate(A, A + N * N, [&] { return dist(rng); });
  std::generate(B, B + N * N, [&] { return dist(rng); });
  std::generate(C, C + N * N, [&] { return 0.0f; });

  // Set up for launching threads
  std::size_t num_threads = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  // Calculate values to pass to threads
  // Assumed to be divisable by num_threads (evenly)
  std::size_t n_rows = N / num_threads;

  // Main benchmark loop
  for (auto _ : s) {
    // Launch threads
    std::size_t end_row = 0;
    for (std::size_t i = 0; i < num_threads - 1; i++) {
      auto start_row = i * n_rows;
      end_row = start_row + n_rows;
      threads.emplace_back(
          [&] { parallel_mmul(A, B, C, N, start_row, end_row); });
    }

    // Wait for all threads to complete
    for (auto &t : threads)
      t.join();

    // Clear the threads each iteration of the benchmark
    threads.clear();
  }

  // Free memory
  free(A);
  free(B);
  free(C);
}

BENCHMARK(parallel_mmul_bench)
    ->Arg(384)
    ->Arg(768)
    ->Arg(1152)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK(hpx_parallel_mmul_bench)
    ->Arg(384)
    ->Arg(768)
    ->Arg(1152)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

int main(int argc, char **argv) {
  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
}