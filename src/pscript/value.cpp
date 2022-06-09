#include <pscript/value.hpp>

#include <iostream>
#include <cstdarg>
#include <sstream>

#include <fmt/format.h>
#include <fmt/args.h>

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

std::string list_type::to_string() const {
    std::ostringstream out {};
    out << '[';
    for (int i = 0; i < storage.size(); ++i) {
        out << storage[i];
        if (i != storage.size() - 1) out << ", ";
    }
    out << ']';
    return out.str();
}

std::ostream& operator<<(std::ostream& out, list_type const& list) {
    return out << list.to_string();
}

string_type::string_type(std::string const& str) {
    storage = str;
}

using arg_store = fmt::dynamic_format_arg_store<fmt::format_context>;

template<typename T>
static void try_push_arg(arg_store& dyn, T const& value) {
    dyn.push_back(value);
}

template<>
void try_push_arg<ps::string_type>(arg_store& dyn, ps::string_type const& value) {
    dyn.push_back(value.representation());
}

template<>
void try_push_arg<ps::list_type>(arg_store& dyn, ps::list_type const& value) {
    dyn.push_back(value.to_string());
}

template<>
void try_push_arg<ps::struct_type>(arg_store& dyn, ps::struct_type const& value) {
    dyn.push_back(value.to_string());
}

static std::string format_vector(std::string_view format, std::vector<ps::value> const& args) {
    arg_store fmt_args {};

    for (auto const& a : args) {
        visit_value(a, [&fmt_args](auto const& v) {
            try_push_arg(fmt_args, v.value());
        });
    }

    return fmt::vformat(format, fmt_args);
}


string_type string_type::format(std::vector<ps::value> const& args) const {
    return ps::string_type { format_vector(storage, args) };
}

int string_type::parse_int() const {
    return std::stoi(storage);
}

float string_type::parse_float() const {
    return std::stof(storage);
}

std::ostream& operator<<(std::ostream& out, string_type const& str) {
    return out << str.storage;
}

struct_type::struct_type(std::unordered_map<std::string, ps::value> const& initializers) {
    members = initializers;
}

std::string struct_type::to_string() const {
    std::ostringstream oss {};
    oss << "{\n";
    for (auto const& [name, value] : members) {
        oss << '\t' << name << ": " << value << '\n';
    }
    oss << "}";
    return oss.str();
}

ps::value& struct_type::access(std::string const& name) {
    return members.at(name);
}

ps::value const& struct_type::access(std::string const& name) const {
    return members.at(name);
}

std::ostream& operator<<(std::ostream& out, struct_type const& s) {
    return out << s.to_string();
}

static bool is_reference_type(ps::type type) {
    if (type == ps::type::list || type == ps::type::str || type == ps::type::structure) return true;
    else return false;
}

value::value(value const& rhs) {
    tpe = rhs.tpe;
    memory = rhs.memory;

    if (!is_null()) {
        if (is_reference_type(tpe)) {
            ptr = rhs.ptr;
            refcount = rhs.refcount;
            (*refcount)++;
        } else {
            visit_value(rhs, [this, &rhs]<typename T>(T const& rhs_val) {
                ptr = memory->allocate<T>();
                if (ptr == ps::null_pointer) throw std::bad_alloc();
                static_cast<T&>(*this) = rhs_val;
            });
        }
    }
}

value& value::operator=(value const& rhs) {
    if (&rhs != this) {
        // destroy old value
        on_destroy();

        tpe = rhs.tpe;
        memory = rhs.memory;
        if (is_reference_type(tpe)) {
            ptr = rhs.ptr;
            refcount = rhs.refcount;
            (*refcount)++;
        } else {
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
        refcount = rhs.refcount;
        rhs.ptr = ps::null_pointer;
        rhs.tpe = {};
        rhs.memory = nullptr;
        rhs.refcount = nullptr;
    }
}

ps::value& value::operator=(ps::value&& rhs)  noexcept {
    if (&rhs != this) {
        on_destroy();

        ptr = rhs.ptr;
        tpe = rhs.tpe;
        memory = rhs.memory;
        refcount = rhs.refcount;
        rhs.ptr = ps::null_pointer;
        rhs.tpe = {};
        rhs.memory = nullptr;
        rhs.refcount = nullptr;
    }
    return *this;
}

void value::on_destroy() {
    if (ptr != ps::null_pointer) {
        // call object destructor
        if (is_reference_type(tpe)) {
            (*refcount)--;
            if (*refcount == 0) {
                visit_value(*this,[] <typename T> (T& val) {
                    val.~T();
                });
                memory->free(ptr);
            }
        }
        else {
            visit_value(*this, []<typename T>(T& val) {
                val.~T();
            });
            memory->free(ptr);
        }
    }
}

value::~value() {
    on_destroy();
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

ps::value value::from(ps::memory_pool& memory, unsigned int v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::uint;
    val.ptr = memory.allocate<ps::uint>();
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::uint>(val.ptr) = v;
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
    val.refcount = std::make_shared<int>(1); // initialize refcounter for reference types
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::list>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, ps::string_type const& v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::str;
    val.ptr = memory.allocate<ps::str>();
    val.refcount = std::make_shared<int>(1); // initialize refcounter for reference types
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::str>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, ps::struct_type const& v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::structure;
    val.ptr = memory.allocate<ps::structure>();
    val.refcount = std::make_shared<int>(1); // initialize refcounter for reference types
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::structure>(val.ptr) = v;
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

std::ostream& operator<<(std::ostream& out, value const& v) {
    auto print = [&out](auto const& val) {
        out << val;
    };
    visit_value(v, print);
    return out;
}

}