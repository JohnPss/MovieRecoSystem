#include "preProcessamento.hpp"
#include <immintrin.h>
#include <cstdint>

// Tabela de conversão ASCII->dígito
static const uint8_t digit_val[256] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    0,1,2,3,4,5,6,7,8,9,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};

// Parse ultra-rápido
__attribute__((always_inline)) inline uint32_t parse_uint(const char*& p) {
    uint32_t val = 0;
    uint8_t digit;
    while ((digit = digit_val[(uint8_t)*p]) < 10) {
        val = val * 10 + digit;
        p++;
    }
    return val;
}

__attribute__((always_inline)) inline uint8_t parse_rating(const char*& p) {
    uint8_t val = 0;
    uint8_t digit;
    
    while ((digit = digit_val[(uint8_t)*p]) < 10) {
        val = val * 10 + digit;
        p++;
    }
    val *= 10;
    
    if (*p == '.') {
        p++;
        if ((digit = digit_val[(uint8_t)*p]) < 10) {
            val += digit;
            p++;
        }
        while (digit_val[(uint8_t)*p] < 10) p++;
    }
    
    return val;
}

// Processamento de chunk otimizado
void process_chunk(DataChunk* chunk) {
    const char* p = chunk->start;
    const char* end = chunk->end;
    
    // Alinha com início de linha
    if (p != chunk->start) {
        while (p < end && *(p-1) != '\n') p++;
    }
    
    // Mapeia userId para índice no vetor
    std::unordered_map<uint32_t, size_t> user_index;
    chunk->movie_count.reserve(30000);
    
    while (p < end) {
        uint32_t userId = parse_uint(p);
        if (*p++ != ',') {
            while (p < end && *p != '\n') p++;
            if (p < end) p++;
            continue;
        }
        
        uint32_t movieId = parse_uint(p);
        if (*p++ != ',') {
            while (p < end && *p != '\n') p++;
            if (p < end) p++;
            continue;
        }
        
        uint8_t rating = parse_rating(p);
        while (p < end && *p != '\n') p++;
        if (p < end) p++;
        
        // Adiciona rating
        auto it = user_index.find(userId);
        if (it == user_index.end()) {
            size_t idx = chunk->user_ids.size();
            chunk->user_ids.push_back(userId);
            chunk->user_ratings.emplace_back();
            chunk->user_ratings.back().reserve(100);
            user_index[userId] = idx;
            chunk->user_ratings[idx].emplace_back(movieId, rating);
        } else {
            chunk->user_ratings[it->second].emplace_back(movieId, rating);
        }
        
        chunk->movie_count[movieId]++;
    }
}

const char* find_ratings_file() {
    const char* paths[] = {"ml-25m/ratings.csv", "datasets/ratings.csv", "ratings.csv"};
    for (const char* path : paths) {
        if (access(path, F_OK) == 0) return path;
    }
    return nullptr;
}

int process_ratings_file() {
    const char* filename = find_ratings_file();
    if (!filename) {
        std::cerr << "ERRO: ratings.csv não encontrado" << std::endl;
        return 1;
    }
    
    // Mapeia arquivo com flags otimizadas
    int fd = open(filename, O_RDONLY);
    if (fd == -1) return 1;
    
    struct stat sb;
    fstat(fd, &sb);
    
    void* mapped = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, fd, 0);
    if (mapped == MAP_FAILED) {
        mapped = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    }
    const char* file_data = (const char*)mapped;
    close(fd);
    
    // Pula header
    const char* data_start = file_data;
    while (*data_start && *data_start != '\n') data_start++;
    if (*data_start) data_start++;
    
    // Processamento paralelo
    const int num_threads = std::thread::hardware_concurrency();
    const size_t chunk_size = ((file_data + sb.st_size) - data_start) / num_threads;
    
    std::vector<DataChunk> chunks(num_threads);
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; i++) {
        chunks[i].start = data_start + (i * chunk_size);
        chunks[i].end = (i == num_threads - 1) ? file_data + sb.st_size : data_start + ((i + 1) * chunk_size);
        threads.emplace_back(process_chunk, &chunks[i]);
    }
    
    for (auto& t : threads) t.join();
    
    // Merge otimizado
    std::unordered_map<uint32_t, size_t> global_user_index;
    std::vector<std::vector<Rating>> all_ratings;
    std::unordered_map<uint32_t, uint32_t> movie_counts;
    
    size_t total_users = 0;
    for (const auto& chunk : chunks) {
        total_users += chunk.user_ids.size();
    }
    all_ratings.reserve(total_users);
    movie_counts.reserve(60000);
    
    // Merge chunks
    for (const auto& chunk : chunks) {
        for (size_t i = 0; i < chunk.user_ids.size(); i++) {
            uint32_t userId = chunk.user_ids[i];
            auto it = global_user_index.find(userId);
            if (it == global_user_index.end()) {
                global_user_index[userId] = all_ratings.size();
                all_ratings.push_back(chunk.user_ratings[i]);
            } else {
                auto& target = all_ratings[it->second];
                const auto& source = chunk.user_ratings[i];
                target.insert(target.end(), source.begin(), source.end());
            }
        }
        
        for (const auto& [movieId, count] : chunk.movie_count) {
            movie_counts[movieId] += count;
        }
    }
    
    munmap(mapped, sb.st_size);
    
    // Filmes válidos em vetor para binary search
    std::vector<uint32_t> valid_movies;
    valid_movies.reserve(20000);
    for (const auto& [movieId, count] : movie_counts) {
        if (count >= 50) valid_movies.push_back(movieId);
    }
    std::sort(valid_movies.begin(), valid_movies.end());
    
    // Escrita otimizada
    system("mkdir -p datasets 2>/dev/null");
    
    FILE* out = fopen("datasets/input.dat", "wb");
    if (!out) return 1;
    
    // Buffer gigante
    const size_t WRITE_BUFFER_SIZE = 128 * 1024 * 1024; // 128MB
    char* buffer = (char*)aligned_alloc(4096, WRITE_BUFFER_SIZE);
    setvbuf(out, buffer, _IOFBF, WRITE_BUFFER_SIZE);
    
    int written = 0;
    
    // Buffer temporário para linha
    std::vector<char> line(1024 * 1024);
    
    for (const auto& [userId, idx] : global_user_index) {
        auto& ratings = all_ratings[idx];
        if (ratings.size() < 50) continue;
        
        // Verifica se tem 50+ válidos
        int valid = 0;
        for (const auto& r : ratings) {
            if (std::binary_search(valid_movies.begin(), valid_movies.end(), r.movieId)) {
                if (++valid >= 50) break;
            }
        }
        if (valid < 50) continue;
        
        // Ordena
        std::sort(ratings.begin(), ratings.end());
        
        // Formata linha
        char* p = line.data();
        p += sprintf(p, "%u", userId);
        
        for (const auto& r : ratings) {
            if (std::binary_search(valid_movies.begin(), valid_movies.end(), r.movieId)) {
                if (r.rating % 10 == 0) {
                    p += sprintf(p, " %u:%u.0", r.movieId, r.rating / 10);
                } else {
                    p += sprintf(p, " %u:%u.%u", r.movieId, r.rating / 10, r.rating % 10);
                }
            }
        }
        *p++ = '\n';
        
        fwrite(line.data(), 1, p - line.data(), out);
        written++;
    }
    
    fclose(out);
    free(buffer);
    
    std::cout << "Pré-processamento concluído: " << written << " usuários escritos" << std::endl;
    return 0;
}