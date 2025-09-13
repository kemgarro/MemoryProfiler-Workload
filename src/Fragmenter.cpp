#include "WorkloadConfig.hpp"
#include "Utilities.hpp"
#include "Types.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>

namespace mp {

/**
 * Fragmenter module - Simulates memory fragmentation patterns
 * 
 * This module creates fragmentation by:
 * - Many small allocations interleaved with medium ones
 * - Shuffled deallocation patterns (not LIFO/FIFO)
 * - Maintaining an active set that oscillates
 * - Creating "sawtooth" patterns in memory usage
 */
class Fragmenter {
public:
    explicit Fragmenter(const WorkloadConfig& config) : config_(config) {}
    
    ModuleResult execute(uint32_t thread_id, uint64_t duration_ms) {
        ModuleResult result("Fragmenter");
        Timer timer;
        RNG rng(config_.seed + thread_id + 2000);  // Different seed offset
        
        // Active allocations - we'll shuffle these for non-ordered deallocation
        std::vector<Allocation> active_allocations;
        active_allocations.reserve(config_.getScaled(1000));
        
        uint64_t end_time = currentTimeMillis() + duration_ms;
        uint32_t cycle_count = 0;
        size_t peak_active = 0;
        
        while (currentTimeMillis() < end_time) {
            // Phase 1: Fill phase - create many small allocations
            uint32_t fill_count = rng.randInt(50, config_.getScaled(200));
            for (uint32_t i = 0; i < fill_count; ++i) {
                if (currentTimeMillis() >= end_time) break;
                
                size_t size = getFragmentationSize(rng, AllocationSize::SMALL);
                char* ptr = new char[size];
                active_allocations.push_back({ptr, size, AllocationSize::SMALL});
                result.stats.allocations++;
                result.stats.bytes_allocated += size;
            }
            
            // Phase 2: Interleave medium allocations
            uint32_t medium_count = rng.randInt(10, config_.getScaled(50));
            for (uint32_t i = 0; i < medium_count; ++i) {
                if (currentTimeMillis() >= end_time) break;
                
                size_t size = getFragmentationSize(rng, AllocationSize::MEDIUM);
                char* ptr = new char[size];
                active_allocations.push_back({ptr, size, AllocationSize::MEDIUM});
                result.stats.allocations++;
                result.stats.bytes_allocated += size;
            }
            
            // Phase 3: Shuffle and partial deallocation
            if (!active_allocations.empty()) {
                // Shuffle the allocation order to create fragmentation
                std::mt19937 shuffle_gen(rng.randInt(0, UINT32_MAX));
                std::shuffle(active_allocations.begin(), active_allocations.end(), shuffle_gen);
                
                // Deallocate 40-80% of allocations in shuffled order
                uint32_t dealloc_count = rng.randInt(
                    (active_allocations.size() * 4) / 10,
                    (active_allocations.size() * 8) / 10
                );
                
                for (uint32_t i = 0; i < dealloc_count && !active_allocations.empty(); ++i) {
                    Allocation alloc = active_allocations.back();
                    active_allocations.pop_back();
                    
                    delete[] alloc.ptr;
                    result.stats.deallocations++;
                    result.stats.bytes_deallocated += alloc.size;
                }
            }
            
            // Phase 4: Add some large allocations to create holes
            if (rng.randBool(0.3)) {
                uint32_t large_count = rng.randInt(1, config_.getScaled(10));
                for (uint32_t i = 0; i < large_count; ++i) {
                    if (currentTimeMillis() >= end_time) break;
                    
                    size_t size = getFragmentationSize(rng, AllocationSize::LARGE);
                    char* ptr = new char[size];
                    active_allocations.push_back({ptr, size, AllocationSize::LARGE});
                    result.stats.allocations++;
                    result.stats.bytes_allocated += size;
                }
            }
            
            // Update peak tracking
            peak_active = std::max(peak_active, active_allocations.size());
            result.stats.peak_memory = std::max(result.stats.peak_memory, 
                active_allocations.size() * 1024);  // Rough estimate
            
            cycle_count++;
            
            // Oscillate the active set size to create sawtooth pattern
            if (active_allocations.size() > config_.getScaled(500)) {
                // Reduce active set
                uint32_t reduce_count = rng.randInt(50, 200);
                for (uint32_t i = 0; i < reduce_count && !active_allocations.empty(); ++i) {
                    Allocation alloc = active_allocations.back();
                    active_allocations.pop_back();
                    
                    delete[] alloc.ptr;
                    result.stats.deallocations++;
                    result.stats.bytes_deallocated += alloc.size;
                }
            }
            
            // Small delay between cycles
            if (rng.randBool(0.3)) {
                sleepMillis(rng.randInt(1, 2));
            }
        }
        
        // Clean up all remaining allocations
        for (const Allocation& alloc : active_allocations) {
            delete[] alloc.ptr;
            result.stats.deallocations++;
            result.stats.bytes_deallocated += alloc.size;
        }
        active_allocations.clear();
        
        result.stats.duration_ms = timer.elapsedMillis();
        
        if (!config_.quiet) {
            std::cout << "Thread " << thread_id << " Fragmenter: " 
                      << result.stats.allocations << " allocs, "
                      << result.stats.deallocations << " deallocs, "
                      << peak_active << " peak active, "
                      << cycle_count << " cycles\n";
        }
        
        return result;
    }

private:
    const WorkloadConfig& config_;
    
    enum class AllocationSize {
        SMALL,   // 16-128 bytes
        MEDIUM,  // 128-2048 bytes  
        LARGE    // 2KB-32KB
    };
    
    struct Allocation {
        char* ptr;
        size_t size;
        AllocationSize size_class;
    };
    
    /**
     * Generate sizes that promote fragmentation
     */
    size_t getFragmentationSize(RNG& rng, AllocationSize size_class) {
        switch (size_class) {
            case AllocationSize::SMALL:
                // Small allocations that don't align well
                return rng.randSize(16, 128, config_.scale);
                
            case AllocationSize::MEDIUM:
                // Medium allocations that create holes
                return rng.randSize(128, 2048, config_.scale);
                
            case AllocationSize::LARGE:
                // Large allocations that span multiple pages
                return rng.randSize(2048, 32768, config_.scale);
                
            default:
                return 64;
        }
    }
};

/**
 * Factory function to create and execute Fragmenter
 */
ModuleResult runFragmenter(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms) {
    Fragmenter fragmenter(config);
    return fragmenter.execute(thread_id, duration_ms);
}

} // namespace mp