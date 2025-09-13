#include "Types.hpp"
#include "Utilities.hpp"
#include <cstring>
#include <algorithm>

namespace mp {

// Blob Implementation
Blob::Blob(size_t size) : size_(size) {
    data_ = new char[size];
    // Initialize with some pattern to avoid zero pages
    std::fill(data_, data_ + size, static_cast<char>(0xAA));
}

Blob::~Blob() {
    delete[] data_;
}

Blob::Blob(Blob&& other) noexcept : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

Blob& Blob::operator=(Blob&& other) noexcept {
    if (this != &other) {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

void Blob::fillRandom(uint32_t seed) {
    RNG rng(seed);
    for (size_t i = 0; i < size_; ++i) {
        data_[i] = static_cast<char>(rng.randInt(0, 255));
    }
}

// Node Implementation
void Node::deleteTree(Node* root) {
    if (root == nullptr) return;
    
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

int Node::getDepth(const Node* root) {
    if (root == nullptr) return 0;
    
    int left_depth = getDepth(root->left);
    int right_depth = getDepth(root->right);
    return 1 + std::max(left_depth, right_depth);
}

int Node::countNodes(const Node* root) {
    if (root == nullptr) return 0;
    
    return 1 + countNodes(root->left) + countNodes(root->right);
}

// LeakRepository Implementation
LeakRepository& LeakRepository::instance() {
    static LeakRepository instance;
    return instance;
}

void LeakRepository::addLeak(void* ptr, size_t size, bool is_array) {
    std::lock_guard<std::mutex> lock(mutex_);
    leaks_.push_back({ptr, size, is_array});
}

LeakRepository::Stats LeakRepository::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.count = leaks_.size();
    
    for (const auto& leak : leaks_) {
        stats.total_bytes += leak.size;
        if (leak.is_array) {
            stats.array_count++;
        } else {
            stats.object_count++;
        }
    }
    
    return stats;
}

void LeakRepository::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    leaks_.clear();
}

// WorkloadStats Implementation
void WorkloadStats::merge(const WorkloadStats& other) {
    allocations += other.allocations;
    deallocations += other.deallocations;
    bytes_allocated += other.bytes_allocated;
    bytes_deallocated += other.bytes_deallocated;
    peak_memory = std::max(peak_memory, other.peak_memory);
    duration_ms = std::max(duration_ms, other.duration_ms);
}

void WorkloadStats::reset() {
    allocations = 0;
    deallocations = 0;
    bytes_allocated = 0;
    bytes_deallocated = 0;
    peak_memory = 0;
    duration_ms = 0;
}

} // namespace mp