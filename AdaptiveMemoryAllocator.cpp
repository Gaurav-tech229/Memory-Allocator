#include "AdaptiveMemoryAllocator.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <chrono>

AdaptiveMemoryAllocator::AdaptiveMemoryAllocator(size_t totalSize)
    : MemoryAllocator(totalSize), adaptiveMode(true)
{
    profiler = std::make_unique<MemoryProfiler>(this);

    // Initialize adaptive parameters
    params = {
        .fragmentationThreshold = 0.3,    // 30% fragmentation trigger
        .poolCreationThreshold = 100,     // Minimum allocations for pool creation
        .adaptationInterval = 1000,       // Check adaptation every 1000 operations
        .operationsSinceLastAdaptation = 0
    };
}

size_t AdaptiveMemoryAllocator::allocate(size_t size) {
    auto startTime = std::chrono::steady_clock::now();
    size_t address;

    if (adaptiveMode) {
        // Try pool allocation first
        address = allocateFromPool(size);
        if (address != 0) {
            profiler->recordAllocation(size, address);
            return address;
        }

        // Check if we should create a new pool
        if (shouldCreatePool(size)) {
            createMemoryPool(size, 10); // Create pool with 10 blocks
            address = allocateFromPool(size);
            if (address != 0) {
                profiler->recordAllocation(size, address);
                return address;
            }
        }
    }

    // Fall back to regular allocation
    address = MemoryAllocator::allocate(size);

    // Record allocation and update statistics
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime).count();

    profiler->recordAllocation(size, address);

    // Check if adaptation is needed
    params.operationsSinceLastAdaptation++;
    if (params.operationsSinceLastAdaptation >= params.adaptationInterval) {
        adaptStrategy();
    }

    RECORD_ALLOCATION(address, size);

    return address;
}

void AdaptiveMemoryAllocator::deallocate(size_t address) {

    RECORD_DEALLOCATION(address);
    // Find if the address belongs to a pool
    for (auto& pool : memoryPools) {
        size_t poolStart = pool.freeBlocks.front();
        size_t poolEnd = poolStart + (pool.totalBlocks * pool.blockSize);

        if (address >= poolStart && address < poolEnd) {
            pool.freeBlocks.push_back(address);
            pool.usedBlocks--;
            profiler->recordDeallocation(address);
            return;
        }
    }

    // Regular deallocation
    MemoryAllocator::deallocate(address);
    profiler->recordDeallocation(address);

    // Update pool utilization
    updatePoolUtilization();
}

void AdaptiveMemoryAllocator::enableAdaptiveMode(bool enable) {
    adaptiveMode = enable;
    if (enable) {
        // Reset adaptation parameters
        params.operationsSinceLastAdaptation = 0;
        adaptStrategy();
    }
}

void AdaptiveMemoryAllocator::adjustParameters() {
    auto metrics = profiler->getPerformanceMetrics();

    // Adjust fragmentation threshold based on performance
    if (metrics.hitRate < 0.8) {
        params.fragmentationThreshold *= 1.1; // Relax threshold
    }
    else if (metrics.hitRate > 0.95) {
        params.fragmentationThreshold *= 0.9; // Tighten threshold
    }

    // Adjust pool creation threshold
    if (metrics.failedAllocations > 100) {
        params.poolCreationThreshold *= 0.9; // Make pool creation more aggressive
    }

    // Adjust adaptation interval
    if (metrics.averageAllocationTime > 1000) { // 1ms
        params.adaptationInterval *= 1.2; // Reduce adaptation frequency
    }
    else {
        params.adaptationInterval *= 0.8; // Increase adaptation frequency
    }
}

void AdaptiveMemoryAllocator::createMemoryPool(size_t blockSize, size_t blockCount) {
    size_t totalSize = blockSize * blockCount;
    size_t baseAddress = MemoryAllocator::allocate(totalSize);

    if (baseAddress == 0) return; // Failed to allocate pool

    MemoryPool newPool{ blockSize, {}, blockCount, 0 };

    // Initialize free blocks
    for (size_t i = 0; i < blockCount; i++) {
        newPool.freeBlocks.push_back(baseAddress + (i * blockSize));
    }

    memoryPools.push_back(std::move(newPool));
}

void AdaptiveMemoryAllocator::optimizePools() {
    auto prediction = profiler->predictNextAllocation();

    // Remove underutilized pools
    cleanupUnusedPools();

    // Create or resize pools based on predictions
    for (size_t size : prediction.recommendedPoolSizes) {
        bool poolExists = false;
        for (const auto& pool : memoryPools) {
            if (pool.blockSize == size) {
                poolExists = true;
                break;
            }
        }

        if (!poolExists) {
            size_t count = std::max(size_t(5),
                size_t(prediction.confidence * 20)); // Scale with confidence
            createMemoryPool(size, count);
        }
    }
}

size_t AdaptiveMemoryAllocator::allocateFromPool(size_t size) {
    // Find suitable pool
    for (auto& pool : memoryPools) {
        if (pool.blockSize >= size && !pool.freeBlocks.empty()) {
            size_t address = pool.freeBlocks.back();
            pool.freeBlocks.pop_back();
            pool.usedBlocks++;
            return address;
        }
    }
    return 0; // No suitable pool found
}

void AdaptiveMemoryAllocator::adaptStrategy() {
    if (!adaptiveMode) return;

    auto metrics = profiler->getPerformanceMetrics();
    auto prediction = profiler->predictNextAllocation();

    // Update allocation strategy
    if (metrics.fragmentationRatio > params.fragmentationThreshold) {
        setAllocationStrategy(prediction.recommendedStrategy);
    }

    // Optimize memory pools
    optimizePools();

    // Adjust parameters based on performance
    adjustParameters();

    // Reset counter
    params.operationsSinceLastAdaptation = 0;
}

void AdaptiveMemoryAllocator::updatePoolUtilization() {
    for (auto& pool : memoryPools) {
        double utilization = static_cast<double>(pool.usedBlocks) / pool.totalBlocks;
        if (utilization < 0.2) { // Less than 20% utilization
            // Mark for potential cleanup
            pool.totalBlocks = 0; // Use as a flag for cleanup
        }
    }
}

bool AdaptiveMemoryAllocator::shouldCreatePool(size_t size) const {
    return profiler->shouldCreatePoolForSize(size, params.poolCreationThreshold);
}

void AdaptiveMemoryAllocator::cleanupUnusedPools() {
    // Remove pools marked for cleanup
    memoryPools.erase(
        std::remove_if(memoryPools.begin(), memoryPools.end(),
            [](const MemoryPool& pool) { return pool.totalBlocks == 0; }),
        memoryPools.end());
}

void AdaptiveMemoryAllocator::printStatistics() const {
    std::cout << "\nAdaptive Memory Allocator Statistics:\n";
    std::cout << std::string(50, '-') << '\n';

    // Print general metrics
    auto metrics = profiler->getPerformanceMetrics();
    std::cout << "Performance Metrics:\n"
        << "  Fragmentation Ratio: " << std::fixed << std::setprecision(2)
        << (metrics.fragmentationRatio * 100) << "%\n"
        << "  Average Allocation Time: " << metrics.averageAllocationTime << "μs\n"
        << "  Hit Rate: " << (metrics.hitRate * 100) << "%\n"
        << "  Failed Allocations: " << metrics.failedAllocations << "\n\n";

    // Print pool statistics
    std::cout << "Memory Pools:\n";
    for (const auto& pool : memoryPools) {
        double utilization = static_cast<double>(pool.usedBlocks) / pool.totalBlocks;
        std::cout << "  Size: " << pool.blockSize << " bytes"
            << "  Utilization: " << (utilization * 100) << "%"
            << "  Blocks: " << pool.usedBlocks << "/" << pool.totalBlocks << "\n";
    }

    // Print adaptive parameters
    std::cout << "\nAdaptive Parameters:\n"
        << "  Fragmentation Threshold: " << (params.fragmentationThreshold * 100) << "%\n"
        << "  Pool Creation Threshold: " << params.poolCreationThreshold << " allocations\n"
        << "  Adaptation Interval: " << params.adaptationInterval << " operations\n";
}