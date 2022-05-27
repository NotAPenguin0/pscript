#pragma once

#include <string>

#include <pscript/tokenizer.hpp>

namespace ps {

class script {
public:
    // TODO: Add constructor from binary_input_stream maybe?
    explicit script(std::string source);

    /**
     * @brief Get source code of the script
     */
    std::string const& source() const;

    /**
     * @brief Get list of tokens of the script
     */
    std::vector<ps::token> const& tokens() const;

private:
    std::string original_source {};
    ps::tokenize_result tok_result {};
};

}