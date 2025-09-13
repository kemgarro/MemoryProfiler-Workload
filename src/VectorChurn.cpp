#include "WorkloadConfig.hpp"
#include "Utilities.hpp"
#include "Types.hpp"
#include <vector>
#include <string>
#include <memory>
#include <iostream>

namespace mp {

/**
 * VectorChurn module - Stresses std::vector with various patterns
 * 
 * This module creates memory churn by:
 * - Creating and destroying std::vector<std::string>
 * - Using std::vector<std::unique_ptr<T>>
 * - reserve() calls that may cause reallocations
 * - Insertions/deletions that trigger multiple internal reallocations
 * - Patterns that stress the vector's growth strategy
 */
class VectorChurn {
public:
    explicit VectorChurn(const WorkloadConfig& config) : config_(config) {}
    
    ModuleResult execute(uint32_t thread_id, uint64_t duration_ms) {
        ModuleResult result("VectorChurn");
        Timer timer;
        RNG rng(config_.seed + thread_id + 3000);  // Different seed offset
        
        uint64_t end_time = currentTimeMillis() + duration_ms;
        uint32_t vector_cycles = 0;
        
        while (currentTimeMillis() < end_time) {
            // Pattern 1: String vector with reserve and growth
            executeStringVectorPattern(rng, result);
            
            // Pattern 2: Unique_ptr vector with mixed operations
            executeUniquePtrVectorPattern(rng, result);
            
            // Pattern 3: Nested vector stress
            executeNestedVectorPattern(rng, result);
            
            vector_cycles++;
            
            // Small delay between cycles
            if (rng.randBool(0.2)) {
                sleepMillis(rng.randInt(1, 3));
            }
        }
        
        result.stats.duration_ms = timer.elapsedMillis();
        
        if (!config_.quiet) {
            std::cout << "Thread " << thread_id << " VectorChurn: " 
                      << vector_cycles << " cycles, "
                      << formatBytes(result.stats.bytes_allocated) << " total\n";
        }
        
        return result;
    }

private:
    const WorkloadConfig& config_;
    
    /**
     * Pattern 1: String vectors with reserve and growth stress
     */
    void executeStringVectorPattern(RNG& rng, ModuleResult& result) {
        uint32_t vector_count = rng.randInt(5, config_.getScaled(20));
        
        for (uint32_t i = 0; i < vector_count; ++i) {
            std::vector<std::string> strings;
            
            // Reserve space (may cause allocation)
            uint32_t reserve_size = rng.randInt(100, config_.getScaled(1000));
            strings.reserve(reserve_size);
            result.stats.allocations++;  // Reserve may allocate
            
            // Fill with strings of varying sizes
            uint32_t fill_count = rng.randInt(reserve_size / 2, reserve_size * 2);
            for (uint32_t j = 0; j < fill_count; ++j) {
                std::string str = generateRandomString(rng, 10, 200);
                strings.push_back(std::move(str));
                result.stats.allocations++;  // String allocation
                result.stats.bytes_allocated += str.capacity();
            }
            
            // Random insertions and deletions
            uint32_t operations = rng.randInt(10, 50);
            for (uint32_t k = 0; k < operations; ++k) {
                if (rng.randBool(0.6) && !strings.empty()) {
                    // Random deletion
                    uint32_t index = rng.randInt(0, static_cast<uint32_t>(strings.size() - 1));
                    strings.erase(strings.begin() + index);
                } else {
                    // Random insertion
                    std::string str = generateRandomString(rng, 5, 100);
                    uint32_t index = strings.empty() ? 0 : rng.randInt(0, static_cast<uint32_t>(strings.size()));
                    strings.insert(strings.begin() + index, std::move(str));
                    result.stats.allocations++;
                }
            }
            
            // Vector destructor will clean up all strings
            result.stats.deallocations += strings.size();
        }
    }
    
    /**
     * Pattern 2: Unique_ptr vectors with mixed operations
     */
    void executeUniquePtrVectorPattern(RNG& rng, ModuleResult& result) {
        uint32_t vector_count = rng.randInt(3, config_.getScaled(15));
        
        for (uint32_t i = 0; i < vector_count; ++i) {
            std::vector<std::unique_ptr<Blob>> blobs;
            
            // Initial fill
            uint32_t initial_size = rng.randInt(50, config_.getScaled(300));
            for (uint32_t j = 0; j < initial_size; ++j) {
                size_t blob_size = rng.randSize(64, 1024, config_.scale);
                blobs.push_back(std::make_unique<Blob>(blob_size));
                result.stats.allocations += 2;  // Blob + unique_ptr
                result.stats.bytes_allocated += blob_size;
            }
            
            // Stress operations
            uint32_t stress_ops = rng.randInt(20, 100);
            for (uint32_t k = 0; k < stress_ops; ++k) {
                if (rng.randBool(0.4) && !blobs.empty()) {
                    // Random removal
                    uint32_t index = rng.randInt(0, static_cast<uint32_t>(blobs.size() - 1));
                    blobs.erase(blobs.begin() + index);
                    result.stats.deallocations++;
                } else {
                    // Random insertion
                    size_t blob_size = rng.randSize(32, 512, config_.scale);
                    uint32_t index = blobs.empty() ? 0 : rng.randInt(0, static_cast<uint32_t>(blobs.size()));
                    blobs.insert(blobs.begin() + index, std::make_unique<Blob>(blob_size));
                    result.stats.allocations += 2;
                    result.stats.bytes_allocated += blob_size;
                }
            }
            
            // Clear and refill to trigger reallocations
            blobs.clear();
            result.stats.deallocations += blobs.size();
            
            uint32_t refill_size = rng.randInt(100, config_.getScaled(500));
            blobs.reserve(refill_size);
            for (uint32_t j = 0; j < refill_size; ++j) {
                size_t blob_size = rng.randSize(128, 2048, config_.scale);
                blobs.push_back(std::make_unique<Blob>(blob_size));
                result.stats.allocations += 2;
                result.stats.bytes_allocated += blob_size;
            }
            
            // Destructor will clean up remaining blobs
            result.stats.deallocations += blobs.size();
        }
    }
    
    /**
     * Pattern 3: Nested vectors for complex allocation patterns
     */
    void executeNestedVectorPattern(RNG& rng, ModuleResult& result) {
        uint32_t outer_count = rng.randInt(2, config_.getScaled(10));
        
        for (uint32_t i = 0; i < outer_count; ++i) {
            std::vector<std::vector<int>> nested_vectors;
            
            // Create nested structure
            uint32_t inner_count = rng.randInt(5, config_.getScaled(20));
            for (uint32_t j = 0; j < inner_count; ++j) {
                std::vector<int> inner_vec;
                
                // Reserve and fill
                uint32_t reserve_size = rng.randInt(50, config_.getScaled(200));
                inner_vec.reserve(reserve_size);
                result.stats.allocations++;
                
                uint32_t fill_size = rng.randInt(reserve_size, reserve_size * 2);
                for (uint32_t k = 0; k < fill_size; ++k) {
                    inner_vec.push_back(rng.randInt(0, 1000));
                }
                
                nested_vectors.push_back(std::move(inner_vec));
                result.stats.allocations++;
            }
            
            // Random operations on nested structure
            uint32_t ops = rng.randInt(5, 20);
            for (uint32_t op = 0; op < ops; ++op) {
                if (rng.randBool(0.3) && !nested_vectors.empty()) {
                    // Remove random inner vector
                    uint32_t index = rng.randInt(0, static_cast<uint32_t>(nested_vectors.size() - 1));
                    nested_vectors.erase(nested_vectors.begin() + index);
                    result.stats.deallocations++;
                } else {
                    // Add new inner vector
                    std::vector<int> new_vec;
                    uint32_t size = rng.randInt(10, config_.getScaled(100));
                    new_vec.reserve(size);
                    for (uint32_t k = 0; k < size; ++k) {
                        new_vec.push_back(rng.randInt(0, 500));
                    }
                    nested_vectors.push_back(std::move(new_vec));
                    result.stats.allocations++;
                }
            }
            
            // Destructor will clean up all nested vectors
            result.stats.deallocations += nested_vectors.size();
        }
    }
    
    /**
     * Generate random string of specified length range
     */
    std::string generateRandomString(RNG& rng, uint32_t min_len, uint32_t max_len) {
        uint32_t len = rng.randInt(min_len, max_len);
        std::string str;
        str.reserve(len);
        
        for (uint32_t i = 0; i < len; ++i) {
            str += static_cast<char>('a' + rng.randInt(0, 25));
        }
        
        return str;
    }
};

/**
 * Factory function to create and execute VectorChurn
 */
ModuleResult runVectorChurn(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms) {
    VectorChurn churn(config);
    return churn.execute(thread_id, duration_ms);
}

} // namespace mp