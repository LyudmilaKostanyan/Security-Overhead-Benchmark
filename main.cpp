#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#if defined(_MSC_VER)
#  include <intrin.h>
static void clobber_memory() {
    _ReadWriteBarrier();
    std::atomic_signal_fence(std::memory_order_seq_cst);
}
#else
static void clobber_memory() {
    asm volatile("" ::: "memory");
    std::atomic_signal_fence(std::memory_order_seq_cst);
}
#endif

static volatile std::uint64_t g_sink = 0;

inline void consume(std::uint64_t v) {
    g_sink += v;
    clobber_memory();
}

static void empty_function(int x) {
    consume(static_cast<std::uint64_t>(x));
}

struct Args {
    std::uint64_t iters = 50'000'000;
    std::size_t   buf_size = 64;
    std::size_t   malloc_size = 32;
};

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string s(argv[i]);
        auto next = [&](std::uint64_t& dst) {
            if (i + 1 < argc) dst = std::stoull(argv[++i]);
        };
        auto next_sz = [&](std::size_t& dst) {
            if (i + 1 < argc) dst = static_cast<std::size_t>(std::stoull(argv[++i]));
        };
        if (s == "--iters" || s == "-n") next(a.iters);
        else if (s == "--buf" ) next_sz(a.buf_size);
        else if (s == "--malloc") next_sz(a.malloc_size);
        else if (s == "--help" || s == "-h") {
            std::cout <<
R"(Usage:
  ./main [--iters N] [--buf B] [--malloc M]

Options:
  --iters, -n   Number of loop iterations per test (default 50,000,000)
  --buf         Stack buffer size for snprintf/memcpy (default 64)
  --malloc      Allocation size for malloc/free (default 32)
)";
            std::exit(0);
        }
    }
    return a;
}

template <class Fn>
static double bench(const char* name, std::uint64_t iters, Fn&& fn) {
    for (int i = 0; i < 3; ++i) fn(1000);

    auto t0 = std::chrono::steady_clock::now();
    fn(iters);
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << name << ": " << ms << " ms\n";
    return ms;
}

int main(int argc, char** argv) {
    Args args = parse_args(argc, argv);
    std::cout << "Iters = " << args.iters
              << ", buf size = " << args.buf_size
              << ", malloc size = " << args.malloc_size << "\n\n";

    double t_calls = bench("1) empty function calls", args.iters, [&](std::uint64_t N){
        for (std::uint64_t i = 0; i < N; ++i) {
            empty_function(static_cast<int>(i));
        }
    });

    double t_snprintf = bench("2) snprintf to stack buffer", args.iters, [&](std::uint64_t N){
        for (std::uint64_t i = 0; i < N; ++i) {
            std::vector<char> local(std::max<std::size_t>(args.buf_size, 16), 0);
            std::snprintf(local.data(), local.size(), "%llu", static_cast<unsigned long long>(i));
            consume(static_cast<std::uint64_t>(local[0]));
        }
    });

    double t_memcpy = bench("3) memcpy on stack buffer", args.iters, [&](std::uint64_t N){
        for (std::uint64_t i = 0; i < N; ++i) {
            std::vector<char> src(std::max<std::size_t>(args.buf_size, 16), 'x');
            std::vector<char> dst(src.size(), 0);
            std::memcpy(dst.data(), src.data(), src.size());
            consume(static_cast<std::uint64_t>(dst[0]));
        }
    });

    double t_malloc = bench("4) malloc/free small blocks", args.iters, [&](std::uint64_t N){
        const std::size_t sz = std::max<std::size_t>(args.malloc_size, 8);
        for (std::uint64_t i = 0; i < N; ++i) {
            void* p = std::malloc(sz);
            if (!p) std::abort();
            static_cast<char*>(p)[0] = static_cast<char>(i);
            consume(static_cast<std::uint64_t>(static_cast<unsigned char>(static_cast<char*>(p)[0])));
            std::free(p);
        }
    });

    return (int)(g_sink & 0);
}
