#include "MemoryAllocator.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

MemoryAllocator::MemoryAllocator(size_t size, AllocationStrategy allocationStrategy)
    : totalMemorySize(size), strategy(allocationStrategy) {
    // Initialize with one large free block
    MemoryBlock initialBlock(0, size);
    memoryBlocks.push_back(initialBlock);
    addressMap[0] = memoryBlocks.begin();
}

size_t MemoryAllocator::allocate(size_t size) {
    if (size == 0) {
        throw AllocationFailedException("Cannot allocate zero bytes");
    }

    auto blockIt = findSuitableBlock(size);
    if (blockIt == memoryBlocks.end()) {
        throw AllocationFailedException("No suitable memory block found");
    }

    size_t allocatedAddress = blockIt->address;

    // If block is larger than needed, split it
    if (blockIt->size > size) {
        splitBlock(blockIt, size);
    }

    blockIt->isFree = false;
    return allocatedAddress;
}

void MemoryAllocator::deallocate(size_t address) {
    auto it = addressMap.find(address);
    if (it == addressMap.end()) {
        throw InvalidAddressException("Invalid address for deallocation");
    }

    it->second->isFree = true;
    mergeFreeBlocks();
}

std::list<MemoryBlock>::iterator MemoryAllocator::findSuitableBlock(size_t size) {
    switch (strategy) {
    case AllocationStrategy::FIRST_FIT: {
        return std::find_if(memoryBlocks.begin(), memoryBlocks.end(),
            [size](const MemoryBlock& block) {
                return block.isFree && block.size >= size;
            });
    }

    case AllocationStrategy::BEST_FIT: {
        auto bestFit = memoryBlocks.end();
        size_t smallestDifference = SIZE_MAX;

        for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
            if (it->isFree && it->size >= size) {
                size_t difference = it->size - size;
                if (difference < smallestDifference) {
                    smallestDifference = difference;
                    bestFit = it;
                }
            }
        }
        return bestFit;
    }

    case AllocationStrategy::WORST_FIT: {
        auto worstFit = memoryBlocks.end();
        size_t largestDifference = 0;

        for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
            if (it->isFree && it->size >= size) {
                size_t difference = it->size - size;
                if (difference > largestDifference) {
                    largestDifference = difference;
                    worstFit = it;
                }
            }
        }
        return worstFit;
    }

    default:
        return memoryBlocks.end();
    }
}

void MemoryAllocator::splitBlock(std::list<MemoryBlock>::iterator block, size_t size) {
    size_t remainingSize = block->size - size;
    if (remainingSize > 0) {
        MemoryBlock newBlock(block->address + size, remainingSize);
        auto newBlockIt = memoryBlocks.insert(std::next(block), newBlock);
        addressMap[newBlock.address] = newBlockIt;
        block->size = size;
    }
}

void MemoryAllocator::mergeFreeBlocks() {
    auto it = memoryBlocks.begin();
    while (it != memoryBlocks.end() && std::next(it) != memoryBlocks.end()) {
        auto nextIt = std::next(it);
        if (it->isFree && nextIt->isFree) {
            // Merge blocks
            it->size += nextIt->size;
            addressMap.erase(nextIt->address);
            memoryBlocks.erase(nextIt);
        }
        else {
            ++it;
        }
    }
}

void MemoryAllocator::setAllocationStrategy(AllocationStrategy newStrategy) {
    strategy = newStrategy;
}

double MemoryAllocator::getFragmentationRatio() const {
    size_t totalFreeMemory = getTotalFreeMemory();
    size_t largestFreeBlock = getLargestFreeBlock();

    if (totalFreeMemory == 0) return 0.0;
    return 1.0 - (static_cast<double>(largestFreeBlock) / totalFreeMemory);
}

size_t MemoryAllocator::getLargestFreeBlock() const {
    size_t largest = 0;
    for (const auto& block : memoryBlocks) {
        if (block.isFree && block.size > largest) {
            largest = block.size;
        }
    }
    return largest;
}

size_t MemoryAllocator::getTotalFreeMemory() const {
    size_t total = 0;
    for (const auto& block : memoryBlocks) {
        if (block.isFree) {
            total += block.size;
        }
    }
    return total;
}

void MemoryAllocator::printMemoryMap() const {
    std::cout << "\nMemory Map:\n";
    std::cout << std::string(50, '-') << "\n";

    for (const auto& block : memoryBlocks) {
        std::cout << "Address: " << std::setw(8) << block.address
            << " | Size: " << std::setw(8) << block.size
            << " | Status: " << (block.isFree ? "Free" : "Allocated") << "\n";
    }

    std::cout << std::string(50, '-') << "\n";
    std::cout << "Total Memory: " << totalMemorySize << " bytes\n";
    std::cout << "Free Memory: " << getTotalFreeMemory() << " bytes\n";
    std::cout << "Fragmentation Ratio: " << std::fixed << std::setprecision(2)
        << (getFragmentationRatio() * 100) << "%\n";
}