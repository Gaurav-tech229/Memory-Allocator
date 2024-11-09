/*
#include "AdaptiveMemoryAllocator.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <random>

void printDetailedMemoryState(AdaptiveMemoryAllocator& allocator, const std::vector<size_t>& activeAddresses) {
    std::cout << "\nDetailed Memory State\n";
    std::cout << std::string(50, '=') << "\n";

    // Print active allocations
    std::cout << "\nActive Allocations:\n";
    for (size_t addr : activeAddresses) {
        std::cout << "Address: " << std::setw(8) << addr << " | ";
    }
    std::cout << "\n\nMemory Statistics:\n";
    std::cout << std::string(30, '-') << "\n";
    std::cout << "Total Memory: " << allocator.getTotalMemory() << " bytes\n";
    std::cout << "Free Memory: " << allocator.getTotalFreeMemory() << " bytes\n";
    std::cout << "Largest Free Block: " << allocator.getLargestFreeBlock() << " bytes\n";
    std::cout << "Fragmentation: " << std::fixed << std::setprecision(2)
        << (allocator.getFragmentationRatio() * 100) << "%\n";

    allocator.printStatistics();
}

void testMemoryAllocator() {
    try {
        const size_t MEMORY_SIZE = 1024 * 1024;  // 1MB
        AdaptiveMemoryAllocator allocator(MEMORY_SIZE);

        std::cout << "Initial state:\n";
        printDetailedMemoryState(allocator, {});

        std::vector<size_t> sizes = { 64, 128, 256, 512, 1024 };
        std::vector<size_t> addresses;

        // Test allocation patterns
        for (int i = 0; i < 20; i++) {
            std::cout << "\nOperation " << (i + 1) << ":\n";
            std::cout << std::string(20, '-') << "\n";

            size_t size = sizes[i % sizes.size()];
            try {
                size_t address = allocator.allocate(size);
                addresses.push_back(address);
                std::cout << "Allocated " << std::setw(6) << size
                    << " bytes at address " << address << "\n";

                // Random deallocation
                if (i > 5 && i % 3 == 0 && !addresses.empty()) {
                    size_t index = rand() % addresses.size();
                    size_t addr_to_free = addresses[index];
                    allocator.deallocate(addr_to_free);
                    std::cout << "Deallocated block at address " << addr_to_free << "\n";
                    addresses.erase(addresses.begin() + index);
                }

                // Print detailed state every 5 operations
                if (i % 5 == 0) {
                    printDetailedMemoryState(allocator, addresses);
                }

            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }

        // Cleanup
        std::cout << "\nCleaning up allocations...\n";
        for (size_t address : addresses) {
            allocator.deallocate(address);
            std::cout << "Freed address " << address << "\n";
        }

        std::cout << "\nFinal state:\n";
        printDetailedMemoryState(allocator, {});

    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "Memory Allocator Test Program\n";
    std::cout << "=============================\n\n";

    testMemoryAllocator();

    std::cout << "\nTest completed.\n";
    std::cin.get();
    return 0;
}
*/

#include "AdaptiveMemoryAllocator.h"
#include "MemoryLeakDetector.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>

// Helper function to print memory state
void printMemoryState(AdaptiveMemoryAllocator& allocator,
    const std::vector<size_t>& addresses,
    const std::string& description) {
    std::cout << "\n" << description << "\n";
    std::cout << std::string(50, '=') << "\n";

    // Print active allocations
    std::cout << "Active Allocations:\n";
    for (size_t addr : addresses) {
        std::cout << "Address: " << std::setw(8) << addr << " | ";
    }
    std::cout << "\n";

    // Print allocator statistics
    std::cout << "\nAllocator Statistics:\n";
    std::cout << "Total Memory: " << allocator.getTotalMemory() << " bytes\n";
    std::cout << "Free Memory: " << allocator.getTotalFreeMemory() << " bytes\n";
    std::cout << "Fragmentation: " << std::fixed << std::setprecision(2)
        << (allocator.getFragmentationRatio() * 100) << "%\n";

    // Print leak detector statistics
    auto& leakDetector = MemoryLeakDetector::getInstance();
    leakDetector.printStatistics();
}

// Test normal allocation/deallocation
void testNormalUsage(AdaptiveMemoryAllocator& allocator) {
    std::cout << "\nTesting Normal Usage\n";
    std::cout << std::string(50, '=') << "\n";

    std::vector<size_t> addresses;

    // Perform some allocations
    try {
        addresses.push_back(allocator.allocate(128));
        addresses.push_back(allocator.allocate(256));
        addresses.push_back(allocator.allocate(512));

        printMemoryState(allocator, addresses, "After allocations");

        // Deallocate all properly
        for (size_t addr : addresses) {
            allocator.deallocate(addr);
        }

        addresses.clear();
        printMemoryState(allocator, addresses, "After proper cleanup");

    }
    catch (const std::exception& e) {
        std::cerr << "Error in normal usage test: " << e.what() << "\n";
    }
}

// Test memory leaks
void testMemoryLeaks(AdaptiveMemoryAllocator& allocator) {
    std::cout << "\nTesting Memory Leaks\n";
    std::cout << std::string(50, '=') << "\n";

    std::vector<size_t> addresses;

    try {
        // Create intentional leaks
        addresses.push_back(allocator.allocate(1024));  // Will be leaked
        addresses.push_back(allocator.allocate(2048));  // Will be deallocated
        addresses.push_back(allocator.allocate(512));   // Will be leaked

        printMemoryState(allocator, addresses, "After creating potential leaks");

        // Only deallocate one allocation
        allocator.deallocate(addresses[1]);
        addresses.erase(addresses.begin() + 1);

        printMemoryState(allocator, addresses, "After partial cleanup");

        // Check for leaks
        auto& leakDetector = MemoryLeakDetector::getInstance();
        if (leakDetector.hasLeaks()) {
            std::cout << "\nLeak Detection Results:\n";
            leakDetector.printLeaks();
            leakDetector.printAllocationHistory();
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Error in memory leak test: " << e.what() << "\n";
    }
}

// Test stress conditions
void testStressConditions(AdaptiveMemoryAllocator& allocator) {
    std::cout << "\nTesting Stress Conditions\n";
    std::cout << std::string(50, '=') << "\n";

    std::vector<size_t> addresses;
    const int NUM_ITERATIONS = 100;

    try {
        // Rapid allocation/deallocation
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            addresses.push_back(allocator.allocate(64));

            if (i % 3 == 0 && !addresses.empty()) {
                allocator.deallocate(addresses.back());
                addresses.pop_back();
            }

            if (i % 10 == 0) {
                printMemoryState(allocator, addresses,
                    "Stress test iteration " + std::to_string(i));
            }
        }

        // Cleanup remaining allocations
        for (size_t addr : addresses) {
            allocator.deallocate(addr);
        }
        addresses.clear();

        printMemoryState(allocator, addresses, "After stress test cleanup");

    }
    catch (const std::exception& e) {
        std::cerr << "Error in stress test: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "Memory Allocator Test Program with Leak Detection\n";
    std::cout << "==============================================\n";

    try {
        // Create allocator with 10MB of memory
        AdaptiveMemoryAllocator allocator(10 * 1024 * 1024);

        // Run tests
        testNormalUsage(allocator);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        testMemoryLeaks(allocator);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        testStressConditions(allocator);

        // Final leak check
        auto& leakDetector = MemoryLeakDetector::getInstance();
        std::cout << "\nFinal Leak Check:\n";
        std::cout << std::string(50, '=') << "\n";

        if (leakDetector.hasLeaks()) {
            std::cout << "WARNING: Memory leaks detected in final check!\n";
            leakDetector.printLeaks();
        }
        else {
            std::cout << "No memory leaks detected in final check.\n";
        }

        // Print final statistics
        std::cout << "\nFinal Statistics:\n";
        leakDetector.printStatistics();

    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
    }

    std::cout << "\nTest program completed. Press Enter to exit...";
    std::cin.get();
    return 0;
}