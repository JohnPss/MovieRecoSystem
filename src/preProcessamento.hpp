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
#include <immintrin.h>

// Para Memory-Mapped Files (Unix/Linux)
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>

// --- Estruturas Ultra-Compactas ---
#pragma pack(push, 1)
struct CompactRating {
    uint32_t movieId : 24;  // Suporta até 16M filmes
    uint8_t rating;         // Rating * 10 (0-50)
    
    CompactRating(uint32_t id, uint8_t r) : movieId(id), rating(r) {}
};
#pragma pack(pop)

struct alignas(64) DataChunk {  // Alinhado em cache line
    const char* start;
    const char* end;
    // Vetores planos para melhor cache
    std::vector<uint32_t> user_ids;
    std::vector<uint32_t> user_offsets;  // Onde começam os ratings de cada user
    std::vector<CompactRating> all_ratings;
    std::vector<std::pair<uint32_t, uint32_t>> movie_counts;  // movieId, count
};

// Tabela de lookup para parsing
extern const uint8_t digit_lookup[256];

// --- Funções Principais ---
const char* find_ratings_file();
int process_ratings_file();

#endif // PREPROCESSAMENTO_HPP