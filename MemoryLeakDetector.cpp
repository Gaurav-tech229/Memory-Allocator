#include "MemoryLeakDetector.h"
#include <iostream>
#include <iomanip>
#include <ctime>

void MemoryLeakDetector::recordAllocation(size_t address, size_t size, const char* file, int line) {
    AllocationInfo info{
        size,
        std::chrono::steady_clock::now(),
        captureCallStack(),
        line,
        std::string(file)
    };

    activeAllocations[address] = info;
    allocationHistory.emplace_back(address, info);
}

void MemoryLeakDetector::recordDeallocation(size_t address) {
    auto it = activeAllocations.find(address);
    if (it != activeAllocations.end()) {
        activeAllocations.erase(it);
    }
    else {
        std::cerr << "Warning: Attempting to deallocate untracked address: " << address << "\n";
    }
}

bool MemoryLeakDetector::hasLeaks() const {
    return !activeAllocations.empty();
}

void MemoryLeakDetector::printLeaks() const {
    if (activeAllocations.empty()) {
        std::cout << "No memory leaks detected.\n";
        return;
    }

    std::cout << "\nMemory Leaks Detected!\n";
    std::cout << std::string(50, '=') << "\n";

    size_t totalLeaked = 0;
    for (const auto& [address, info] : activeAllocations) {
        totalLeaked += info.size;

        std::cout << "Leak at address " << address << ":\n"
            << "  Size: " << formatBytes(info.size) << "\n"
            << "  Allocated in: " << info.fileName << ":" << info.lineNumber << "\n"
            << "  Time: " << getTimeString(info.allocationTime) << "\n"
            << "  Call Stack:\n" << info.callStack << "\n\n";
    }

    std::cout << "Total memory leaked: " << formatBytes(totalLeaked) << "\n";
    std::cout << std::string(50, '=') << "\n";
}

void MemoryLeakDetector::printAllocationHistory() const {
    std::cout << "\nAllocation History:\n";
    std::cout << std::string(50, '=') << "\n";

    for (const auto& [address, info] : allocationHistory) {
        std::cout << "Address: " << address << "\n"
            << "  Size: " << formatBytes(info.size) << "\n"
            << "  Location: " << info.fileName << ":" << info.lineNumber << "\n"
            << "  Time: " << getTimeString(info.allocationTime) << "\n\n";
    }
}

void MemoryLeakDetector::printStatistics() const {
    std::cout << "\nMemory Leak Detector Statistics:\n";
    std::cout << std::string(30, '-') << "\n";
    std::cout << "Total number of allocations: " << getTotalAllocations() << "\n"
        << "Active allocations: " << activeAllocations.size() << "\n"
        << "Total memory currently allocated: " << formatBytes(getCurrentlyAllocated()) << "\n"
        << "Average allocation size: ";

    if (getTotalAllocations() > 0) {
        double avgSize = static_cast<double>(getCurrentlyAllocated()) /
            activeAllocations.size();
        std::cout << formatBytes(static_cast<size_t>(avgSize));
    }
    else {
        std::cout << "N/A";
    }
    std::cout << "\n";

    // Add leak summary
    if (hasLeaks()) {
        std::cout << "WARNING: Memory leaks detected!\n"
            << "Number of leaks: " << activeAllocations.size() << "\n"
            << "Total leaked memory: " << formatBytes(getCurrentlyAllocated()) << "\n";
    }
    else {
        std::cout << "No memory leaks detected\n";
    }

    std::cout << std::string(30, '-') << "\n";
}

void MemoryLeakDetector::reset() {
    activeAllocations.clear();
    allocationHistory.clear();
}

std::string MemoryLeakDetector::captureCallStack() const {
    // Basic call stack capture (can be enhanced with platform-specific implementations)
    return "Call stack capture not implemented\n";
}

std::string MemoryLeakDetector::formatBytes(size_t bytes) const {
    const char* units[] = { "B", "KB", "MB", "GB" };
    int i = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && i < 3) {
        size /= 1024;
        i++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[i];
    return ss.str();
}

std::string MemoryLeakDetector::getTimeString(
    const std::chrono::steady_clock::time_point& time) const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time);
    return std::to_string(duration.count()) + "ms ago";
}


size_t MemoryLeakDetector::getTotalAllocations() const {
    return allocationHistory.size();
}

size_t MemoryLeakDetector::getCurrentlyAllocated() const {
    size_t totalBytes = 0;
    for (const auto& [address, info] : activeAllocations) {
        totalBytes += info.size;
    }
    return totalBytes;
}