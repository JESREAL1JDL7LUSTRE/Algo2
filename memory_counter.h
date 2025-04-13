#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <iostream>

inline void printMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX memCounter;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memCounter, sizeof(memCounter))) {
        std::cout << "========== Memory Usage ==========" << std::endl;
        std::cout << "Working Set Size (Current RAM used): " 
                  << memCounter.WorkingSetSize / 1024 << " KB" << std::endl;
        std::cout << "Peak Working Set Size: " 
                  << memCounter.PeakWorkingSetSize / 1024 << " KB" << std::endl;
        std::cout << "Pagefile Usage (Committed Virtual Memory): " 
                  << memCounter.PagefileUsage / 1024 << " KB" << std::endl;
        std::cout << "Private Usage (Exclusive to process): " 
                  << memCounter.PrivateUsage / 1024 << " KB" << std::endl;
        std::cout << "===================================" << std::endl;
    } else {
        std::cerr << "Failed to retrieve memory usage information." << std::endl;
    }
}

#endif // MEMORY_UTILS_H
