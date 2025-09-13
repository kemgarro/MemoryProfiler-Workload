#pragma once

#include <cstdint>
#include <string>

namespace mp {

/**
 * Configuration for the memory workload
 * Contains all CLI parameters and runtime settings
 */
struct WorkloadConfig {
    // Threading
    uint32_t threads = 2;
    
    // Timing
    uint32_t seconds = 6;
    
    // Randomization
    uint32_t seed = 12345;
    double scale = 1.0;
    
    // Memory behavior
    double leak_rate = 0.05;  // Fraction of allocations that leak
    uint32_t burst_size = 500;
    bool no_leaks = false;
    
    // Output
    bool quiet = false;
    
    // API integration (only available if MP_USE_API is defined)
#ifdef MP_USE_API
    uint32_t snapshot_every_ms = 1000;
#endif
    
    /**
     * Parse command line arguments and populate config
     * @param argc Argument count
     * @param argv Argument vector
     * @return true if parsing succeeded, false otherwise
     */
    bool parseArgs(int argc, char* argv[]);
    
    /**
     * Print usage information to stdout
     */
    void printUsage(const char* program_name) const;
    
    /**
     * Validate configuration parameters
     * @return true if valid, false otherwise
     */
    bool validate() const;
    
    /**
     * Get scaled value based on scale factor
     * @param base Base value to scale
     * @return Scaled value
     */
    uint32_t getScaled(uint32_t base) const;
    
    /**
     * Get scaled value for size calculations
     * @param base Base size in bytes
     * @return Scaled size respecting MP_MAX_MEM_MB limit
     */
    size_t getScaledSize(size_t base) const;
};

} // namespace mp