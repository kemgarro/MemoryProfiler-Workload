#include "WorkloadConfig.hpp"
#include "Utilities.hpp"
#include "Types.hpp"
#include <vector>
#include <memory>
#include <iostream>

namespace mp {

/**
 * LeakFactory module - Generates controlled memory leaks
 * 
 * This module creates intentional memory leaks according to the leak_rate parameter:
 * - Stores leaked pointers in global LeakRepository
 * - Mixes different allocation types (objects, arrays, blobs)
 * - Varies sizes to create realistic leak patterns
 * - Avoids double-free and use-after-free issues
 */
class LeakFactory {
public:
    explicit LeakFactory(const WorkloadConfig& config) : config_(config) {}
    
    ModuleResult execute(uint32_t thread_id, uint64_t duration_ms) {
        ModuleResult result("LeakFactory");
        Timer timer;
        RNG rng(config_.seed + thread_id + 1000);  // Different seed offset
        
        std::vector<void*> temp_allocations;  // For non-leaked allocations
        std::vector<std::unique_ptr<Blob>> temp_blobs;
        
        uint64_t end_time = currentTimeMillis() + duration_ms;
        uint32_t leak_count = 0;
        uint32_t normal_count = 0;
        
        while (currentTimeMillis() < end_time) {
            // Determine batch size for this iteration
            uint32_t batch_size = rng.randInt(10, config_.getScaled(100));
            
            for (uint32_t i = 0; i < batch_size; ++i) {
                if (currentTimeMillis() >= end_time) break;
                
                // Decide allocation type
                AllocationType type = static_cast<AllocationType>(rng.randInt(0, 2));
                size_t size = getLeakSize(rng);
                
                // Decide if this allocation will leak
                bool will_leak = !config_.no_leaks && rng.randBool(config_.leak_rate);
                
                switch (type) {
                    case AllocationType::SIMPLE_OBJECT: {
                        char* ptr = new char[size];
                        if (will_leak) {
                            // Add to leak repository - this will NOT be freed
                            LeakRepository::instance().addLeak(ptr, size, false);
                            leak_count++;
                        } else {
                            // Add to temp list for later cleanup
                            temp_allocations.push_back(ptr);
                            normal_count++;
                        }
                        result.stats.allocations++;
                        result.stats.bytes_allocated += size;
                        break;
                    }
                    
                    case AllocationType::ARRAY: {
                        char* ptr = new char[size];
                        if (will_leak) {
                            // Add to leak repository - this will NOT be freed
                            LeakRepository::instance().addLeak(ptr, size, true);
                            leak_count++;
                        } else {
                            // Add to temp list for later cleanup
                            temp_allocations.push_back(ptr);
                            normal_count++;
                        }
                        result.stats.allocations++;
                        result.stats.bytes_allocated += size;
                        break;
                    }
                    
                    case AllocationType::BLOB: {
                        auto blob = std::make_unique<Blob>(size);
                        if (will_leak) {
                            // Transfer ownership to leak repository
                            LeakRepository::instance().addLeak(blob.release(), size, false);
                            leak_count++;
                        } else {
                            // Add to temp list for later cleanup
                            temp_blobs.push_back(std::move(blob));
                            normal_count++;
                        }
                        result.stats.allocations++;
                        result.stats.bytes_allocated += size;
                        break;
                    }
                }
                
                // Occasionally clean up some non-leaked allocations
                if (!temp_allocations.empty() && rng.randBool(0.3)) {
                    uint32_t cleanup_count = rng.randInt(1, static_cast<uint32_t>(temp_allocations.size() / 2));
                    for (uint32_t j = 0; j < cleanup_count && !temp_allocations.empty(); ++j) {
                        uint32_t index = rng.randInt(0, static_cast<uint32_t>(temp_allocations.size() - 1));
                        delete[] static_cast<char*>(temp_allocations[index]);
                        temp_allocations.erase(temp_allocations.begin() + index);
                        result.stats.deallocations++;
                    }
                }
                
                if (!temp_blobs.empty() && rng.randBool(0.2)) {
                    uint32_t cleanup_count = rng.randInt(1, static_cast<uint32_t>(temp_blobs.size() / 3));
                    for (uint32_t j = 0; j < cleanup_count && !temp_blobs.empty(); ++j) {
                        temp_blobs.pop_back();  // unique_ptr auto-deletes
                        result.stats.deallocations++;
                    }
                }
            }
            
            // Small delay between batches
            if (rng.randBool(0.4)) {
                sleepMillis(rng.randInt(1, 3));
            }
        }
        
        // Clean up all remaining non-leaked allocations
        for (void* ptr : temp_allocations) {
            delete[] static_cast<char*>(ptr);
            result.stats.deallocations++;
        }
        temp_allocations.clear();
        temp_blobs.clear();  // unique_ptr auto-deletes
        
        result.stats.duration_ms = timer.elapsedMillis();
        
        if (!config_.quiet) {
            std::cout << "Thread " << thread_id << " LeakFactory: " 
                      << leak_count << " leaks, " << normal_count << " normal, "
                      << formatBytes(result.stats.bytes_allocated) << " total\n";
        }
        
        return result;
    }

private:
    const WorkloadConfig& config_;
    
    enum class AllocationType {
        SIMPLE_OBJECT = 0,
        ARRAY = 1,
        BLOB = 2
    };
    
    /**
     * Generate realistic leak sizes with different distributions
     */
    size_t getLeakSize(RNG& rng) {
        // 60% small leaks, 30% medium, 10% large
        if (rng.randBool(0.6)) {
            // Small leaks: 16-256 bytes
            return rng.randSize(16, 256, config_.scale);
        } else if (rng.randBool(0.75)) {  // 75% of remaining 40%
            // Medium leaks: 256-4096 bytes
            return rng.randSize(256, 4096, config_.scale);
        } else {
            // Large leaks: 4KB-64KB
            return rng.randSize(4096, 65536, config_.scale);
        }
    }
};

/**
 * Factory function to create and execute LeakFactory
 */
ModuleResult runLeakFactory(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms) {
    LeakFactory factory(config);
    return factory.execute(thread_id, duration_ms);
}

} // namespace mp