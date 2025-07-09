#ifndef PREPROCESSAMENTO_HPP
#define PREPROCESSAMENTO_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <unordered_set> // Adicionado
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

// --- Estruturas (sem alterações) ---
struct Rating {
    int movieId;
    float rating;
    
    Rating(int id, float r) : movieId(id), rating(r) {}
    
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

// --- Funções Auxiliares de Parsing (sem alterações) ---
inline bool is_digit(char c);
inline int safe_fast_stoi(char*& p, char* end);
inline float safe_fast_stof(char*& p, char* end);
inline void safe_advance_to_next_line(char*& p, char* end);

// --- Funções Principais e de Otimização ---

// PASSO 1: Processa o chunk inicial para contagem (função original)
void process_chunk(DataChunk* chunk);

// PASSO 2: Nova função para filtrar dados e escrever em arquivos temporários
void filter_and_write_chunk(const DataChunk* chunk, const std::unordered_set<int>* valid_movies, int thread_id);

// PASSO 3: Nova função para juntar os arquivos temporários
void concatenate_temp_files(int num_threads);

// Função principal de orquestração
int process_ratings_file();

// Função utilitária (sem alterações)
const char* find_ratings_file();

#endif // PREPROCESSAMENTO_HPP