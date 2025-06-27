#include "preProcessamento.hpp"
#include <unordered_set>

// --- Funções Auxiliares de Parsing Otimizadas ---

inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

inline int safe_fast_stoi(char *&p, char *end)
{
    if (p >= end)
        return 0;

    int val = 0;
    bool negative = (*p == '-');
    if (negative)
    {
        p++;
        if (p >= end)
            return 0;
    }

    while (p < end && is_digit(*p))
    {
        val = (val << 3) + (val << 1) + (*p - '0'); // val * 10 otimizado
        p++;
    }
    return negative ? -val : val;
}

inline float safe_fast_stof(char *&p, char *end)
{
    if (p >= end)
        return 0.0f;

    float val = 0.0f;
    bool negative = (*p == '-');
    if (negative)
    {
        p++;
        if (p >= end)
            return 0.0f;
    }

    // Parte inteira
    while (p < end && is_digit(*p))
    {
        val = val * 10.0f + (*p - '0');
        p++;
    }

    // Parte decimal
    if (p < end && *p == '.')
    {
        p++;
        float decimal_multiplier = 0.1f;
        while (p < end && is_digit(*p))
        {
            val += (*p - '0') * decimal_multiplier;
            decimal_multiplier *= 0.1f;
            p++;
        }
    }
    return negative ? -val : val;
}

inline void safe_advance_to_next_line(char *&p, char *end)
{
    while (p < end && *p != '\n')
        p++;
    if (p < end)
        p++;
}

void process_chunk(DataChunk *chunk)
{
    char *current_pos = chunk->start;
    char *end_pos = chunk->end;

    // Avança para o início de uma linha válida
    while (current_pos < end_pos && *current_pos != '\n')
        current_pos++;
    if (current_pos < end_pos)
        current_pos++;

    // OTIMIZAÇÃO: Reservas mais realistas
    chunk->local_user_data.reserve(50000);   // Aumentado
    chunk->local_movie_count.reserve(60000);  // Ajustado

    while (current_pos < end_pos)
    {
        // Parse userId
        int userId = safe_fast_stoi(current_pos, end_pos);
        if (current_pos >= end_pos || *current_pos != ',')
        {
            safe_advance_to_next_line(current_pos, end_pos);
            continue;
        }
        current_pos++;

        // Parse movieId
        int movieId = safe_fast_stoi(current_pos, end_pos);
        if (current_pos >= end_pos || *current_pos != ',')
        {
            safe_advance_to_next_line(current_pos, end_pos);
            continue;
        }
        current_pos++;

        // Parse rating
        float rating = safe_fast_stof(current_pos, end_pos);

        // Pula timestamp (não precisamos dele)
        safe_advance_to_next_line(current_pos, end_pos);

        // Validação rápida e inserção direta
        if (userId > 0 && movieId > 0 && rating >= 0.0f && rating <= 5.0f)
        {
            chunk->local_user_data[userId].emplace_back(movieId, rating);
            chunk->local_movie_count[movieId]++;
        }
    }
}

const char *find_ratings_file()
{
    const char *possible_paths[] = {
        "ml-25m/ratings.csv",
        "datasets/ratings.csv",
        "ratings.csv"};

    for (const char *path : possible_paths)
    {
        if (access(path, F_OK) == 0)
        {
            return path;
        }
    }
    return nullptr;
}

int process_ratings_file()
{
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Encontrar arquivo
    const char *filename = find_ratings_file();

    if (!filename)
    {
        std::cerr << "ERRO: ratings.csv não encontrado" << std::endl;
        return 1;
    }

    // --- Mapeamento do Arquivo ---
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        std::cerr << "ERRO ao abrir " << filename << std::endl;
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        close(fd);
        return 1;
    }

    char *file_data = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED)
    {
        close(fd);
        return 1;
    }
    
    // OTIMIZAÇÃO: Advise ao kernel sobre padrão de acesso
    madvise(file_data, sb.st_size, MADV_SEQUENTIAL);
    close(fd);

    char *current_pos = file_data;
    char *end_pos = file_data + sb.st_size;

    // Pula cabeçalho
    safe_advance_to_next_line(current_pos, end_pos);

    // --- Processamento Paralelo ---
    const int num_threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    const size_t data_size = end_pos - current_pos;
    const size_t chunk_size = data_size / num_threads;

    std::vector<DataChunk> chunks(num_threads);
    std::vector<std::thread> threads;

    // Dividir dados em chunks
    for (int i = 0; i < num_threads; i++)
    {
        chunks[i].start = current_pos + (i * chunk_size);
        chunks[i].end = (i == num_threads - 1) ? end_pos : current_pos + ((i + 1) * chunk_size);
    }

    // Processar chunks em paralelo
    for (int i = 0; i < num_threads; i++)
    {
        threads.emplace_back(process_chunk, &chunks[i]);
    }

    // Aguardar threads
    for (auto &t : threads)
    {
        t.join();
    }

    // --- Merge dos Resultados ---
    std::unordered_map<int, std::vector<Rating>> avaliacoes;
    std::unordered_map<int, int> movie_count;

    // OTIMIZAÇÃO: Reservas mais precisas
    avaliacoes.reserve(300000);   // MovieLens tem ~280k usuários
    movie_count.reserve(60000);    // ~60k filmes

    for (const auto &chunk : chunks)
    {
        // Merge user data
        for (const auto &user_pair : chunk.local_user_data)
        {
            auto &target_ratings = avaliacoes[user_pair.first];
            const auto &source_ratings = user_pair.second;
            target_ratings.insert(target_ratings.end(), source_ratings.begin(), source_ratings.end());
        }

        // Merge movie counts
        for (const auto &movie_pair : chunk.local_movie_count)
        {
            movie_count[movie_pair.first] += movie_pair.second;
        }
    }

    munmap(file_data, sb.st_size);

    // --- Filtrar filmes com mais de 50 avaliações ---
    std::unordered_set<int> valid_movies;
    valid_movies.reserve(20000);  // ~20k filmes passam o filtro

    for (const auto &movie_pair : movie_count)
    {
        if (movie_pair.second > 50)
        {
            valid_movies.insert(movie_pair.first);
        }
    }

    // --- Escrita Otimizada ---
    if (system("mkdir -p datasets 2>/dev/null") != 0)
    {
        // Ignora erro se diretório já existe
    }

    FILE *output_file = fopen("datasets/input.dat", "w");
    if (!output_file)
    {
        std::cerr << "ERRO ao criar arquivo de saída" << std::endl;
        return 1;
    }

    // OTIMIZAÇÃO: Buffer muito maior para escrita
    const size_t BUFFER_SIZE = 4 * 1024 * 1024; // 4MB
    std::vector<char> write_buffer(BUFFER_SIZE);
    size_t buffer_pos = 0;

    auto flush_buffer = [&]()
    {
        if (buffer_pos > 0)
        {
            fwrite(write_buffer.data(), 1, buffer_pos, output_file);
            buffer_pos = 0;
        }
    };

    auto add_to_buffer = [&](const char *data, size_t len)
    {
        if (buffer_pos + len >= BUFFER_SIZE)
        {
            flush_buffer();
        }
        memcpy(write_buffer.data() + buffer_pos, data, len);
        buffer_pos += len;
    };

    // OTIMIZAÇÃO PRINCIPAL: Escreve diretamente sem criar estrutura intermediária
    char line_buffer[16384];  // Buffer maior
    int users_written = 0;

    for (const auto &[userId, ratings] : avaliacoes)
    {
        if (ratings.size() <= 50) continue;

        // Conta ratings válidos on-the-fly
        int valid_count = 0;
        for (const auto &rating : ratings)
        {
            if (valid_movies.count(rating.movieId))
            {
                valid_count++;
            }
        }

        if (valid_count <= 50) continue;

        // Escreve diretamente
        char *buf_ptr = line_buffer;
        buf_ptr += sprintf(buf_ptr, "%d", userId);

        for (const auto &rating : ratings)
        {
            if (valid_movies.count(rating.movieId))
            {
                buf_ptr += sprintf(buf_ptr, " %d:%.1f", rating.movieId, rating.rating);

                // Proteção contra overflow
                if (buf_ptr - line_buffer > 15000) break;
            }
        }

        *buf_ptr++ = '\n';
        add_to_buffer(line_buffer, buf_ptr - line_buffer);
        users_written++;
    }

    flush_buffer();
    fclose(output_file);

    std::cout << "Pré-processamento concluído: " << users_written << " usuários escritos" << std::endl;

    return 0;
}