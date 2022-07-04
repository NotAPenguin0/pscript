#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

#include <argumentum/argparse.h>
#include <pscript/context.hpp>

namespace fs = std::filesystem;

int run_from_file(fs::path const& file, std::size_t memory) {
    ps::context ctx(memory);

    std::ifstream in { file };
    if (!in.good()) {
        std::cerr << "Failed to open file " << file << std::endl;
        return -1;
    }

    std::string source { std::istreambuf_iterator<char>{in}, {} };
    ps::script script(source, ctx);

    ctx.execute(script);

    return 0;
}

int run_interactive(std::size_t memory) {
    ps::context ctx(memory);
    std::cout << "====================== Pscript interactive tool ======================\n";
    while(true) {
        std::cout << ">>> " << std::flush;
        std::string input {};
        std::getline(std::cin, input);
        if (input == "quit") break;

        std::shared_ptr<ps::script> script = std::make_shared<ps::script>(input, ctx);
        ctx.execute(script);
    }

    return 0;
}

int main(int argc, char** argv) {
    fs::path file = "";
    std::size_t memory = 1024 * 1024;

    auto parser = argumentum::argument_parser{};
    auto params = parser.params();

    parser.config().program(argv[0]).description("Command line interpreter for the pscript programming language.");
    params.add_parameter(file, "--file", "-f")
        .nargs(1)
        .help("path to the pscript file to execute.")
        .absent("")
        .required(false);
    params.add_parameter(memory, "--memory", "-m")
        .required(false)
        .absent(1024 * 1024)
        .nargs(1)
        .help("Memory to allocate for interpreter");

    if (!parser.parse_args(argc, argv)) {
        return 1;
    }

    // If absent, run in interactive mode.
    if (file == "") {
        return run_interactive(memory);
    }
    else {
        return run_from_file(file, memory);
    }
}