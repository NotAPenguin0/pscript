#include <pscript/value.hpp>

#include <iostream>
#include <sstream>

#include <fmt/args.h>

#include <plib/macros.hpp>

namespace ps {

bool may_cast(type from, type to) {
    if (to == ps::type::any) return true;
    if (from == ps::type::any) return true;

    if (from == to) return true;
    if (to == ps::type::null) return false;
    if (from == ps::type::null) return false;
    if (from == ps::type::structure) return false;
    if (from == ps::type::list) return false;
    if (from == ps::type::str) return false;
    if (from == ps::type::external) return false;

    if (from == ps::type::integer || from == ps::type::real || from == ps::type::uint || from == ps::type::boolean) {
        if (to == ps::type::integer || to == ps::type::real || to == ps::type::uint || to == ps::type::boolean) return true;
        else return false;
    }

    PLIB_UNREACHABLE();
}

std::string_view type_str(type t) {
    switch(t) {
        case type::null:
            return "null";
        case type::any:
            return "any";
        case type::integer:
            return "int";
        case type::uint:
            return "uint";
        case type::real:
            return "float";
        case type::boolean:
            return "bool";
        case type::str:
            return "str";
        case type::list:
            return "list";
        case type::structure:
            return "struct";
        case type::external:
            return "external";
    }

    PLIB_UNREACHABLE();
}

list_type::list_type(std::vector<ps::value> const& values) {
    storage = values;
    if (!values.empty()) {
        stored_type = values.front().get_type();
    }
}

using namespace std::literals::string_literals;

void list_type::append(value const& val) {
    if (stored_type != ps::type::null && stored_type != ps::type::any && val.get_type() != stored_type) {
        throw std::runtime_error("TypeError: List stores objects of type "s + type_str(stored_type).data() + ", cannot insert object of type "s + type_str(val.get_type()).data());
    }
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
[[maybe_unused]] void try_push_arg<ps::string_type>(arg_store& dyn, ps::string_type const& value) {
    dyn.push_back(value.representation());
}

template<>
[[maybe_unused]] void try_push_arg<ps::list_type>(arg_store& dyn, ps::list_type const& value) {
    dyn.push_back(value.to_string());
}

template<>
[[maybe_unused]] void try_push_arg<ps::struct_type>(arg_store& dyn, ps::struct_type const& value) {
    dyn.push_back(value.to_string());
}

template<>
[[maybe_unused]] void try_push_arg<ps::external_type>(arg_store& dyn, ps::external_type const& value) {
    dyn.push_back(value.pointer());
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

struct_type::struct_type(std::string const& name, std::unordered_map<std::string, ps::value> const& initializers) {
    this->name = name;
    members = initializers;
}

std::string struct_type::to_string() const {
    std::ostringstream oss {};
    oss << name;
    oss << " {\n";
    for (auto const& [field_name, value] : members) {
        oss << '\t' << field_name << ": " << value << '\n';
    }
    oss << "}";
    return oss.str();
}

ps::value& struct_type::access(std::string const& field_name) {
    return members.at(field_name);
}

ps::value const& struct_type::access(std::string const& field_name) const {
    return members.at(field_name);
}

[[nodiscard]] std::string const& struct_type::type_name() const {
    return name;
}

std::ostream& operator<<(std::ostream& out, struct_type const& s) {
    return out << s.to_string();
}

external_type::external_type(void* pointer, ps::type type) {
    ptr = pointer;
    this->type = type;
}

std::ostream& operator<<(std::ostream& out, external_type const& e) {
    return out << "[external object at " << e.ptr << "]";
}

static bool is_reference_type(ps::type type) {
    if (type == ps::type::list || type == ps::type::str || type == ps::type::structure) return true;
    else return false;
}

value::value(value const& rhs) {
    tpe = rhs.tpe;
    memory = rhs.memory;
    is_ref = rhs.is_ref;

    if (!is_null()) {
        if (rhs.is_reference() || is_reference_type(tpe)) {
            ptr = rhs.ptr;
            refcount = rhs.refcount;
            if (refcount) (*refcount)++;
        } else {
            visit_value(rhs, [this]<typename T>(T const& rhs_val) {
                ptr = memory->allocate<T>();
                if (ptr == ps::null_pointer) throw std::bad_alloc();
                static_cast<T&>(*this) = rhs_val;
            });
        }
    }
}

using namespace std::literals::string_literals;

value& value::operator=(value const& rhs) {
    if (&rhs != this) {
        // First do a type check
        if (tpe != ps::type::null && tpe != rhs.tpe && !may_cast(rhs.tpe, tpe)) {
            throw std::runtime_error("TypeError: Invalid cast from "s + type_str(rhs.tpe).data() + " to "s + type_str(tpe).data() + ".");
        }

        // do typecheck for struct types by comparing names
        if (tpe == ps::type::structure && rhs.tpe == ps::type::structure) {
            auto& lhs_struct = static_cast<ps::structure&>(*this);
            auto const& rhs_struct = static_cast<ps::structure const&>(rhs);
            if (lhs_struct->type_name() != rhs_struct->type_name()) {
                throw std::runtime_error("TypeError: Invalid cast from "s + lhs_struct->type_name() + " to "s + rhs_struct->type_name() + ".");
            }
        }

        // destroy old value
        on_destroy();

        is_ref = rhs.is_ref;
        tpe = rhs.tpe;
        memory = rhs.memory;
        if (rhs.is_reference() || is_reference_type(tpe)) {
            ptr = rhs.ptr;
            refcount = rhs.refcount;
            if (refcount) (*refcount)++;
        } else {
            visit_value(rhs, [this]<typename T>(T const& rhs_val) {
                ptr = memory->allocate<T>();
                if (ptr == ps::null_pointer) throw std::bad_alloc();
                static_cast<T&>(*this) = rhs_val;
            });
        }
    }
    return *this;
}

value::value(ps::value&& rhs) noexcept {
    if (&rhs != this) {
        ptr = rhs.ptr;
        tpe = rhs.tpe;
        memory = rhs.memory;
        refcount = rhs.refcount;
        is_ref = rhs.is_ref;
        rhs.ptr = ps::null_pointer;
        rhs.tpe = {};
        rhs.memory = nullptr;
        rhs.refcount = nullptr;
        rhs.is_ref = false;
    }
}

ps::value& value::operator=(ps::value&& rhs) noexcept {
    if (&rhs != this) {
        on_destroy();

        ptr = rhs.ptr;
        tpe = rhs.tpe;
        memory = rhs.memory;
        refcount = rhs.refcount;
        is_ref = rhs.is_ref;
        rhs.ptr = ps::null_pointer;
        rhs.tpe = {};
        rhs.memory = nullptr;
        rhs.refcount = nullptr;
        rhs.is_ref = false;
    }
    return *this;
}

void value::on_destroy() {
    if (ptr != ps::null_pointer) {
        // call object destructor
        if (is_reference()) {
            if (refcount) {
                (*refcount)--;
                if (*refcount == 0) {
                    visit_value(*this, []<typename T>(T& val) {
                        val.~T();
                    });
                    memory->free(ptr);
                }
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
    val.is_ref = true;
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
    val.is_ref = true;
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
    val.is_ref = true;
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::structure>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, ps::external_type const& v) {
    ps::value val {};
    val.memory = &memory;
    val.tpe = type::external;
    val.ptr = memory.allocate<ps::external>();
    if (val.ptr == ps::null_pointer) throw std::bad_alloc();
    memory.get<ps::external>(val.ptr) = v;
    return val;
}

ps::value value::ref(ps::value const& rhs) {
    ps::value val {};
    val.refcount = rhs.refcount;
    val.tpe = rhs.tpe;
    val.memory = rhs.memory;
    val.ptr = rhs.ptr;
    val.is_ref = true;
    return val;
}

ps::pointer value::pointer() const {
    return ptr;
}

ps::type value::get_type() const {
    return tpe;
}

[[maybe_unused]] ps::integer& value::int_value() {
    return memory->get<ps::integer>(ptr);
}

ps::real& value::real_value() {
    return memory->get<ps::real>(ptr);
}

[[maybe_unused]] ps::integer const& value::int_value() const {
    return memory->get<ps::integer>(ptr);
}

ps::real const& value::real_value() const {
    return memory->get<ps::real>(ptr);
}

void value::cast_this(ps::type new_type) {
    if (!may_cast(new_type, tpe)) throw std::runtime_error("TypeError: Invalid cast_this() call");
    visit_type(new_type, [this]<typename T>() {
        T new_val = cast<T>();
        *this = value::from(*memory, new_val.value());
    });
}

std::ostream& operator<<(std::ostream& out, value const& v) {
    auto print = [&out](auto const& val) {
        out << val;
    };
    visit_value(v, print);
    return out;
}

}
