#ifndef MEMORY_LEAK_DETECTOR_H
#define MEMORY_LEAK_DETECTOR_H

#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <sstream>

struct AllocationInfo {
    size_t size;
    std::chrono::steady_clock::time_point allocationTime;
    std::string callStack;
    int lineNumber;
    std::string fileName;
};

class MemoryLeakDetector {
public:
    static MemoryLeakDetector& getInstance() {
        static MemoryLeakDetector instance;
        return instance;
    }

    // Track allocations
    void recordAllocation(size_t address, size_t size, const char* file, int line);
    void recordDeallocation(size_t address);

    // Leak detection
    bool hasLeaks() const;
    void printLeaks() const;
    void printAllocationHistory() const;

    // Statistics
    size_t getTotalAllocations() const;
    size_t getCurrentlyAllocated() const;
    void printStatistics() const;

    // Reset detector
    void reset();

private:
    MemoryLeakDetector() = default;
    ~MemoryLeakDetector() = default;

    // Delete copy constructor and assignment operator
    MemoryLeakDetector(const MemoryLeakDetector&) = delete;
    MemoryLeakDetector& operator=(const MemoryLeakDetector&) = delete;

    std::unordered_map<size_t, AllocationInfo> activeAllocations;
    std::vector<std::pair<size_t, AllocationInfo>> allocationHistory;

    // Helper methods
    std::string captureCallStack() const;
    std::string formatBytes(size_t bytes) const;
    std::string getTimeString(const std::chrono::steady_clock::time_point& time) const;
};

// Macros for easy usage
#define RECORD_ALLOCATION(addr, size) \
    MemoryLeakDetector::getInstance().recordAllocation(addr, size, __FILE__, __LINE__)

#define RECORD_DEALLOCATION(addr) \
    MemoryLeakDetector::getInstance().recordDeallocation(addr)

#endif // MEMORY_LEAK_DETECTOR_H