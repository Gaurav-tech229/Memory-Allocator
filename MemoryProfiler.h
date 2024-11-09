#ifndef MEMORY_PROFILER_H
#define MEMORY_PROFILER_H

#include <chrono>
#include <deque>
#include <unordered_map>
#include <vector>
#include <map>
#include <memory>
#include "MemoryAllocator.h"

// Forward declaration
class MemoryAllocator;

struct AllocationRecord {
    size_t size;
    size_t address;
    std::chrono::steady_clock::time_point allocationTime;
    std::chrono::steady_clock::time_point deallocationTime;
    bool isActive;
    size_t poolId;  // For pool-based allocations
};

struct PoolStatistics {
    size_t size;
    size_t utilization;
    size_t fragmentationCount;
    double hitRate;
};

class MemoryProfiler {
public:
    explicit MemoryProfiler(MemoryAllocator* allocator);

    // Allocation tracking
    void recordAllocation(size_t size, size_t address, size_t poolId = 0);
    void recordDeallocation(size_t address);

    // Pattern analysis
    struct AllocationPattern {
        std::vector<size_t> commonSizes;
        double averageLifetime;
        std::map<size_t, double> sizeDistribution;
        std::vector<std::pair<size_t, size_t>> hotSpots;  // Changed from vector<size_t>
    };

    AllocationPattern analyzePatterns() const;

    // Prediction system
    struct Prediction {
        size_t nextLikelySize;
        AllocationStrategy recommendedStrategy;
        std::vector<size_t> recommendedPoolSizes;
        double confidence;
    };

    Prediction predictNextAllocation() const;

    // Performance metrics
    struct PerformanceMetrics {
        double fragmentationRatio;
        double averageAllocationTime;
        double hitRate;
        size_t failedAllocations;
        std::map<AllocationStrategy, double> strategyEfficiency;
    };

    PerformanceMetrics getPerformanceMetrics() const;

    // Pool management recommendations
    struct PoolRecommendation {
        std::vector<size_t> optimalSizes;
        std::vector<size_t> counts;
        double expectedImprovement;
    };

    PoolRecommendation recommendPoolConfiguration() const;

    size_t getTotalAllocations() const {
        return allocationHistory.size();
    }

    bool shouldCreatePoolForSize(size_t size, size_t threshold) const {
        auto pattern = analyzePatterns();
        size_t totalAllocations = getTotalAllocations();

        auto it = pattern.sizeDistribution.find(size);
        if (it != pattern.sizeDistribution.end()) {
            return static_cast<size_t>(it->second * totalAllocations) >= threshold;
        }
        return false;
    }

private:
    MemoryAllocator* allocator;
    std::deque<AllocationRecord> allocationHistory;
    std::map<size_t, std::vector<double>> lifetimeDistribution;
    std::map<size_t, size_t> sizeFrequency;
    std::map<AllocationStrategy, PerformanceMetrics> strategyMetrics;

    // Pattern analysis helpers
    void updateLifetimeStats(const AllocationRecord& record);
    void updateSizeFrequency(size_t size);
    void updateStrategyMetrics(AllocationStrategy strategy,
        const PerformanceMetrics& metrics);

    // Prediction helpers
    double calculatePatternConfidence(const std::vector<size_t>& sizes) const;
    AllocationStrategy determineOptimalStrategy(const AllocationPattern& pattern) const;

    // Statistical analysis
    double calculateMovingAverage(const std::vector<double>& values, size_t window) const;

    // Change the declaration to match the implementation
    std::vector<std::pair<size_t, size_t>> identifyHotSpots(
        const std::deque<AllocationRecord>& records) const;  // Changed from vector to deque
};

#endif // MEMORY_PROFILER_H