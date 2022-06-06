#pragma once

#include <cstddef>
#include <memory>

namespace ps {

/**
 * @brief Pointer to byte in memory.
 */
using pointer = std::size_t;
constexpr pointer null_pointer = static_cast<pointer>(-1);

using byte = std::byte;

class memory_pool {
public:
    /**
     * @brief Memory pool for allocating pscript objects from.
     * @param size
     */
    explicit memory_pool(std::size_t size);

    [[nodiscard]] bool verify_pointer(ps::pointer ptr) const noexcept;
    void verify_pointer_throw(ps::pointer ptr) const;

    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Get pointer to beginning of address range.
     */
    [[nodiscard]] ps::pointer begin() const;

    /**
     * @brief Get pointer to end of address range. Note that dereferencing this pointer is invalid.
     */
    [[nodiscard]] ps::pointer end() const;

    /**
     * @brief Unstructured memory access.
     * @param ptr Pointer with valid address.
     * @return Reference to byte at given address.
     */
    [[nodiscard]] ps::byte& operator[](ps::pointer ptr);

    /**
     * @brief Unstructured memory access.
     * @param ptr Pointer with valid address.
     * @return Const reference to byte at given address.
     */
    [[nodiscard]] ps::byte const& operator[](ps::pointer ptr) const;

    /**
     * @brief Access memory
     * @tparam T Datatype of object stored in memory.
     * @param ptr Pointer pointing to start of object.
     * @return Reference to the object in memory.
     */
    template<typename T>
    [[nodiscard]] T& get(ps::pointer ptr) {
        ps::byte* p = decode_pointer(ptr);
        return *reinterpret_cast<T*>(p);
    }

    /**
     * @brief Access memory
     * @tparam T Datatype of object stored in memory.
     * @param ptr Pointer pointing to start of object.
     * @return Const reference to the object in memory.
     */
    template<typename T>
    [[nodiscard]] T const& get(ps::pointer ptr) const {
        ps::byte const* p = decode_pointer(ptr);
        return *reinterpret_cast<T const*>(p);
    }

    /**
     * @brief Allocates memory from the pool. If this was not possible, a null pointer is returned.
     * @param bytes Amount of bytes to allocate.
     * @return Pointer to the allocated memory, or null_pointer on failure.
     */
    [[nodiscard]] ps::pointer allocate(std::size_t bytes);

    /**
     * @brief Allocates memory from the pool to fit a type T and constructs a default T at that location.
     * @tparam T Type to allocate memory for.
     * @return Pointer to the allocated memory, or null_pointer on failure.
     */
    template<typename T>
    [[nodiscard]] ps::pointer allocate() {
        ps::pointer ptr = allocate(sizeof(T));
        if (ptr == ps::null_pointer) return ptr;
        T* address = reinterpret_cast<T*>(decode_pointer(ptr));
        new (address) T {};
        return ptr;
    }

    /**
     * @brief Free a previously allocated pointer (from allocate()).
     * @param ptr Pointer to free. If null, this function does nothing.
     */
    void free(ps::pointer ptr);

private:
    // Backing store for allocator
    std::unique_ptr<ps::byte[]> memory = nullptr;
    std::size_t mem_size = 0;

    static constexpr inline std::size_t min_block_size = 8;

    /**
     * @brief Represents a block in the buddy allocator.
     *        If left and right are both nullptr, this block is a full size block.
     *        Otherwise, left and right are two buddies each half the size of the original block.
     */
    struct block {
        ps::pointer ptr = ps::null_pointer;
        std::size_t size = 0;
        bool free = true;

        std::unique_ptr<block> left = nullptr;
        std::unique_ptr<block> right = nullptr;
    };

    // Root block is the initial block and represents the full memory range.
    // Traversal will always start here.
    std::unique_ptr<block> root_block = nullptr;

    // Finds a block to allocate with given size, starting at a given root block.
    // This may further subdivide the blocks to create new smaller blocks.
    [[nodiscard]] block* find_block(block* root, std::size_t block_size);

    // Descends the tree to find the block holding the pointer and frees this block.
    // Possibly also merges it with its buddy.
    bool free_block(block* root, block* parent, ps::pointer ptr);

    // Divides a block into two buddies.
    // If the block is already the minimum size, this does nothing and returns false, otherwise it returns true.
    // If the block is allocated, this returns false.
    // If the block is already subdivided into different blocks, this returns false.
    [[nodiscard]] bool subdivide_block(block* b);

    // Merges left and right buddy blocks of given parent block back into the parent block.
    // If the block or its left and right children are not free, this returns false.
    // If this is a buddy block, this returns false.
    [[nodiscard]] bool merge_blocks(block* parent);

    [[nodiscard]] ps::byte* decode_pointer(ps::pointer ptr);
    [[nodiscard]] ps::byte const* decode_pointer(ps::pointer ptr) const;
};

}