#include <pscript/context.hpp>

namespace ps {

context::context(std::size_t mem_size) : mem(mem_size) {

}

ps::memory_pool& context::memory() noexcept {
    return mem;
}

ps::memory_pool const& context::memory() const noexcept {
    return mem;
}

void context::execute(ps::script const& script) {

}

} // namespace ps