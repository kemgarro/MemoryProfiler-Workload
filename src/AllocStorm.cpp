#include "WorkloadConfig.hpp"
#include "Utilities.hpp"
#include "Types.hpp"
#include <vector>
#include <memory>
#include <iostream>

namespace mp {

/**
 * AllocStorm module - Creates burst allocations with mixed object types
 * 
 * This module generates memory allocation patterns that stress the allocator:
 * - Burst allocations with geometric and pseudo-random sizes
 * - Mix of trivial and non-trivial objects (Blob class)
 * - Non-ordered deallocations (not LIFO/FIFO)
 * - Mix of short and medium-lived objects
 */
class AllocStorm {
public:
    explicit AllocStorm(const WorkloadConfig& config) : config_(config) {}
    
    ModuleResult execute(uint32_t thread_id, uint64_t duration_ms) {
        ModuleResult result("AllocStorm");
        Timer timer;
        RNG rng(config_.seed + thread_id);
        
        std::vector<void*> allocations;
        std::vector<std::unique_ptr<Blob>> blobs;
        
        // Reserve space to avoid reallocations during bursts
        allocations.reserve(config_.burst_size * 2);
        blobs.reserve(config_.burst_size / 4);
        
        uint64_t end_time = currentTimeMillis() + duration_ms;
        uint32_t burst_count = 0;
        
        while (currentTimeMillis() < end_time) {
            // Determine burst size (varies to create different patterns)
            uint32_t burst_size = config_.getScaled(config_.burst_size);
            if (rng.randBool(0.3)) {
                burst_size = rng.randInt(1, burst_size / 2);  // Smaller bursts
            } else if (rng.randBool(0.1)) {
                burst_size = rng.randInt(burst_size, burst_size * 2);  // Larger bursts
            }
            
            // Phase 1: Allocate burst
            for (uint32_t i = 0; i < burst_size; ++i) {
                if (currentTimeMillis() >= end_time) break;
                
                // Mix of allocation types
                if (rng.randBool(0.7)) {
                    // Array allocations (geometric sizes)
                    size_t size = getGeometricSize(rng, config_.scale);
                    char* ptr = new char[size];
                    allocations.push_back(ptr);
                    result.stats.allocations++;
                    result.stats.bytes_allocated += size;
                } else {
                    // Blob objects (non-trivial)
                    size_t size = rng.randSize(64, 4096, config_.scale);
                    blobs.push_back(std::make_unique<Blob>(size));
                    result.stats.allocations++;
                    result.stats.bytes_allocated += size;
                }
            }
            
            // Phase 2: Partial deallocation (non-ordered)
            if (!allocations.empty()) {
                // Deallocate 30-70% of current allocations
                uint32_t dealloc_count = rng.randInt(
                    allocations.size() / 3,
                    (allocations.size() * 7) / 10
                );
                
                for (uint32_t i = 0; i < dealloc_count && !allocations.empty(); ++i) {
                    // Random selection (not LIFO/FIFO)
                    uint32_t index = rng.randInt(0, static_cast<uint32_t>(allocations.size() - 1));
                    delete[] static_cast<char*>(allocations[index]);
                    allocations.erase(allocations.begin() + index);
                    result.stats.deallocations++;
                }
            }
            
            // Phase 3: Clean up some blobs (medium-lived objects)
            if (!blobs.empty() && rng.randBool(0.4)) {
                uint32_t cleanup_count = rng.randInt(1, static_cast<uint32_t>(blobs.size() / 3));
                for (uint32_t i = 0; i < cleanup_count && !blobs.empty(); ++i) {
                    blobs.pop_back();  // unique_ptr will auto-delete
                    result.stats.deallocations++;
                }
            }
            
            // Update peak memory estimation
            size_t current_memory = (allocations.size() * 1024) + (blobs.size() * 2048);
            result.stats.peak_memory = std::max(result.stats.peak_memory, current_memory);
            
            burst_count++;
            
            // Small delay between bursts to create realistic patterns
            if (rng.randBool(0.2)) {
                sleepMillis(rng.randInt(1, 5));
            }
        }
        
        // Clean up remaining allocations
        for (void* ptr : allocations) {
            delete[] static_cast<char*>(ptr);
            result.stats.deallocations++;
        }
        allocations.clear();
        blobs.clear();  // unique_ptr will auto-delete
        
        result.stats.duration_ms = timer.elapsedMillis();
        
        if (!config_.quiet) {
            std::cout << "Thread " << thread_id << " AllocStorm: " 
                      << result.stats.allocations << " allocs, "
                      << result.stats.deallocations << " deallocs, "
                      << formatBytes(result.stats.bytes_allocated) << " total, "
                      << burst_count << " bursts\n";
        }
        
        return result;
    }

private:
    const WorkloadConfig& config_;
    
    /**
     * Generate geometric distribution sizes for realistic allocation patterns
     */
    size_t getGeometricSize(RNG& rng, double /*scale*/) {
        // Geometric distribution favors smaller sizes
        double p = 0.3;  // Success probability
        uint32_t trials = 0;
        
        while (rng.randDouble() > p && trials < 20) {
            trials++;
        }
        
        // Base size grows exponentially with trials
        size_t base_size = 1 << std::min(trials, 12U);  // Cap at 4KB
        return config_.getScaledSize(base_size);
    }
};

/**
 * Factory function to create and execute AllocStorm
 */
ModuleResult runAllocStorm(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms) {
    AllocStorm storm(config);
    return storm.execute(thread_id, duration_ms);
}

} // namespace mp