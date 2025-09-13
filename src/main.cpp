#include "WorkloadConfig.hpp"
#include "Utilities.hpp"
#include "Types.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <map>

// Conditional profiler API inclusion
#ifdef MP_USE_API
#if __has_include("ProfilerAPI.hpp")
#include "ProfilerAPI.hpp"
#define MP_HAVE_API 1
#else
#define MP_HAVE_API 0
#endif
#else
#define MP_HAVE_API 0
#endif

// Forward declarations for module functions
namespace mp {
    ModuleResult runAllocStorm(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms);
    ModuleResult runLeakFactory(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms);
    ModuleResult runFragmenter(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms);
    ModuleResult runVectorChurn(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms);
    ModuleResult runTreeFactory(const WorkloadConfig& config, uint32_t thread_id, uint64_t duration_ms);
}

namespace mp {

/**
 * Worker thread function that executes memory workload modules
 */
void workerThread(const WorkloadConfig& config, uint32_t thread_id, 
                  std::atomic<bool>& should_stop, std::vector<ModuleResult>& results, std::mutex& results_mutex) {
    Timer thread_timer;
    RNG rng(config.seed + thread_id);
    
    // Module execution sequence
    std::vector<std::function<ModuleResult()>> modules = {
        [&]() { return runAllocStorm(config, thread_id, 1000); },
        [&]() { return runVectorChurn(config, thread_id, 1000); },
        [&]() { return runFragmenter(config, thread_id, 1000); },
        [&]() { return runTreeFactory(config, thread_id, 1000); },
        [&]() { return runLeakFactory(config, thread_id, 1000); }
    };
    
    uint32_t cycle_count = 0;
    
    while (!should_stop.load()) {
        // Execute each module in sequence
        for (auto& module_func : modules) {
            if (should_stop.load()) break;
            
            try {
                ModuleResult result = module_func();
                {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results.push_back(result);
                }
            } catch (const std::exception& e) {
                if (!config.quiet) {
                    std::cerr << "Thread " << thread_id << " module error: " << e.what() << std::endl;
                }
            }
        }
        
        cycle_count++;
        
        // Small delay between cycles
        sleepMillis(rng.randInt(10, 50));
    }
    
    if (!config.quiet) {
        std::cout << "Thread " << thread_id << " completed " << cycle_count << " cycles in " 
                  << thread_timer.elapsedMillis() << "ms\n";
    }
}

/**
 * Snapshot thread for periodic profiler API calls
 */
void snapshotThread(const WorkloadConfig& /*config*/, std::atomic<bool>& /*should_stop*/) {
#if MP_HAVE_API
    while (!should_stop.load()) {
        try {
            std::string snapshot = mp::api::getSnapshotJson();
            std::cout << "SNAPSHOT: " << snapshot << std::endl;
        } catch (const std::exception& e) {
            // Silently continue if snapshot fails
        }
        
        // Sleep for the specified interval
        uint64_t sleep_start = currentTimeMillis();
        while (!should_stop.load() && (currentTimeMillis() - sleep_start) < config.snapshot_every_ms) {
            sleepMillis(10);
        }
    }
#endif
}

/**
 * Print final summary statistics
 */
void printSummary(const WorkloadConfig& config, const std::vector<std::vector<ModuleResult>>& all_results) {
    WorkloadStats total_stats;
    LeakRepository::Stats leak_stats = LeakRepository::instance().getStats();
    
    // Aggregate statistics from all threads
    for (const auto& thread_results : all_results) {
        for (const auto& result : thread_results) {
            total_stats.merge(result.stats);
        }
    }
    
    std::cout << "\n=== WORKLOAD SUMMARY ===\n";
    std::cout << "Configuration:\n";
    std::cout << "  Threads: " << config.threads << "\n";
    std::cout << "  Duration: " << config.seconds << "s\n";
    std::cout << "  Scale: " << config.scale << "\n";
    std::cout << "  Leak rate: " << (config.no_leaks ? 0.0 : config.leak_rate) << "\n";
    std::cout << "  Burst size: " << config.burst_size << "\n";
    
    std::cout << "\nMemory Statistics:\n";
    std::cout << "  Total allocations: " << total_stats.allocations << "\n";
    std::cout << "  Total deallocations: " << total_stats.deallocations << "\n";
    std::cout << "  Bytes allocated: " << formatBytes(total_stats.bytes_allocated) << "\n";
    std::cout << "  Bytes deallocated: " << formatBytes(total_stats.bytes_deallocated) << "\n";
    std::cout << "  Peak memory estimate: " << formatBytes(total_stats.peak_memory) << "\n";
    
    std::cout << "\nLeak Statistics:\n";
    std::cout << "  Leaked objects: " << leak_stats.object_count << "\n";
    std::cout << "  Leaked arrays: " << leak_stats.array_count << "\n";
    std::cout << "  Total leaks: " << leak_stats.count << "\n";
    std::cout << "  Leaked bytes: " << formatBytes(leak_stats.total_bytes) << "\n";
    
    std::cout << "\nPerformance:\n";
    std::cout << "  Total duration: " << total_stats.duration_ms << "ms\n";
    std::cout << "  Allocations/sec: " << (total_stats.allocations * 1000 / std::max(total_stats.duration_ms, 1UL)) << "\n";
    
    std::cout << "\nModule Breakdown:\n";
    std::map<std::string, WorkloadStats> module_stats;
    
    for (const auto& thread_results : all_results) {
        for (const auto& result : thread_results) {
            module_stats[result.module_name].merge(result.stats);
        }
    }
    
    for (const auto& [module_name, stats] : module_stats) {
        std::cout << "  " << module_name << ": " 
                  << stats.allocations << " allocs, "
                  << formatBytes(stats.bytes_allocated) << " bytes, "
                  << stats.duration_ms << "ms\n";
    }
    
    std::cout << "========================\n";
}

} // namespace mp

int main(int argc, char* argv[]) {
    mp::WorkloadConfig config;
    
    // Parse command line arguments
    if (!config.parseArgs(argc, argv)) {
        config.printUsage(argv[0]);
        return 1;
    }
    
    // Check for help flag
    mp::ArgParser parser(argc, argv);
    if (parser.hasFlag("--help")) {
        config.printUsage(argv[0]);
        return 0;
    }
    
    if (!config.quiet) {
        std::cout << "Starting memory workload with " << config.threads 
                  << " threads for " << config.seconds << " seconds\n";
        std::cout << "Scale: " << config.scale << ", Leak rate: " 
                  << (config.no_leaks ? 0.0 : config.leak_rate) << "\n";
#ifdef MP_USE_API
        if (MP_HAVE_API) {
            std::cout << "Profiler API enabled (snapshots every " << config.snapshot_every_ms << "ms)\n";
        } else {
            std::cout << "Profiler API requested but not available\n";
        }
#endif
        std::cout << std::endl;
    }
    
    // Start timing
    mp::Timer total_timer;
    std::atomic<bool> should_stop{false};
    
    // Results storage for all threads
    std::vector<std::vector<mp::ModuleResult>> all_results(config.threads);
    std::vector<std::mutex> results_mutexes(config.threads);
    std::vector<std::thread> threads;
    
    // Start worker threads
    for (uint32_t i = 0; i < config.threads; ++i) {
        threads.emplace_back(mp::workerThread, std::ref(config), i, 
                           std::ref(should_stop), std::ref(all_results[i]), std::ref(results_mutexes[i]));
    }
    
    // Start snapshot thread if API is available
    std::thread snapshot_thread;
#ifdef MP_USE_API
    if (MP_HAVE_API) {
        snapshot_thread = std::thread(mp::snapshotThread, std::ref(config), std::ref(should_stop));
    }
#endif
    
    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::seconds(config.seconds));
    
    // Signal threads to stop
    should_stop.store(true);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
#ifdef MP_USE_API
    if (snapshot_thread.joinable()) {
        snapshot_thread.join();
    }
#endif
    
    // Print summary
    mp::printSummary(config, all_results);
    
    if (!config.quiet) {
        std::cout << "Workload completed in " << total_timer.elapsedMillis() << "ms\n";
    }
    
    return 0;
}