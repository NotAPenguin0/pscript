#include <pscript/context.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace ch = std::chrono;
namespace fs = std::filesystem;

// returns runtime in milliseconds
float bench_script(std::string const& source, std::size_t iterations) {
    ch::nanoseconds time {};
    std::ostringstream output {};
    std::ostringstream error_output {};
    for (std::size_t i = 0; i < iterations; ++i) {
        ps::context ctx(16 * 1024 * 1024); // 16 MiB memory heap for benchmarks
        ps::script script(source, ctx);
        ps::execution_context exec;
        exec.out = &output;
        exec.err = &error_output;
        auto start = ch::high_resolution_clock::now();
        ctx.execute(script, exec);
        auto end = ch::high_resolution_clock::now();
        time += ch::duration_cast<ch::nanoseconds>(end - start);
    }
    if (!error_output.str().empty()) {
        std::cerr << error_output.str() << std::endl;
    }
    return time.count() / (iterations * 1000000.0f);
}


std::string read_file(fs::path const& path) {
    std::ifstream in { path };
    return std::string { std::istreambuf_iterator{ in }, {} };
}

int main() {
    constexpr std::size_t iterations = 50;

    std::cout << std::setprecision(4);
    std::cout << "Benchmark\t\t||\t\tAverage runtime (milliseconds)\n";
    for (auto const& entry : fs::directory_iterator("benchmarks/")) {
        std::string source = read_file(entry.path());
        auto average_runtime = bench_script(source, iterations);
        std::cout << entry.path().stem().generic_string() << "\t\t||\t\t" << average_runtime << std::endl;
    }
}