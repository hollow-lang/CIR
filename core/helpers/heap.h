#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

class Heap {
    static constexpr size_t ALIGNMENT = 8;

    struct Block {
        size_t size;
        bool is_free;
        Block *next;
        Block *prev;
    };

    std::vector<uint8_t> heap;
    Block *free_list{};

    size_t align(size_t size) const {
        return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }

    Block *findFreeBlock(size_t size) {
        Block *current = free_list;
        while (current) {
            if (current->is_free && current->size >= size) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    void splitBlock(Block *block, size_t size) {
        if (block->size >= size + sizeof(Block) + ALIGNMENT) {
            Block *new_block = reinterpret_cast<Block *>(
                reinterpret_cast<uint8_t *>(block) + sizeof(Block) + size
            );

            new_block->size = block->size - size - sizeof(Block);
            new_block->is_free = true;
            new_block->next = block->next;
            new_block->prev = block;

            if (block->next) {
                block->next->prev = new_block;
            }

            block->size = size;
            block->next = new_block;
        }
    }

public:
    explicit Heap(size_t heap_size) : heap(heap_size) {
        free_list = reinterpret_cast<Block *>(heap.data());
        free_list->size = heap_size - sizeof(Block);
        free_list->is_free = true;
        free_list->next = nullptr;
        free_list->prev = nullptr;
    }

    void *allocate(size_t size) {
        if (size == 0) return nullptr;

        size = align(size);
        Block *block = findFreeBlock(size);

        if (!block) return nullptr;

        splitBlock(block, size);
        block->is_free = false;

        return reinterpret_cast<uint8_t *>(block) + sizeof(Block);
    }

    static void deallocate(void *ptr) {
        if (!ptr) return;

        Block *block = reinterpret_cast<Block *>(
            static_cast<uint8_t *>(ptr) - sizeof(Block)
        );
        block->is_free = true;
    }

    void coalesce() const {
        Block *current = free_list;

        while (current && current->next) {
            if (current->is_free && current->next->is_free) {
                current->size += sizeof(Block) + current->next->size;
                current->next = current->next->next;

                if (current->next) {
                    current->next->prev = current;
                }
            } else {
                current = current->next;
            }
        }
    }

    size_t getFreeMemory() const {
        size_t total = 0;
        Block *current = free_list;

        while (current) {
            if (current->is_free) {
                total += current->size;
            }
            current = current->next;
        }

        return total;
    }
};
