#pragma once

#include <string>
#include <vector>

namespace ps {

class script;

struct token {
    std::string str {};
    // add source location information
};

struct tokenize_result {
    std::vector<ps::token> tokens;
    // maybe add diagnostics
};

ps::tokenize_result tokenize(ps::script const& script);

}