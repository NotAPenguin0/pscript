#include <pscript/value.hpp>

namespace ps {

value::value(value const& rhs) {
    if (&rhs != this) {
        tpe = rhs.tpe;
        memory = rhs.memory;
        if (tpe == type::integer) {
            ptr = memory->allocate(sizeof(ps::types::integer));
            int_value() = rhs.int_value();
        } else if (tpe == type::real) {
            ptr = memory->allocate(sizeof(ps::types::real));
            real_value() = rhs.real_value();
        }
    }
}

value::value(ps::value&& rhs)  noexcept {
    if (&rhs != this) {
        ptr = rhs.ptr;
        tpe = rhs.tpe;
        memory = rhs.memory;
        rhs.ptr = ps::null_pointer;
        rhs.tpe = {};
        rhs.memory = nullptr;
    }
}

ps::value& value::operator=(ps::value&& rhs)  noexcept {
    if (&rhs != this) {
        ptr = rhs.ptr;
        tpe = rhs.tpe;
        memory = rhs.memory;
        rhs.ptr = ps::null_pointer;
        rhs.tpe = {};
        rhs.memory = nullptr;
    }
    return *this;
}

value::~value() {
    if (ptr != ps::null_pointer)
        memory->free(ptr);
}

ps::value value::null() {
    ps::value val {};
    val.tpe = type::null;
    val.ptr = ps::null_pointer;
    return val;
}

ps::value value::from(ps::memory_pool& memory, int v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::integer;
    val.ptr = memory.allocate(sizeof(ps::types::integer));
    memory.get<ps::types::integer>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, float v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::real;
    val.ptr = memory.allocate(sizeof(ps::types::real));
    memory.get<ps::types::real>(val.ptr) = v;
    return val;
}

ps::pointer value::pointer() {
    return ptr;
}

ps::value::type value::get_type() const {
    return tpe;
}

ps::types::integer& value::int_value() {
    return memory->get<ps::types::integer>(ptr);
}

ps::types::real& value::real_value() {
    return memory->get<ps::types::real>(ptr);
}

ps::types::integer const& value::int_value() const {
    return memory->get<ps::types::integer>(ptr);
}

ps::types::real const& value::real_value() const {
    return memory->get<ps::types::real>(ptr);
}

ps::value value::operator+(ps::value const& rhs) const {
    if (tpe == type::integer) return value::from(*memory, int_value() + rhs.int_value());
    else if (tpe == type::real) return value::from(*memory, real_value() + rhs.real_value());
    return {};
}

ps::value value::operator*(ps::value const& rhs) const {
    if (tpe == type::integer) return value::from(*memory, int_value() * rhs.int_value());
    else if (tpe == type::real) return value::from(*memory, real_value() * rhs.real_value());
    return {};
}

}