#ifndef ADAPTIVE_MEMORY_ALLOCATOR_H
#define ADAPTIVE_MEMORY_ALLOCATOR_H

#include "MemoryAllocator.h"
#include "MemoryProfiler.h"
#include "MemoryLeakDetector.h"
#include <vector>
#include <memory>

class AdaptiveMemoryAllocator : public MemoryAllocator {
private:
    std::unique_ptr<MemoryProfiler> profiler;
    bool adaptiveMode;

    // Memory pools for common sizes
    struct MemoryPool {
        size_t blockSize;
        std::vector<size_t> freeBlocks;
        size_t totalBlocks;
        size_t usedBlocks;
    };
    std::vector<MemoryPool> memoryPools;

    // Adaptive strategy parameters
    struct AdaptiveParameters {
        double fragmentationThreshold;
        size_t poolCreationThreshold;
        size_t adaptationInterval;
        size_t operationsSinceLastAdaptation;
    } params;

public:
    AdaptiveMemoryAllocator(size_t totalSize);

    // Override base class allocation methods
    size_t allocate(size_t size) override;
    void deallocate(size_t address) override;

    // Adaptive features
    void enableAdaptiveMode(bool enable = true);
    void adjustParameters();
    void createMemoryPool(size_t blockSize, size_t blockCount);
    void optimizePools();

    // Statistics and monitoring
    MemoryProfiler::PerformanceMetrics getPerformanceMetrics() const;
    void printStatistics() const;

private:
    // Helper methods
    size_t allocateFromPool(size_t size);
    void adaptStrategy();
    void updatePoolUtilization();
    bool shouldCreatePool(size_t size) const;
    void cleanupUnusedPools();
};

#endif // ADAPTIVE_MEMORY_ALLOCATOR_H