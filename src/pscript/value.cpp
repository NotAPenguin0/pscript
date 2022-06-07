#include <pscript/value.hpp>

#include <iostream>

namespace ps {

list_type::list_type(std::vector<ps::value> const& values) {
    storage = values;
    if (!values.empty()) {
        stored_type = values.front().get_type();
    }
}

void list_type::append(value const& val) {
    // TODO: Check stored type against pushed value?
    // Note that this would be hard to guarantee with the reference in get().
    storage.push_back(val);
}

ps::value& list_type::get(size_t index) {
    if (index >= storage.size()) throw std::out_of_range("ps::list index out of range");
    return storage[index];
}

size_t list_type::size() const {
    return storage.size();
}

std::ostream& operator<<(std::ostream& out, list_type const& list) {
    auto print = [&out](auto const& val) {
        out << val;
    };
    out << '[';
    for (int i = 0; i < list.storage.size(); ++i) {
        visit_value(list.storage[i], print);
        if (i != list.storage.size() - 1) out << ", ";
    }
    out << ']';
    return out;
}

string_type::string_type(std::string const& str) {
    storage = str;
}

std::ostream& operator<<(std::ostream& out, string_type const& str) {
    for (char c : str.storage) {
        out << c;
    }
    return out;
}

value::value(value const& rhs) {
    tpe = rhs.tpe;
    memory = rhs.memory;

    if (!is_null()) {
        visit_value(rhs, [this, &rhs]<typename T>(T const& rhs_val) {
            ptr = memory->allocate<T>();
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
                ptr = memory->allocate<T>();
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
    val.ptr = memory.allocate<ps::integer>();
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::integer>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, float v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::real;
    val.ptr = memory.allocate<ps::real>();
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::real>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, bool v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::boolean;
    val.ptr = memory.allocate<ps::boolean>();
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::boolean>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, ps::list_type const& v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::list;
    val.ptr = memory.allocate<ps::list>();
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::list>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, ps::string_type const& v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::str;
    val.ptr = memory.allocate<ps::str>();
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::str>(val.ptr) = v;
    return val;
}

ps::pointer value::pointer() {
    return ptr;
}

ps::type value::get_type() const {
    return tpe;
}

ps::integer& value::int_value() {
    return memory->get<ps::integer>(ptr);
}

ps::real& value::real_value() {
    return memory->get<ps::real>(ptr);
}

ps::integer const& value::int_value() const {
    return memory->get<ps::integer>(ptr);
}

ps::real const& value::real_value() const {
    return memory->get<ps::real>(ptr);
}

}