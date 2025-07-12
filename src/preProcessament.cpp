#include "preProcessament.hpp"


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
        val = (val << 3) + (val << 1) + (*p - '0');
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
    while (p < end && is_digit(*p))
    {
        val = val * 10.0f + (*p - '0');
        p++;
    }
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

    while (current_pos < end_pos && *current_pos != '\n')
        current_pos++;
    if (current_pos < end_pos)
        current_pos++;

    chunk->local_user_data.reserve(50000);
    chunk->local_movie_count.reserve(60000);

    while (current_pos < end_pos)
    {
        int userId = safe_fast_stoi(current_pos, end_pos);
        if (current_pos >= end_pos || *current_pos != ',')
        {
            safe_advance_to_next_line(current_pos, end_pos);
            continue;
        }
        current_pos++;

        int movieId = safe_fast_stoi(current_pos, end_pos);
        if (current_pos >= end_pos || *current_pos != ',')
        {
            safe_advance_to_next_line(current_pos, end_pos);
            continue;
        }
        current_pos++;

        float rating = safe_fast_stof(current_pos, end_pos);
        safe_advance_to_next_line(current_pos, end_pos);

        if (userId > 0 && movieId > 0 && rating >= 0.0f && rating <= 5.0f)
        {
            chunk->local_user_data[userId].emplace_back(movieId, rating);
            chunk->local_movie_count[movieId]++;
        }
    }
}

void filter_and_write_chunk(const DataChunk *chunk, const std::unordered_set<int> *valid_movies, int thread_id)
{
    std::string temp_filename = "datasets/input.dat.tmp." + std::to_string(thread_id);
    FILE *output_file = fopen(temp_filename.c_str(), "w");
    if (!output_file) { return; }

    const size_t BUFFER_SIZE = 4 * 1024 * 1024;
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

    char line_buffer[16384];
    std::vector<Rating> valid_ratings_for_user;
    valid_ratings_for_user.reserve(200);

    for (const auto &user_pair : chunk->local_user_data)
    {
        const auto &all_ratings = user_pair.second;

        valid_ratings_for_user.clear();
        for (const auto &rating : all_ratings)
        {
            if (valid_movies->count(rating.movieId))
            {
                valid_ratings_for_user.push_back(rating);
            }
        }

        if (valid_ratings_for_user.size() >= 50)
        {
            int userId = user_pair.first;
            char *ptr = line_buffer;
            char *const end_ptr = line_buffer + sizeof(line_buffer);

            auto [p1, ec1] = std::to_chars(ptr, end_ptr, userId);
            if (ec1 != std::errc())
                continue;
            ptr = p1;

            for (const auto &rating : valid_ratings_for_user)
            {
                if (end_ptr - ptr < 50)
                    break;
                *ptr++ = ' ';
                auto [p2, ec2] = std::to_chars(ptr, end_ptr, rating.movieId);
                if (ec2 != std::errc())
                    break;
                ptr = p2;
                *ptr++ = ':';
                auto [p3, ec3] = std::to_chars(ptr, end_ptr, rating.rating, std::chars_format::fixed, 1);
                if (ec3 != std::errc())
                    break;
                ptr = p3;
            }
            *ptr++ = '\n';
            add_to_buffer(line_buffer, ptr - line_buffer);
        }
    }

    flush_buffer();
    fclose(output_file);
}

void concatenate_temp_files(int num_threads)
{
    const char *final_filename = "datasets/input.dat";
    FILE *final_output = fopen(final_filename, "wb");
    if (!final_output){ return; }

    std::vector<char> concat_buffer(4 * 1024 * 1024);

    for (int i = 0; i < num_threads; ++i)
    {
        std::string temp_filename = "datasets/input.dat.tmp." + std::to_string(i);
        FILE *temp_input = fopen(temp_filename.c_str(), "rb");
        if (temp_input)
        {
            size_t bytes_read;
            while ((bytes_read = fread(concat_buffer.data(), 1, concat_buffer.size(), temp_input)) > 0)
            {
                fwrite(concat_buffer.data(), 1, bytes_read, final_output);
            }
            fclose(temp_input);
            remove(temp_filename.c_str());
        }
    }
    fclose(final_output);
}

int process_ratings_file()
{
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const char *filename = find_ratings_file();
    if (!filename)
    {
        return 1;
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
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
    madvise(file_data, sb.st_size, MADV_SEQUENTIAL);
    close(fd);

    char *current_pos = file_data;
    char *end_pos = file_data + sb.st_size;
    safe_advance_to_next_line(current_pos, end_pos);

    const int num_threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    const size_t data_size = end_pos - current_pos;
    const size_t chunk_size = data_size / num_threads;

    std::vector<DataChunk> chunks(num_threads);
    std::vector<std::thread> threads;

    // --- PASSO 1: CONTAGEM PARALELA ---
    for (int i = 0; i < num_threads; i++)
    {
        chunks[i].start = current_pos + (i * chunk_size);
        chunks[i].end = (i == num_threads - 1) ? end_pos : current_pos + ((i + 1) * chunk_size);
        threads.emplace_back(process_chunk, &chunks[i]);
    }
    for (auto &t : threads)
    {
        t.join();
    }

    std::unordered_map<int, int> movie_count;
    movie_count.reserve(60000);
    for (const auto &chunk : chunks)
    {
        for (const auto &movie_pair : chunk.local_movie_count)
        {
            movie_count[movie_pair.first] += movie_pair.second;
        }
    }

    std::unordered_set<int> valid_movies;
    valid_movies.reserve(20000);
    for (const auto &[movieId, count] : movie_count)
    {
        if (count >= 50)
        {
            valid_movies.insert(movieId);
        }
    }
    movie_count.clear();

    if (system("mkdir -p datasets 2>/dev/null") != 0)
    {
    }

    threads.clear();
    for (int i = 0; i < num_threads; i++)
    {
        threads.emplace_back(filter_and_write_chunk, &chunks[i], &valid_movies, i);
    }
    for (auto &t : threads)
    {
        t.join();
    }

    concatenate_temp_files(num_threads);

    munmap(file_data, sb.st_size);

    return 0;
}
const char *find_ratings_file()
{
    const char *possible_paths[] = {"ml-25m/ratings.csv", "datasets/ratings.csv", "ratings.csv"};
    for (const char *path : possible_paths)
    {
        if (access(path, F_OK) == 0)
        {
            return path;
        }
    }
    return nullptr;
}