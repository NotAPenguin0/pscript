#include <pscript/variable.hpp>

namespace ps {

variable::variable(std::string_view name, ps::value&& initializer) : var_name(name), val(std::move(initializer)) {

}

void variable::set_name(std::string_view name) {
    var_name = name;
}

std::string_view variable::name() const {
    return var_name;
}

ps::value& variable::value() {
    return val;
}

ps::value const& variable::value() const {
    return val;
}


}