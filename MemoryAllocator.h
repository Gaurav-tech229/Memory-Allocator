#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <vector>
#include <list>
#include <unordered_map>
#include <cstddef>
#include <string>
#include <stdexcept>

enum class AllocationStrategy {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
};

struct MemoryBlock {
    size_t address;
    size_t size;
    bool isFree;

    MemoryBlock(size_t addr, size_t sz, bool free = true)
        : address(addr), size(sz), isFree(free) {}
};

class MemoryAllocator {
private:
    std::list<MemoryBlock> memoryBlocks;
    std::unordered_map<size_t, std::list<MemoryBlock>::iterator> addressMap;
    size_t totalMemorySize;
    AllocationStrategy strategy;

    // Helper functions
    std::list<MemoryBlock>::iterator findSuitableBlock(size_t size);
    void splitBlock(std::list<MemoryBlock>::iterator block, size_t size);
    void mergeFreeBlocks();

   

public:
    MemoryAllocator(size_t size, AllocationStrategy allocationStrategy = AllocationStrategy::FIRST_FIT);
    ~MemoryAllocator() = default;

    // Main operations
    virtual size_t allocate(size_t size);
    virtual void deallocate(size_t address);
    void setAllocationStrategy(AllocationStrategy newStrategy);

    // Statistics and debugging
    double getFragmentationRatio() const;
    size_t getLargestFreeBlock() const;
    size_t getTotalFreeMemory() const;
    void printMemoryMap() const;

    // Exception classes
    class AllocationFailedException : public std::runtime_error {
    public:
        AllocationFailedException(const std::string& message)
            : std::runtime_error(message) {}
    };

    class InvalidAddressException : public std::runtime_error {
    public:
        InvalidAddressException(const std::string& message)
            : std::runtime_error(message) {}
    };

    size_t getTotalMemory() const { return totalMemorySize; }
};

#endif // MEMORY_ALLOCATOR_H