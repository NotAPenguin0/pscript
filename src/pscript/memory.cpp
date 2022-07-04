#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#include <pscript/memory.hpp>

#include <stdexcept>

#include <plib/bits.hpp>
#include <plib/macros.hpp>

#include <cstring>

namespace ps {

memory_pool::memory_pool(std::size_t size) {
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
    // Compute the smallest possible block size that would fit this allocation
    std::size_t const block_size = std::max(min_block_size, plib::next_pow_two(bytes));
    // Find a block, possibly subdividing blocks
    block* block = find_block(root_block.get(), block_size);
    // If no block was found, return a null pointer
    if (!block) return null_pointer;
    // Mark it as allocated and return the pointer
    block->free = false;
    return block->ptr;
}

void memory_pool::free(ps::pointer ptr) {
    if (!verify_pointer(ptr)) return;

    free_block(root_block.get(), nullptr, ptr);
}

[[nodiscard]] bool memory_pool::verify_pointer(ps::pointer ptr) const noexcept {
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

bool memory_pool::free_block(block* root, block* parent, ps::pointer ptr) {
    // We need to find the block holding the allocated pointer.
    // This is going to be the block with the free flag on false, and holding this pointer.

    if (!root) return false;

    // Found matching pointer. This will be the allocated block if it has no child buddies.
    if (root->ptr == ptr && !root->left && !root->right) {
        // Mark the block as free
        root->free = true;
        // Zero out block memory
        std::memset(decode_pointer(root->ptr), 0, root->size);
        // If this was a small block, and there is space left in the small block cache, add it to the cache.
        if (root->size == min_block_size && num_small_blocks != small_block_cache.size()) {
            small_block_cache[num_small_blocks++] = root;
        }
        // If there is a parent, check if both left and right of it are free
        if (parent && parent->left->free && parent->right->free) {
            // If so, merge those blocks
            bool success = merge_blocks(parent);
            // Something went wrong
            if (!success) return false;
        }
        return true; // freed a block, return true

    } else {
        // This isn't the block holding the allocation, try to free in left and right

        // Has no children, early exit.
        if (!root->left || !root->right) return false;

        // We can make a small optimization by comparing the pointer with the pointer of the right child.
        if (ptr < root->right->ptr) {
            // Block must be in left child
            bool free = free_block(root->left.get(), root, ptr);

            // Once again try to merge
            if (free && parent && parent->free && parent->left->free && parent->right->free) {
                bool success = merge_blocks(parent);
                if (!success) return false;
            }

            return free;
        } else {
            // Block must be in right child
            bool free = free_block(root->right.get(), root, ptr);

            // Once again try to merge
            if (free && parent && parent->free && parent->left->free && parent->right->free) {
                bool success = merge_blocks(parent);
                if (!success) return false;
            }

            return free;
        }

        PLIB_UNREACHABLE();
    }
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

    // Right block starts at same pointer but offset for left block, with all leftover bytes.
    b->right = std::make_unique<block>();
    b->right->ptr = b->ptr + new_size;
    b->right->size = b->size - new_size;

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
    // TODO: possibly add toggle to disable this
    verify_pointer_throw(ptr);
    return &memory[ptr];
}

[[nodiscard]] ps::byte const* memory_pool::decode_pointer(ps::pointer ptr) const {
    verify_pointer_throw(ptr);
    return &memory[ptr];
}

} // namespace ps
#pragma clang diagnostic pop
