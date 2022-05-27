#include <pscript/value.hpp>

namespace ps {

ps::value value::from(ps::memory_pool& memory, int v) {
    ps::value val {};
    val.tpe = type::integer;
    val.ptr = memory.allocate(sizeof(ps::types::integer));
    memory.get<ps::types::integer>(val.ptr) = v;
    return val;
}

ps::value value::from(ps::memory_pool& memory, float v) {
    ps::value val {};
    val.tpe = type::fp;
    val.ptr = memory.allocate(sizeof(ps::types::fp));
    memory.get<ps::types::fp>(val.ptr) = v;
    return val;
}

ps::pointer value::pointer() {
    return ptr;
}

ps::value::type value::get_type() {
    return tpe;
}

ps::types::integer& value::int_value(ps::memory_pool& memory) {
    return memory.get<ps::types::integer>(ptr);
}

ps::types::fp& value::fp_value(ps::memory_pool& memory) {
    return memory.get<ps::types::fp>(ptr);
}

ps::types::integer const& value::int_value(ps::memory_pool const& memory) const {
    return memory.get<ps::types::integer>(ptr);
}

ps::types::fp const& value::fp_value(ps::memory_pool const& memory) const {
    return memory.get<ps::types::fp>(ptr);
}

}