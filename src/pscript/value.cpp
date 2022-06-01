#include <pscript/value.hpp>

#include <iostream>

namespace ps {

value::value(value const& rhs) {
    tpe = rhs.tpe;
    memory = rhs.memory;

    if (!is_null()) {
        visit_value(rhs, [this, &rhs]<typename T>(T const& rhs_val) {
            ptr = memory->allocate(sizeof(T));
            if (ptr == ps::null_pointer) throw std::bad_alloc();
            static_cast<T&>(*this) = rhs_val;
        });
    }
}

value& value::operator=(value const& rhs) {
    if (&rhs != this) {
        // destroy old value
        if (ptr != ps::null_pointer) {
            // call object destructor
            visit_value(*this,[this] <typename T> (T& val) {
                val.~T();
            });
            memory->free(ptr);
        }

        tpe = rhs.tpe;
        memory = rhs.memory;
        if (!is_null()) {
            visit_value(rhs, [this, &rhs]<typename T>(T const& rhs_val) {
                ptr = memory->allocate(sizeof(T));
                if (ptr == ps::null_pointer) throw std::bad_alloc();
                static_cast<T&>(*this) = rhs_val;
            });
        }
    }
    return *this;
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
    if (ptr != ps::null_pointer) {
        // call object destructor
        visit_value(*this,[this] <typename T> (T& val) {
            val.~T();
        });
        memory->free(ptr);
    }
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
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::types::integer>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, float v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::real;
    val.ptr = memory.allocate(sizeof(ps::types::real));
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::types::real>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, bool v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::boolean;
    val.ptr = memory.allocate(sizeof(ps::types::boolean));
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::types::boolean>(val.ptr) = v;
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

}