#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#include <pscript/memory.hpp>

#include <stdexcept>

#include <plib/bits.hpp>
#include <plib/macros.hpp>

#include <cstring>

namespace ps {

memory_pool::memory_pool(std::size_t size) {
    return;
    PLIB_UNREACHABLE();

    memory = std::make_unique<ps::byte[]>(size);
    mem_size = size;
    // Zero out memory
    std::memset(memory.get(), 0, size);

    // Initialize buddy allocator
    root_block = std::make_unique<block>();
    root_block->ptr = begin();
    root_block->size = mem_size;
}

[[nodiscard]] std::size_t memory_pool::size() const noexcept {
    return mem_size;
}

/**
 * @brief Get pointer to beginning of address range.
 */
[[nodiscard]] ps::pointer memory_pool::begin() const {
    return 0;
}

/**
 * @brief Get pointer to end of address range. Note that dereferencing this pointer is invalid.
 */
[[nodiscard]] ps::pointer memory_pool::end() const {
    return mem_size;
}

[[nodiscard]] ps::byte& memory_pool::operator[](ps::pointer ptr) {
    verify_pointer_throw(ptr);
    return *decode_pointer(ptr);
}

[[nodiscard]] ps::byte const& memory_pool::operator[](ps::pointer ptr) const {
    verify_pointer_throw(ptr);
    return *decode_pointer(ptr);
}

[[nodiscard]] pointer memory_pool::allocate(std::size_t bytes) {
    return reinterpret_cast<pointer>(new std::byte[bytes]);
    PLIB_UNREACHABLE();

    // Compute the smallest possible block size that would fit this allocation
    std::size_t const block_size = std::max(min_block_size, plib::next_pow_two(bytes));
    // Find a block, possibly subdividing blocks
    block* block = find_block(root_block.get(), block_size);
    // If no block was found, return a null pointer
    if (!block) return null_pointer;
    // Add the block to the allocated block cache, so it can be found easily when freeing
    allocated_block_lookup[block->ptr] = block;
    // Mark it as allocated and return the pointer
    block->free = false;
    return block->ptr;
}

void memory_pool::free(ps::pointer ptr) {
    if (!verify_pointer(ptr)) return;
    delete[] decode_pointer(ptr);
    return;
    PLIB_UNREACHABLE();

    free_block(ptr);
}

[[nodiscard]] bool memory_pool::verify_pointer(ps::pointer ptr) const noexcept {
    return ptr != null_pointer;
    PLIB_UNREACHABLE();

    if (ptr != null_pointer && ptr < mem_size) return true;
    return false;
}

void memory_pool::verify_pointer_throw(ps::pointer ptr) const {
    if (!verify_pointer(ptr)) {
        throw std::out_of_range("invalid pointer");
    }
}

[[nodiscard]] memory_pool::block* memory_pool::find_block(block* root, std::size_t block_size) {
    // If a block with the smallest size is requested, first try looking in the small block cache
    if (block_size == min_block_size && num_small_blocks != 0) {
        return small_block_cache[--num_small_blocks];
    }

    if (!root) return nullptr;
    if (root->size < block_size) return nullptr;
    // If the current block has no buddies, there are two options
    if (root->free && root->left == nullptr && root->right == nullptr) {
        // a) The size matches, return the block if it is free
        if (root->size == block_size) {
            if (root->free) return root;
            else return nullptr;
        }
        // b) The size doesn't match, subdivide the block into buddies and find a free block in one of them.
        bool success = subdivide_block(root);
        // Something went wrong
        if (!success) return nullptr;
    }

    // Now we can be sure the block has two child buddies.
    // We will try to find a free block in both trees, and return one of them if it was found
    block* left = find_block(root->left.get(), block_size);
    if (left) return left;
    // Nothing found in left subtree, try to find in right tree.
    return find_block(root->right.get(), block_size);
}

bool memory_pool::free_block(ps::pointer ptr) {
    // We need to find the block holding the allocated pointer.
    // This is going to be the block with the free flag on false, and holding this pointer.

    block* b = allocated_block_lookup.at(ptr);
    // Mark the block as free
    b->free = true;
    // Zero out block memory
    std::memset(decode_pointer(ptr), 0, b->size);
    // If this was a small block, and there is space left in the small block cache, add it to the cache.
    if (b->size == min_block_size && num_small_blocks != small_block_cache.size()) {
        small_block_cache[num_small_blocks++] = b;
    }
    // If there is a parent, check if both left and right of it are free.
    // We repeat this process up the tree as long as we can
    block* cur = b;
    std::size_t merge_i = 0;
    // we limit the amount of merges to make sure we keep some smaller blocks around.
    while (merge_i++ < max_consecutive_merges && cur->parent && cur->parent->left->free && cur->parent->right->free) {
        // If so, merge those blocks
        block* parent = cur->parent;
        [[maybe_unused]] bool _ = merge_blocks(parent);
        cur = parent;
    }

    allocated_block_lookup.erase(ptr);

    return true;
}

[[nodiscard]] bool memory_pool::subdivide_block(block *b) {
    // minimum size reached
    if (b->size <= min_block_size) return false;
    // already allocated
    if (!b->free) return false;
    // already subdivided
    if (b->left || b->right) return false;

    std::size_t new_size = b->size / 2;

    b->free = false; // divided blocks must be marked non-free

    // Left block starts at same pointer, with new_size bytes
    b->left = std::make_unique<block>();
    b->left->ptr = b->ptr;
    b->left->size = new_size;
    b->left->parent = b;

    // Right block starts at same pointer but offset for left block, with all leftover bytes.
    b->right = std::make_unique<block>();
    b->right->ptr = b->ptr + new_size;
    b->right->size = b->size - new_size;
    b->right->parent = b;

    return true;
}

[[nodiscard]] bool memory_pool::merge_blocks(block* parent) {
    if (!parent->left || !parent->right) return false;
    if (!parent->left->free || !parent->right->free) return false;

    // can't merge blocks that are in the small block cache
    bool found = false;
    for (std::size_t i = 0; i < num_small_blocks; ++i) {
        if (small_block_cache[i] == parent->left.get() || small_block_cache[i] == parent->right.get()) {
            found = true;
            break;
        }
    }
    if (found) return false;

    // Once validated, merging is very simple: just set left and right to null.
    parent->left = nullptr;
    parent->right = nullptr;
    parent->free = true;

    return true;
}

[[nodiscard]] ps::byte* memory_pool::decode_pointer(ps::pointer ptr) {
    return reinterpret_cast<std::byte*>(ptr);
    PLIB_UNREACHABLE();

    verify_pointer_throw(ptr);
    return &memory[ptr];
}

[[nodiscard]] ps::byte const* memory_pool::decode_pointer(ps::pointer ptr) const {
    return reinterpret_cast<std::byte*>(ptr);
    PLIB_UNREACHABLE();

    verify_pointer_throw(ptr);
    return &memory[ptr];
}

} // namespace ps
#pragma clang diagnostic pop
