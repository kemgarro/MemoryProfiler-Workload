#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace mp {

/**
 * Deterministic random number generator for reproducible workloads
 */
class RNG {
public:
    explicit RNG(uint32_t seed);
    
    /**
     * Generate random integer in range [min, max]
     */
    uint32_t randInt(uint32_t min, uint32_t max);
    
    /**
     * Generate random size respecting memory limits
     * @param min Minimum size in bytes
     * @param max Maximum size in bytes
     * @param scale Scale factor
     * @return Random size in bytes
     */
    size_t randSize(size_t min, size_t max, double scale = 1.0);
    
    /**
     * Generate random double in range [0.0, 1.0)
     */
    double randDouble();
    
    /**
     * Generate random boolean with given probability
     * @param probability Probability of returning true (0.0 to 1.0)
     */
    bool randBool(double probability = 0.5);

private:
    std::mt19937 gen_;
    std::uniform_int_distribution<uint32_t> int_dist_;
    std::uniform_real_distribution<double> real_dist_;
};

/**
 * Simple timing utilities
 */
class Timer {
public:
    Timer();
    
    /**
     * Get elapsed time in milliseconds
     */
    uint64_t elapsedMillis() const;
    
    /**
     * Reset timer to current time
     */
    void reset();

private:
    uint64_t start_time_;
};

/**
 * Parse command line arguments into key-value pairs
 */
class ArgParser {
public:
    explicit ArgParser(int argc, char* argv[]);
    
    /**
     * Check if flag exists
     */
    bool hasFlag(const std::string& flag) const;
    
    /**
     * Get value for option, with default
     */
    std::string getOption(const std::string& option, const std::string& default_value = "") const;
    
    /**
     * Get integer value for option, with default
     */
    int getIntOption(const std::string& option, int default_value = 0) const;
    
    /**
     * Get double value for option, with default
     */
    double getDoubleOption(const std::string& option, double default_value = 0.0) const;

private:
    std::vector<std::string> args_;
};

/**
 * Get current time in milliseconds since epoch
 */
uint64_t currentTimeMillis();

/**
 * Sleep for specified milliseconds
 */
void sleepMillis(uint32_t ms);

/**
 * Format bytes into human readable string
 */
std::string formatBytes(size_t bytes);

/**
 * Get maximum allowed memory size in bytes based on MP_MAX_MEM_MB
 */
size_t getMaxMemoryBytes();

} // namespace mp