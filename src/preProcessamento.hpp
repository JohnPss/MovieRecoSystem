#ifndef PREPROCESSAMENTO_HPP
#define PREPROCESSAMENTO_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <utility>
#include <algorithm>
#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>

// Para Memory-Mapped Files (Unix/Linux)
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>

// --- Estruturas Compactas ---
struct Rating {
    uint32_t movieId;
    uint8_t rating; // 0-50 (rating * 10)
    
    Rating(uint32_t id, uint8_t r) : movieId(id), rating(r) {}
    
    bool operator<(const Rating& other) const {
        return movieId < other.movieId;
    }
};

struct DataChunk {
    const char* start;
    const char* end;
    // Arrays paralelos são mais cache-friendly
    std::vector<uint32_t> user_ids;
    std::vector<std::vector<Rating>> user_ratings;
    std::unordered_map<uint32_t, uint32_t> movie_count;
};

// --- Funções Principais ---
const char* find_ratings_file();
int process_ratings_file();

#endif // PREPROCESSAMENTO_HPP