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

// --- Estruturas ---
struct Rating {
    int movieId;
    float rating;
    
    Rating(int id, float r) : movieId(id), rating(r) {}
    
    // Operador de comparação para ordenação
    bool operator<(const Rating& other) const {
        if (movieId != other.movieId) {
            return movieId < other.movieId;
        }
        return rating < other.rating;
    }
};

struct DataChunk {
    char* start;
    char* end;
    std::unordered_map<int, std::vector<Rating>> local_user_data;
    std::unordered_map<int, int> local_movie_count;
};

// --- Funções Auxiliares ---
inline bool is_digit(char c);
inline int safe_fast_stoi(char*& p, char* end);
inline float safe_fast_stof(char*& p, char* end);
inline void safe_advance_to_next_line(char*& p, char* end);

// --- Funções Principais ---
void process_chunk(DataChunk* chunk);
const char* find_ratings_file();
int process_ratings_file();

#endif // PREPROCESSAMENTO_HPP