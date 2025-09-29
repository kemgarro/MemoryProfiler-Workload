#include "WorkloadConfig.hpp"
#include "Utilities.hpp"
#include <iostream>
#include <sstream>

namespace mp {

bool WorkloadConfig::parseArgs(int argc, char* argv[]) {
    ArgParser parser(argc, argv);
    
    // Parse all options
    threads = static_cast<uint32_t>(parser.getIntOption("--threads", static_cast<int>(threads)));
    seconds = static_cast<uint32_t>(parser.getIntOption("--seconds", static_cast<int>(seconds)));
    seed = static_cast<uint32_t>(parser.getIntOption("--seed", static_cast<int>(seed)));
    scale = parser.getDoubleOption("--scale", scale);
    leak_rate = parser.getDoubleOption("--leak-rate", leak_rate);
    burst_size = static_cast<uint32_t>(parser.getIntOption("--burst-size", static_cast<int>(burst_size)));
    no_leaks = parser.hasFlag("--no-leaks");
    quiet = parser.hasFlag("--quiet");
    
#ifdef MP_USE_API
    snapshot_every_ms = static_cast<uint32_t>(parser.getIntOption("--snapshot-every-ms", static_cast<int>(snapshot_every_ms)));
#endif
    
    // Validate configuration
    if (!validate()) {
        return false;
    }
    
    return true;
}

void WorkloadConfig::printUsage(const char* program_name) const {
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --threads <N>           Number of worker threads (default: " << threads << ")\n";
    std::cout << "  --seconds <S>           Duration in seconds (default: " << seconds << ")\n";
    std::cout << "  --seed <UINT>           Random seed (default: " << seed << ")\n";
    std::cout << "  --scale <K>             Scale factor for sizes/repetitions (default: " << scale << ")\n";
    std::cout << "  --leak-rate <p>         Fraction of allocations that leak (default: " << leak_rate << ")\n";
    std::cout << "  --burst-size <B>        Size of allocation bursts (default: " << burst_size << ")\n";
    std::cout << "  --no-leaks              Disable memory leaks\n";
    std::cout << "  --quiet                 Reduce log output\n";
#ifdef MP_USE_API
    std::cout << "  --snapshot-every-ms <M> Snapshot interval in milliseconds (default: " << snapshot_every_ms << ")\n";
#endif
    std::cout << "  --help                  Show this help message\n";
}

bool WorkloadConfig::validate() const {
    if (threads == 0) {
        std::cerr << "Error: threads must be > 0\n";
        return false;
    }
    
    if (seconds == 0) {
        std::cerr << "Error: seconds must be > 0\n";
        return false;
    }
    
    if (scale <= 0.0) {
        std::cerr << "Error: scale must be > 0.0\n";
        return false;
    }
    
    if (leak_rate < 0.0 || leak_rate > 1.0) {
        std::cerr << "Error: leak-rate must be between 0.0 and 1.0\n";
        return false;
    }
    
    if (burst_size == 0) {
        std::cerr << "Error: burst-size must be > 0\n";
        return false;
    }
    
#ifdef MP_USE_API
    if (snapshot_every_ms == 0) {
        std::cerr << "Error: snapshot-every-ms must be > 0\n";
        return false;
    }
#endif
    
    return true;
}

uint32_t WorkloadConfig::getScaled(uint32_t base) const {
    return static_cast<uint32_t>(base * scale);
}

size_t WorkloadConfig::getScaledSize(size_t base) const {
    size_t scaled = static_cast<size_t>(base * scale);
    size_t max_mem = getMaxMemoryBytes();
    return std::min(scaled, max_mem);
}

} // namespace mp