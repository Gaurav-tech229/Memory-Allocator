#include "MemoryProfiler.h"
#include "MemoryAllocator.h"
#include <algorithm>
#include <numeric>
#include <cmath>

MemoryProfiler::MemoryProfiler(MemoryAllocator* alloc) : allocator(alloc) {}

void MemoryProfiler::recordAllocation(size_t size, size_t address, size_t poolId) {
    AllocationRecord record{
        size,
        address,
        std::chrono::steady_clock::now(),
        std::chrono::steady_clock::time_point(),
        true,
        poolId
    };

    allocationHistory.push_back(record);
    updateSizeFrequency(size);

    // Limit history size to prevent excessive memory usage
    if (allocationHistory.size() > 10000) {
        allocationHistory.pop_front();
    }
}

void MemoryProfiler::recordDeallocation(size_t address) {
    auto now = std::chrono::steady_clock::now();

    for (auto& record : allocationHistory) {
        if (record.address == address && record.isActive) {
            record.isActive = false;
            record.deallocationTime = now;
            updateLifetimeStats(record);
            break;
        }
    }
}

MemoryProfiler::AllocationPattern MemoryProfiler::analyzePatterns() const {
    AllocationPattern pattern;

    // Analyze common sizes
    std::vector<std::pair<size_t, size_t>> sizeFreqVec(
        sizeFrequency.begin(), sizeFrequency.end());
    std::sort(sizeFreqVec.begin(), sizeFreqVec.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Extract top N common sizes
    const size_t N = 5;
    for (size_t i = 0; i < std::min(N, sizeFreqVec.size()); ++i) {
        pattern.commonSizes.push_back(sizeFreqVec[i].first);
    }

    // Calculate average lifetime
    double totalLifetime = 0;
    size_t count = 0;
    for (const auto& [size, lifetimes] : lifetimeDistribution) {
        if (!lifetimes.empty()) {
            totalLifetime += std::accumulate(lifetimes.begin(), lifetimes.end(), 0.0);
            count += lifetimes.size();
        }
    }
    pattern.averageLifetime = count > 0 ? totalLifetime / count : 0;

    // Calculate size distribution
    size_t totalAllocations = std::accumulate(
        sizeFreqVec.begin(), sizeFreqVec.end(), 0ULL,
        [](size_t sum, const auto& pair) { return sum + pair.second; });

    for (const auto& [size, freq] : sizeFreqVec) {
        pattern.sizeDistribution[size] = static_cast<double>(freq) / totalAllocations;
    }

    // Fix the hotspots assignment
    pattern.hotSpots = identifyHotSpots(allocationHistory);

    return pattern;
}

MemoryProfiler::Prediction MemoryProfiler::predictNextAllocation() const {
    Prediction prediction;
    auto pattern = analyzePatterns();

    // Predict next size based on most common sizes and recent history
    if (!pattern.commonSizes.empty()) {
        prediction.nextLikelySize = pattern.commonSizes[0];
    }

    // Determine best strategy based on current patterns
    prediction.recommendedStrategy = determineOptimalStrategy(pattern);

    // Calculate prediction confidence
    prediction.confidence = calculatePatternConfidence(pattern.commonSizes);

    // Recommend pool sizes based on size distribution
    for (const auto& [size, frequency] : pattern.sizeDistribution) {
        if (frequency > 0.1) {  // 10% threshold
            prediction.recommendedPoolSizes.push_back(size);
        }
    }

    return prediction;
}

MemoryProfiler::PerformanceMetrics MemoryProfiler::getPerformanceMetrics() const {
    PerformanceMetrics metrics;

    // Calculate current fragmentation ratio
    metrics.fragmentationRatio = allocator->getFragmentationRatio();

    // Calculate average allocation time
    auto calculateAvgTime = [](const auto& records) {
        std::vector<double> times;
        for (size_t i = 1; i < records.size(); ++i) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                records[i].allocationTime - records[i - 1].allocationTime).count();
            times.push_back(duration);
        }
        return times.empty() ? 0.0 :
            std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        };

    metrics.averageAllocationTime = calculateAvgTime(allocationHistory);

    // Calculate hit rate (successful allocations / total attempts)
    size_t successful = 0;
    for (const auto& record : allocationHistory) {
        if (record.isActive || record.deallocationTime != std::chrono::steady_clock::time_point()) {
            successful++;
        }
    }
    metrics.hitRate = static_cast<double>(successful) / allocationHistory.size();

    // Count failed allocations
    metrics.failedAllocations = allocationHistory.size() - successful;

    // Calculate strategy efficiency
    for (const auto& [strategy, stratMetrics] : strategyMetrics) {
        metrics.strategyEfficiency[strategy] =
            (stratMetrics.hitRate * 0.4 +
                (1.0 - stratMetrics.fragmentationRatio) * 0.4 +
                (1.0 / (1.0 + stratMetrics.averageAllocationTime)) * 0.2);
    }

    return metrics;
}

MemoryProfiler::PoolRecommendation MemoryProfiler::recommendPoolConfiguration() const {
    PoolRecommendation recommendation;
    auto pattern = analyzePatterns();

    // Group similar sizes
    std::map<size_t, size_t> sizeGroups;
    for (const auto& [size, freq] : pattern.sizeDistribution) {
        // Round to nearest power of 2
        size_t roundedSize = std::pow(2, std::ceil(std::log2(size)));
        sizeGroups[roundedSize] += freq * 100;  // Convert frequency to percentage
    }

    // Generate recommendations
    for (const auto& [size, count] : sizeGroups) {
        if (count >= 5) {  // 5% threshold
            recommendation.optimalSizes.push_back(size);
            recommendation.counts.push_back(count);
        }
    }

    // Calculate expected improvement
    double currentFragmentation = allocator->getFragmentationRatio();
    double expectedFragmentation = currentFragmentation * 0.7;  // Estimate 30% improvement
    recommendation.expectedImprovement =
        (currentFragmentation - expectedFragmentation) / currentFragmentation * 100;

    return recommendation;
}

// Private helper methods
void MemoryProfiler::updateLifetimeStats(const AllocationRecord& record) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        record.deallocationTime - record.allocationTime).count();
    lifetimeDistribution[record.size].push_back(duration);
}

void MemoryProfiler::updateSizeFrequency(size_t size) {
    sizeFrequency[size]++;
}

void MemoryProfiler::updateStrategyMetrics(
    AllocationStrategy strategy, const PerformanceMetrics& metrics) 
{
    strategyMetrics[strategy] = metrics;
}

double MemoryProfiler::calculatePatternConfidence(
    const std::vector<size_t>& sizes) const {
    if (sizes.empty()) return 0.0;

    // Calculate confidence based on frequency distribution
    size_t totalAllocs = std::accumulate(
        sizeFrequency.begin(), sizeFrequency.end(), 0ULL,
        [](size_t sum, const auto& pair) { return sum + pair.second; });

    size_t commonAllocs = 0;
    for (size_t size : sizes) {
        auto it = sizeFrequency.find(size);
        if (it != sizeFrequency.end()) {
            commonAllocs += it->second;
        }
    }

    return static_cast<double>(commonAllocs) / totalAllocs;
}

AllocationStrategy MemoryProfiler::determineOptimalStrategy(
    const AllocationPattern& pattern) const {
    // Calculate strategy scores based on pattern characteristics
    double firstFitScore = 0.0;
    double bestFitScore = 0.0;
    double worstFitScore = 0.0;

    // Score based on size distribution
    double sizeVariance = 0.0;
    for (const auto& [size, freq] : pattern.sizeDistribution) {
        sizeVariance += std::pow(size - pattern.commonSizes[0], 2) * freq;
    }

    // Adjust scores based on pattern characteristics
    if (sizeVariance < 1000) {
        bestFitScore += 0.5;  // Best fit works well with consistent sizes
    }
    else {
        firstFitScore += 0.3;  // First fit is more flexible with varying sizes
    }

    // Consider fragmentation
    if (pattern.hotSpots.size() > 5) {
        worstFitScore += 0.4;  // Worst fit can help with many hot spots
    }

    // Consider lifetime
    if (pattern.averageLifetime < 1000) {  // Short-lived allocations
        firstFitScore += 0.4;
    }
    else {
        bestFitScore += 0.3;
    }

    // Return the strategy with highest score
    if (firstFitScore >= bestFitScore && firstFitScore >= worstFitScore) {
        return AllocationStrategy::FIRST_FIT;
    }
    else if (bestFitScore >= worstFitScore) {
        return AllocationStrategy::BEST_FIT;
    }
    else {
        return AllocationStrategy::WORST_FIT;
    }
}

std::vector<std::pair<size_t, size_t>> MemoryProfiler::identifyHotSpots(
    const std::deque<AllocationRecord>& records) const {
    std::map<size_t, size_t> addressFrequency;
    const size_t REGION_SIZE = 4096;  // 4KB regions

    // Track allocation frequency by memory regions
    for (const auto& record : records) {
        size_t region = record.address / REGION_SIZE;
        addressFrequency[region]++;
    }

    // Convert to vector and sort by frequency
    std::vector<std::pair<size_t, size_t>> hotSpots(
        addressFrequency.begin(), addressFrequency.end());
    std::sort(hotSpots.begin(), hotSpots.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Return top hot spots
    const size_t MAX_HOTSPOTS = 10;
    if (hotSpots.size() > MAX_HOTSPOTS) {
        hotSpots.resize(MAX_HOTSPOTS);
    }

    return hotSpots;
}

// Calculate moving averages
double MemoryProfiler::calculateMovingAverage(
    const std::vector<double>& values, size_t window) const {
    if (values.empty() || window == 0) return 0.0;

    size_t effectiveWindow = std::min(window, values.size());
    double sum = 0.0;

    // Calculate sum of last 'window' elements
    for (size_t i = values.size() - effectiveWindow; i < values.size(); ++i) {
        sum += values[i];
    }

    return sum / effectiveWindow;
}