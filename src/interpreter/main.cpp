#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

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
    constexpr std::size_t memory = 1024 * 1024;

    if (argc > 3) {
        std::cerr << "usage: pscript [filename] [memory]" << std::endl;
        return -1;
    }

    if (argc == 1) {
        run_interactive(memory);
    }

    if (argc == 2) {
        // either memory argument or filename argument
        // if it contains a dot, it's a filename
        std::string arg { argv[1] };
        if (arg.find('.') != std::string::npos) {
            run_from_file(arg, memory);
        } else {
            run_interactive(std::stoi(arg));
        }
    }

    else if (argc == 3) {
        // path + memory
        run_from_file(argv[1], std::stoi(argv[2]));
    }
}