#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace mp {

/**
 * Blob class for testing non-trivial object allocations
 * Encapsulates a char array with variable size and proper cleanup
 */
class Blob {
public:
    explicit Blob(size_t size);
    ~Blob();
    
    // Non-copyable but movable
    Blob(const Blob&) = delete;
    Blob& operator=(const Blob&) = delete;
    Blob(Blob&& other) noexcept;
    Blob& operator=(Blob&& other) noexcept;
    
    /**
     * Get the data buffer
     */
    char* data() { return data_; }
    const char* data() const { return data_; }
    
    /**
     * Get the size of the blob
     */
    size_t size() const { return size_; }
    
    /**
     * Fill the blob with random data
     */
    void fillRandom(uint32_t seed);

private:
    char* data_;
    size_t size_;
};

/**
 * Binary tree node for tree construction patterns
 */
struct Node {
    int payload;
    Node* left;
    Node* right;
    
    explicit Node(int value) : payload(value), left(nullptr), right(nullptr) {}
    
    /**
     * Recursively delete the tree
     */
    static void deleteTree(Node* root);
    
    /**
     * Get the depth of the tree
     */
    static int getDepth(const Node* root);
    
    /**
     * Count total nodes in the tree
     */
    static int countNodes(const Node* root);
};

/**
 * Global leak repository for controlled memory leaks
 * Thread-safe container for storing leaked pointers
 */
class LeakRepository {
public:
    static LeakRepository& instance();
    
    /**
     * Add a pointer to the leak repository
     * @param ptr Pointer to leak
     * @param size Size of the allocation
     * @param is_array Whether this is an array allocation
     */
    void addLeak(void* ptr, size_t size, bool is_array = false);
    
    /**
     * Get statistics about current leaks
     */
    struct Stats {
        size_t count = 0;
        size_t total_bytes = 0;
        size_t array_count = 0;
        size_t object_count = 0;
    };
    
    Stats getStats() const;
    
    /**
     * Clear all leaks (for cleanup if needed)
     */
    void clear();

private:
    LeakRepository() = default;
    
    struct LeakInfo {
        void* ptr;
        size_t size;
        bool is_array;
    };
    
    mutable std::mutex mutex_;
    std::vector<LeakInfo> leaks_;
};

/**
 * Workload statistics for tracking performance
 */
struct WorkloadStats {
    uint64_t allocations = 0;
    uint64_t deallocations = 0;
    uint64_t bytes_allocated = 0;
    uint64_t bytes_deallocated = 0;
    uint64_t peak_memory = 0;
    uint64_t duration_ms = 0;
    
    /**
     * Merge stats from another instance
     */
    void merge(const WorkloadStats& other);
    
    /**
     * Reset all counters
     */
    void reset();
};

/**
 * Module execution result
 */
struct ModuleResult {
    std::string module_name;
    WorkloadStats stats;
    bool success = true;
    std::string error_message;
    
    ModuleResult(const std::string& name) : module_name(name) {}
};

} // namespace mp