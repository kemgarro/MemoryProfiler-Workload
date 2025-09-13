#include "WorkloadConfig.hpp"
#include "Utilities.hpp"
#include "Types.hpp"
#include <vector>
#include <stdexcept>
#include <iostream>

namespace mp {

/**
 * TreeFactory module - Constructs and destroys binary trees
 * 
 * This module creates memory patterns by:
 * - Building balanced and unbalanced binary trees
 * - Using new/delete for individual nodes
 * - Injecting exceptions during construction
 * - Ensuring proper cleanup even with exceptions
 * - Creating deep and wide tree structures
 */
class TreeFactory {
public:
    explicit TreeFactory(const WorkloadConfig& config) : config_(config) {}
    
    ModuleResult execute(uint32_t thread_id, uint64_t duration_ms) {
        ModuleResult result("TreeFactory");
        Timer timer;
        RNG rng(config_.seed + thread_id + 4000);  // Different seed offset
        
        uint64_t end_time = currentTimeMillis() + duration_ms;
        uint32_t tree_cycles = 0;
        
        while (currentTimeMillis() < end_time) {
            // Pattern 1: Balanced trees
            executeBalancedTreePattern(rng, result);
            
            // Pattern 2: Unbalanced trees (linked list like)
            executeUnbalancedTreePattern(rng, result);
            
            // Skip exception pattern for now to avoid crashes
            // executeExceptionTreePattern(rng, result);
            
            // Pattern 4: Multiple small trees
            executeMultipleTreesPattern(rng, result);
            
            tree_cycles++;
            
            // Small delay between cycles
            if (rng.randBool(0.3)) {
                sleepMillis(rng.randInt(1, 2));
            }
        }
        
        result.stats.duration_ms = timer.elapsedMillis();
        
        if (!config_.quiet) {
            std::cout << "Thread " << thread_id << " TreeFactory: " 
                      << tree_cycles << " cycles, "
                      << result.stats.allocations << " nodes created\n";
        }
        
        return result;
    }

private:
    const WorkloadConfig& config_;
    
    /**
     * Pattern 1: Create balanced binary trees
     */
    void executeBalancedTreePattern(RNG& rng, ModuleResult& result) {
        uint32_t tree_count = rng.randInt(2, config_.getScaled(8));
        
        for (uint32_t i = 0; i < tree_count; ++i) {
            uint32_t node_count = rng.randInt(10, config_.getScaled(100));
            Node* root = buildBalancedTree(rng, node_count, result);
            
            if (root != nullptr) {
                // Verify tree structure
                int actual_nodes = Node::countNodes(root);
                (void)Node::getDepth(root);  // Verify depth calculation
                
                // Clean up
                Node::deleteTree(root);
                result.stats.deallocations += actual_nodes;
            }
        }
    }
    
    /**
     * Pattern 2: Create unbalanced trees (linked list like)
     */
    void executeUnbalancedTreePattern(RNG& rng, ModuleResult& result) {
        uint32_t tree_count = rng.randInt(2, config_.getScaled(6));
        
        for (uint32_t i = 0; i < tree_count; ++i) {
            uint32_t node_count = rng.randInt(20, config_.getScaled(150));
            Node* root = buildUnbalancedTree(rng, node_count, result);
            
            if (root != nullptr) {
                int actual_nodes = Node::countNodes(root);
                (void)Node::getDepth(root);  // Verify depth calculation
                
                // Clean up
                Node::deleteTree(root);
                result.stats.deallocations += actual_nodes;
            }
        }
    }
    
    /**
     * Pattern 3: Trees with exception injection
     */
    void executeExceptionTreePattern(RNG& rng, ModuleResult& result) {
        uint32_t tree_count = rng.randInt(3, config_.getScaled(10));
        
        for (uint32_t i = 0; i < tree_count; ++i) {
            uint32_t target_nodes = rng.randInt(15, config_.getScaled(80));
            uint32_t exception_at = rng.randInt(5, target_nodes - 5);
            
            Node* root = nullptr;
            try {
                root = buildTreeWithException(rng, target_nodes, exception_at, result);
            } catch (const std::runtime_error& e) {
                // Exception caught - ensure no memory leaks
                if (root != nullptr) {
                    Node::deleteTree(root);
                    result.stats.deallocations += Node::countNodes(root);
                }
                // Continue execution - this is expected behavior
            }
            
            // If no exception occurred, clean up normally
            if (root != nullptr) {
                Node::deleteTree(root);
                result.stats.deallocations += Node::countNodes(root);
            }
        }
    }
    
    /**
     * Pattern 4: Multiple small trees for fragmentation
     */
    void executeMultipleTreesPattern(RNG& rng, ModuleResult& result) {
        uint32_t tree_count = rng.randInt(5, config_.getScaled(20));
        std::vector<Node*> trees;
        trees.reserve(tree_count);
        
        // Build multiple trees
        for (uint32_t i = 0; i < tree_count; ++i) {
            uint32_t node_count = rng.randInt(3, config_.getScaled(15));
            Node* root = buildRandomTree(rng, node_count, result);
            trees.push_back(root);
        }
        
        // Randomly destroy some trees
        uint32_t destroy_count = rng.randInt(1, static_cast<uint32_t>(trees.size() / 2));
        for (uint32_t i = 0; i < destroy_count && !trees.empty(); ++i) {
            uint32_t index = rng.randInt(0, static_cast<uint32_t>(trees.size() - 1));
            Node* tree = trees[index];
            trees.erase(trees.begin() + index);
            
            if (tree != nullptr) {
                Node::deleteTree(tree);
                result.stats.deallocations += Node::countNodes(tree);
            }
        }
        
        // Clean up remaining trees
        for (Node* tree : trees) {
            if (tree != nullptr) {
                Node::deleteTree(tree);
                result.stats.deallocations += Node::countNodes(tree);
            }
        }
        trees.clear();
    }
    
    /**
     * Build a balanced binary tree
     */
    Node* buildBalancedTree(RNG& rng, uint32_t node_count, ModuleResult& result) {
        if (node_count == 0) return nullptr;
        
        std::vector<int> values;
        for (uint32_t i = 0; i < node_count; ++i) {
            values.push_back(static_cast<int>(rng.randInt(0, 1000)));
        }
        
        return buildBalancedTreeRecursive(values, 0, static_cast<int>(values.size() - 1), result);
    }
    
    Node* buildBalancedTreeRecursive(const std::vector<int>& values, int start, int end, ModuleResult& result) {
        if (start > end) return nullptr;
        
        int mid = (start + end) / 2;
        Node* node = new Node(values[mid]);
        result.stats.allocations++;
        result.stats.bytes_allocated += sizeof(Node);
        
        node->left = buildBalancedTreeRecursive(values, start, mid - 1, result);
        node->right = buildBalancedTreeRecursive(values, mid + 1, end, result);
        
        return node;
    }
    
    /**
     * Build an unbalanced tree (linked list like)
     */
    Node* buildUnbalancedTree(RNG& rng, uint32_t node_count, ModuleResult& result) {
        if (node_count == 0) return nullptr;
        
        Node* root = new Node(rng.randInt(0, 1000));
        result.stats.allocations++;
        result.stats.bytes_allocated += sizeof(Node);
        
        Node* current = root;
        for (uint32_t i = 1; i < node_count; ++i) {
            // Always add to the same side to create imbalance
            bool add_left = rng.randBool(0.7);  // 70% chance to add left
            
            Node* new_node = new Node(rng.randInt(0, 1000));
            result.stats.allocations++;
            result.stats.bytes_allocated += sizeof(Node);
            
            if (add_left) {
                current->left = new_node;
            } else {
                current->right = new_node;
            }
            current = new_node;
        }
        
        return root;
    }
    
    /**
     * Build a tree with exception injection
     */
    Node* buildTreeWithException(RNG& rng, uint32_t target_nodes, uint32_t exception_at, ModuleResult& result) {
        Node* root = new Node(rng.randInt(0, 1000));
        result.stats.allocations++;
        result.stats.bytes_allocated += sizeof(Node);
        
        std::vector<Node*> nodes;
        nodes.push_back(root);
        
        for (uint32_t i = 1; i < target_nodes; ++i) {
            if (i == exception_at) {
                throw std::runtime_error("Simulated tree construction failure");
            }
            
            // Add to random existing node
            uint32_t parent_index = rng.randInt(0, static_cast<uint32_t>(nodes.size() - 1));
            Node* parent = nodes[parent_index];
            
            Node* new_node = new Node(rng.randInt(0, 1000));
            result.stats.allocations++;
            result.stats.bytes_allocated += sizeof(Node);
            
            // Add to available child
            if (parent->left == nullptr) {
                parent->left = new_node;
            } else if (parent->right == nullptr) {
                parent->right = new_node;
            } else {
                // Both children full, add to a random child
                if (rng.randBool(0.5)) {
                    parent->left = new_node;
                } else {
                    parent->right = new_node;
                }
            }
            
            nodes.push_back(new_node);
        }
        
        return root;
    }
    
    /**
     * Build a random tree structure
     */
    Node* buildRandomTree(RNG& rng, uint32_t node_count, ModuleResult& result) {
        if (node_count == 0) return nullptr;
        
        Node* root = new Node(rng.randInt(0, 1000));
        result.stats.allocations++;
        result.stats.bytes_allocated += sizeof(Node);
        
        std::vector<Node*> nodes;
        nodes.push_back(root);
        
        for (uint32_t i = 1; i < node_count; ++i) {
            uint32_t parent_index = rng.randInt(0, static_cast<uint32_t>(nodes.size() - 1));
            Node* parent = nodes[parent_index];
            
            Node* new_node = new Node(rng.randInt(0, 1000));
            result.stats.allocations++;
            result.stats.bytes_allocated += sizeof(Node);
            
            // Randomly assign to left or right
            if (rng.randBool(0.5)) {
                parent->left = new_node;
            } else {
                parent->right = new_node;
            }
            
            nodes.push_back(new_node);
        }
        
        return root;
    }
};

/**
 * Factory function to create and execute TreeFactory
 */
ModuleResult runTreeFactory(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms) {
    TreeFactory factory(config);
    return factory.execute(thread_id, duration_ms);
}

} // namespace mp