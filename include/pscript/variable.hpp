#pragma once

#include <pscript/value.hpp>

#include <string_view>

namespace ps {

/**
 * @brief Represents a variable as the combination of a value and a name.
 */
class variable {
public:
    template<typename T>
    variable(ps::memory_pool& memory, std::string_view name, T const& initializer)
        : var_name(name), val(ps::value::from(memory, initializer)) {

    }

    variable(std::string_view name, ps::value&& initializer);

    std::string_view name() const;
    void set_name(std::string_view name);

    ps::value& value();
    ps::value const& value() const;

private:
    std::string_view var_name;
    ps::value val;
};

}