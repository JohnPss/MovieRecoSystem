#include "preProcessamento.hpp"

#include <unordered_set>
#include <x86intrin.h>

// Tabela de lookup para conversão rápida ASCII->dígito
alignas(64) const uint8_t digit_lookup[256] = {
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

// Parse ultra-rápido sem branches
__attribute__((always_inline)) inline uint32_t parse_int_fast(const char*& p) {
    uint32_t val = 0;
    uint8_t digit;
    
    // Unroll loop para melhor performance
    while ((digit = digit_lookup[(uint8_t)*p]) < 10) {
        val = val * 10 + digit;
        p++;
    }
    return val;
}

__attribute__((always_inline)) inline uint8_t parse_rating_fast(const char*& p) {
    uint8_t val = digit_lookup[(uint8_t)*p++] * 10;
    
    if (digit_lookup[(uint8_t)*p] < 10) {
        val += digit_lookup[(uint8_t)*p++];
    }
    
    if (*p == '.') {
        p++;
        if (digit_lookup[(uint8_t)*p] < 10) {
            val += digit_lookup[(uint8_t)*p++];
        } else {
            val *= 10;
        }
        // Pula dígitos extras
        while (digit_lookup[(uint8_t)*p] < 10) p++;
    } else {
        val *= 10;
    }
    
    return val;
}

// Processamento de chunk otimizado
void process_chunk_optimized(DataChunk* chunk) {
    const char* p = chunk->start;
    const char* end = chunk->end;
    
    // Pré-aloca vetores
    chunk->user_ids.reserve(80000);
    chunk->user_offsets.reserve(80000);
    chunk->all_ratings.reserve(2000000);
    
    // Hash map local para contagem de filmes
    std::unordered_map<uint32_t, uint32_t> local_movie_count;
    local_movie_count.reserve(30000);
    
    // Hash map para mapear userId -> índice
    std::unordered_map<uint32_t, uint32_t> user_index_map;
    user_index_map.reserve(80000);
    
    // Alinha com início de linha
    if (p != chunk->start) {
        while (p < end && *(p-1) != '\n') p++;
    }
    
    while (p < end - 20) {  // -20 para garantir que não passamos do fim
        // Parse userId
        uint32_t userId = parse_int_fast(p);
        if (*p++ != ',') {
            while (p < end && *p != '\n') p++;
            p++;
            continue;
        }
        
        // Parse movieId
        uint32_t movieId = parse_int_fast(p);
        if (*p++ != ',') {
            while (p < end && *p != '\n') p++;
            p++;
            continue;
        }
        
        // Parse rating
        uint8_t rating = parse_rating_fast(p);
        while (p < end && *p != '\n') p++;
        p++;
        
        // Adiciona rating
        auto it = user_index_map.find(userId);
        if (it == user_index_map.end()) {
            uint32_t idx = chunk->user_ids.size();
            chunk->user_ids.push_back(userId);
            chunk->user_offsets.push_back(chunk->all_ratings.size());
            user_index_map[userId] = idx;
        }
        
        chunk->all_ratings.emplace_back(movieId, rating);
        local_movie_count[movieId]++;
    }
    
    // Finaliza último user offset
    chunk->user_offsets.push_back(chunk->all_ratings.size());
    
    // Converte movie counts para vetor
    chunk->movie_counts.reserve(local_movie_count.size());
    for (const auto& pair : local_movie_count) {
        chunk->movie_counts.push_back(pair);
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
    
    // Mapeia arquivo com huge pages se possível
    int fd = open(filename, O_RDONLY);
    if (fd == -1) return 1;
    
    struct stat sb;
    fstat(fd, &sb);
    
    // Tenta huge pages primeiro
    void* mapped = mmap(NULL, sb.st_size, PROT_READ, 
                       MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, fd, 0);
    if (mapped == MAP_FAILED) {
        mapped = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    }
    const char* file_data = (const char*)mapped;
    close(fd);
    
    // Pula header
    const char* data_start = file_data;
    while (*data_start && *data_start != '\n') data_start++;
    data_start++;
    
    // Processamento paralelo
    const int num_threads = std::thread::hardware_concurrency();
    const size_t chunk_size = ((file_data + sb.st_size) - data_start) / num_threads;
    
    std::vector<DataChunk> chunks(num_threads);
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; i++) {
        chunks[i].start = data_start + (i * chunk_size);
        chunks[i].end = (i == num_threads - 1) ? file_data + sb.st_size : data_start + ((i + 1) * chunk_size);
        threads.emplace_back(process_chunk_optimized, &chunks[i]);
    }
    
    for (auto& t : threads) t.join();
    
    // Merge otimizado dos chunks
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> global_users; // userId -> (offset, count)
    std::vector<CompactRating> all_ratings;
    std::unordered_map<uint32_t, uint32_t> movie_counts;
    
    // Calcula tamanho total
    size_t total_ratings = 0;
    for (const auto& chunk : chunks) {
        total_ratings += chunk.all_ratings.size();
    }
    all_ratings.reserve(total_ratings);
    movie_counts.reserve(60000);
    
    // Merge ratings
    for (const auto& chunk : chunks) {
        for (size_t i = 0; i < chunk.user_ids.size(); i++) {
            uint32_t userId = chunk.user_ids[i];
            uint32_t start = chunk.user_offsets[i];
            uint32_t end = chunk.user_offsets[i + 1];
            uint32_t count = end - start;
            
            auto it = global_users.find(userId);
            if (it == global_users.end()) {
                global_users[userId] = {all_ratings.size(), count};
                all_ratings.insert(all_ratings.end(), 
                                 chunk.all_ratings.begin() + start,
                                 chunk.all_ratings.begin() + end);
            } else {
                // Usuário já existe, adiciona ratings
                all_ratings.insert(all_ratings.end(), 
                                 chunk.all_ratings.begin() + start,
                                 chunk.all_ratings.begin() + end);
                it->second.second += count;
            }
        }
        
        // Merge movie counts
        for (const auto& pair : chunk.movie_counts) {
            movie_counts[pair.first] += pair.second;
        }
    }
    
    munmap(mapped, sb.st_size);
    
    // Filtra filmes válidos
    std::unordered_set<uint32_t> valid_movies;
    valid_movies.reserve(20000);
    for (const auto& [movieId, count] : movie_counts) {
        if (count >= 50) valid_movies.insert(movieId);
    }
    
    // Escrita ultra-otimizada
    system("mkdir -p datasets 2>/dev/null");
    
    int out_fd = open("datasets/input.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1) return 1;
    
    // Buffer gigante alinhado
    const size_t BUFFER_SIZE = 128 * 1024 * 1024; // 128MB
    char* buffer = (char*)aligned_alloc(4096, BUFFER_SIZE);
    size_t buffer_pos = 0;
    
    // Lambda para flush
    auto flush = [&]() {
        if (buffer_pos > 0) {
            write(out_fd, buffer, buffer_pos);
            buffer_pos = 0;
        }
    };
    
    // Buffer local para formatação
    char line[131072]; // 128KB
    int written = 0;
    
    for (const auto& [userId, info] : global_users) {
        uint32_t offset = info.first;
        uint32_t count = info.second;
        
        if (count < 50) continue;
        
        // Conta ratings válidos
        int valid_count = 0;
        for (uint32_t i = 0; i < count; i++) {
            if (valid_movies.count(all_ratings[offset + i].movieId)) {
                valid_count++;
                if (valid_count >= 50) break;
            }
        }
        
        if (valid_count < 50) continue;
        
        // Formata linha
        char* p = line;
        p += sprintf(p, "%u", userId);
        
        for (uint32_t i = 0; i < count; i++) {
            const auto& r = all_ratings[offset + i];
            if (valid_movies.count(r.movieId)) {
                if (r.rating % 10 == 0) {
                    p += sprintf(p, " %u:%u.0", (uint32_t)r.movieId, r.rating / 10);
                } else {
                    p += sprintf(p, " %u:%u.%u", (uint32_t)r.movieId, r.rating / 10, r.rating % 10);
                }
            }
        }
        *p++ = '\n';
        
        size_t line_len = p - line;
        if (buffer_pos + line_len > BUFFER_SIZE - 131072) {
            flush();
        }
        
        memcpy(buffer + buffer_pos, line, line_len);
        buffer_pos += line_len;
        written++;
    }
    
    flush();
    close(out_fd);
    free(buffer);
    
    std::cout << "Pré-processamento concluído: " << written << " usuários escritos" << std::endl;
    return 0;
}