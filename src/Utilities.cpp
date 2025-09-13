#include "Utilities.hpp"
#include "WorkloadConfig.hpp"
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace mp {

// RNG Implementation
RNG::RNG(uint32_t seed) : gen_(seed), int_dist_(0, UINT32_MAX), real_dist_(0.0, 1.0) {
}

uint32_t RNG::randInt(uint32_t min, uint32_t max) {
    if (min >= max) return min;
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(gen_);
}

size_t RNG::randSize(size_t min, size_t max, double scale) {
    if (min >= max) return min;
    
    // Apply scale factor
    min = static_cast<size_t>(min * scale);
    max = static_cast<size_t>(max * scale);
    
    // Respect memory limits
    size_t max_mem = getMaxMemoryBytes();
    max = std::min(max, max_mem);
    min = std::min(min, max);
    
    if (min >= max) return min;
    
    std::uniform_int_distribution<size_t> dist(min, max);
    return dist(gen_);
}

double RNG::randDouble() {
    return real_dist_(gen_);
}

bool RNG::randBool(double probability) {
    return real_dist_(gen_) < probability;
}

// Timer Implementation
Timer::Timer() : start_time_(currentTimeMillis()) {
}

uint64_t Timer::elapsedMillis() const {
    return currentTimeMillis() - start_time_;
}

void Timer::reset() {
    start_time_ = currentTimeMillis();
}

// ArgParser Implementation
ArgParser::ArgParser(int argc, char* argv[]) {
    for (int i = 0; i < argc; ++i) {
        args_.emplace_back(argv[i]);
    }
}

bool ArgParser::hasFlag(const std::string& flag) const {
    return std::find(args_.begin(), args_.end(), flag) != args_.end();
}

std::string ArgParser::getOption(const std::string& option, const std::string& default_value) const {
    auto it = std::find(args_.begin(), args_.end(), option);
    if (it != args_.end() && std::next(it) != args_.end()) {
        return *std::next(it);
    }
    return default_value;
}

int ArgParser::getIntOption(const std::string& option, int default_value) const {
    std::string value = getOption(option);
    if (value.empty()) return default_value;
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

double ArgParser::getDoubleOption(const std::string& option, double default_value) const {
    std::string value = getOption(option);
    if (value.empty()) return default_value;
    
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

// Utility functions
uint64_t currentTimeMillis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void sleepMillis(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

size_t getMaxMemoryBytes() {
    return static_cast<size_t>(MP_MAX_MEM_MB) * 1024 * 1024;
}

} // namespace mp