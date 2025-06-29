#include "DataLoader.hpp"
#include "Config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

using namespace std;
using namespace chrono;

// --- Funções Auxiliares de Parsing (para operar em char*) ---
namespace
{
    inline bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    // Função de parsing rápida e segura para uint32_t a partir de um buffer de char
    inline uint32_t fast_str_to_u32(char *&p, char *end)
    {
        uint32_t val = 0;
        while (p < end && is_digit(*p))
        {
            val = val * 10 + (*p - '0');
            p++;
        }
        return val;
    }

    // Função de parsing rápida e segura para float a partir de um buffer de char
    inline float fast_str_to_float(char *&p, char *end)
    {
        float val = 0.0f;
        while (p < end && is_digit(*p))
        {
            val = val * 10.0f + (*p - '0');
            p++;
        }
        if (p < end && *p == '.')
        {
            p++;
            float dec = 0.1f;
            while (p < end && is_digit(*p))
            {
                val += (*p - '0') * dec;
                dec *= 0.1f;
                p++;
            }
        }
        return val;
    }
}

DataLoader::DataLoader(
    unordered_map<uint32_t, UserProfile> &u,
    unordered_map<uint32_t, Movie> &m,
    unordered_map<string, int> &g,
    unordered_map<uint32_t, vector<pair<uint32_t, float>>> &mtu,
    unordered_map<uint32_t, vector<uint32_t>> &gtm,
    float &gar,
    unordered_map<uint32_t, float> &mar,
    unordered_map<uint32_t, int> &mp) : users(u), movies(m), genreToId(g), movieToUsers(mtu),
                                        genreToMovies(gtm), globalAvgRating(gar),
                                        movieAvgRatings(mar), moviePopularity(mp) {}

void DataLoader::loadRatings(const string &filename)
{
    cout << "Carregando ratings usando mmap..." << endl;
    auto start = high_resolution_clock::now();

    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
    {
        cerr << "Erro ao abrir " << filename << endl;
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        close(fd);
        cerr << "Erro ao obter status do arquivo " << filename << endl;
        return;
    }

    char *file_data = (char *)mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED)
    {
        close(fd);
        cerr << "Erro ao mapear arquivo na memoria." << endl;
        return;
    }
    close(fd);

    madvise(file_data, sb.st_size, MADV_SEQUENTIAL);

    int num_threads = min((int)thread::hardware_concurrency(), max(1, (int)(sb.st_size / 5000000)));
    cout << "Usando " << num_threads << " threads para processar " << sb.st_size / (1024 * 1024) << " MB de dados" << endl;

    struct ThreadData
    {
        unordered_map<uint32_t, UserProfile> users;
        unordered_map<uint32_t, vector<pair<uint32_t, float>>> movieToUsers;
        unordered_map<uint32_t, float> movieSums;
        unordered_map<uint32_t, int> movieCounts;
        double ratingSum = 0;
        uint64_t ratingCount = 0;
    };

    vector<ThreadData> threadData(num_threads);
    vector<thread> threads;

    size_t chunk_size = sb.st_size / num_threads;

    for (int t = 0; t < num_threads; ++t)
    {
        char *chunk_start = file_data + t * chunk_size;
        char *chunk_end = (t == num_threads - 1) ? (file_data + sb.st_size) : (chunk_start + chunk_size);

        if (t > 0)
        {
            while (chunk_start < chunk_end && *(chunk_start - 1) != '\n')
            {
                chunk_start++;
            }
        }

        threads.emplace_back([&, chunk_start, chunk_end, t]()
                             {
            auto &data = threadData[t];
            char *p = chunk_start;

            while (p < chunk_end) {
                while (p < chunk_end && (*p == '\n' || *p == '\r')) p++;
                if (p >= chunk_end) break;

                uint32_t userId = fast_str_to_u32(p, chunk_end);
                while (p < chunk_end && (*p == ' ' || *p == '\t')) p++;

                UserProfile &user = data.users[userId];
                user.ratings.clear(); // <<<<<<< CORREÇÃO CRÍTICA
                
                float sumRatings = 0.0f;
                int ratings_in_line = 0;

                while (p < chunk_end && *p != '\n' && *p != '\r') {
                    uint32_t movieId = fast_str_to_u32(p, chunk_end);
                    p++; // Pula ':'
                    float rating = fast_str_to_float(p, chunk_end);
                    while (p < chunk_end && (*p == ' ' || *p == '\t')) p++;

                    user.ratings.emplace_back(movieId, rating);
                    data.movieToUsers[movieId].emplace_back(userId, rating);

                    sumRatings += rating;
                    data.movieSums[movieId] += rating;
                    data.movieCounts[movieId]++;
                    ratings_in_line++;
                }
                
                if (ratings_in_line > 0) {
                    data.ratingSum += sumRatings; // <<<<<<< CORREÇÃO
                    data.ratingCount += ratings_in_line; // <<<<<<< CORREÇÃO
                    user.avgRating = sumRatings / ratings_in_line; // <<<<<<< CORREÇÃO
                    sort(user.ratings.begin(), user.ratings.end());
                }
                while (p < chunk_end && (*p == '\n' || *p == '\r')) p++;
            } });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    munmap(file_data, sb.st_size);

    uint64_t totalRatings = 0;
    double totalSum = 0.0;

    for (const auto &data : threadData)
    {
        for (const auto &[userId, profile] : data.users)
        {
            users[userId] = profile;
        }

        for (const auto &[movieId, ratings] : data.movieToUsers)
        {
            movieToUsers[movieId].insert(movieToUsers[movieId].end(), ratings.begin(), ratings.end());
        }

        totalSum += data.ratingSum;
        totalRatings += data.ratingCount;

        for (const auto &[movieId, sum] : data.movieSums)
        {
            movieAvgRatings[movieId] += sum;
        }
        for (const auto &[movieId, count] : data.movieCounts)
        {
            moviePopularity[movieId] += count;
        }
    }

    globalAvgRating = (totalRatings > 0) ? (totalSum / totalRatings) : 0.0f;

    for (auto &[movieId, sum] : movieAvgRatings)
    {
        if (moviePopularity[movieId] > 0)
        {
            sum /= moviePopularity[movieId];
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Ratings carregados em " << duration.count() << "ms" << endl;
    cout << "Usuários: " << users.size() << ", Ratings: " << totalRatings << endl;
}

// As funções abaixo permanecem inalteradas
void DataLoader::loadMovies(const string &filename)
{
    cout << "Carregando filmes..." << endl;
    auto start = high_resolution_clock::now();

    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir " << filename << endl;
        return;
    }

    movies.reserve(65000);
    genreToMovies.reserve(25);

    string line;
    getline(file, line); // Skip header

    int genreCounter = 0;

    while (getline(file, line))
    {
        istringstream iss(line);
        string movieIdStr, title, genres;

        getline(iss, movieIdStr, ',');
        getline(iss, title, ',');
        getline(iss, genres);

        if (!title.empty() && title.front() == '"')
            title = title.substr(1, title.length() - 2);

        uint32_t movieId = stoul(movieIdStr);
        Movie &movie = movies[movieId];
        movie.title = title;
        movie.genreBitmask = 0;
        movie.genres.reserve(5);

        istringstream genreStream(genres);
        string genre;
        while (getline(genreStream, genre, '|'))
        {
            movie.genres.push_back(genre);

            if (genreToId.find(genre) == genreToId.end())
            {
                genreToId[genre] = genreCounter++;
            }

            int genreId = genreToId[genre];
            movie.genreBitmask |= (1 << genreId);
            genreToMovies[genreId].push_back(movieId);
        }
    }

    auto parseEnd = high_resolution_clock::now();
    cout << "Parse dos filmes: " << duration_cast<milliseconds>(parseEnd - start).count() << "ms" << endl;

    calculateUserPreferences();

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Filmes carregados em " << duration.count() << "ms" << endl;
}

void DataLoader::calculateUserPreferences()
{
    cout << "Calculando preferências de " << users.size() << " usuários..." << endl;
    auto start = high_resolution_clock::now();

    if (users.size() < 10000)
    {
        for (auto &[userId, user] : users)
        {
            unordered_map<int, float> genreScores;
            for (const auto &[movieId, rating] : user.ratings)
            {
                if (rating >= Config::MIN_RATING && movies.count(movieId))
                {
                    for (const auto &genre : movies.at(movieId).genres)
                    {
                        genreScores[genreToId[genre]] += rating - Config::MIN_RATING;
                    }
                }
            }

            vector<pair<int, float>> sortedGenres(genreScores.begin(), genreScores.end());
            sort(sortedGenres.begin(), sortedGenres.end(),
                 [](const auto &a, const auto &b)
                 { return a.second > b.second; });

            user.preferredGenres = 0;
            for (int i = 0; i < min(5, (int)sortedGenres.size()); ++i)
            {
                user.preferredGenres |= (1 << sortedGenres[i].first);
            }
        }
    }
    else
    {
        vector<UserProfile *> userPtrs;
        userPtrs.reserve(users.size());
        for (auto &pair : users)
        {
            userPtrs.push_back(&pair.second);
        }

        const int num_threads = min((int)thread::hardware_concurrency(), max(1, (int)userPtrs.size() / 5000));
        cout << "Usando " << num_threads << " threads para calcular preferências..." << endl;

        vector<thread> threads;
        atomic<int> processed(0);
        size_t chunk_size = userPtrs.size() / num_threads;

        for (int t = 0; t < num_threads; t++)
        {
            size_t start_idx = t * chunk_size;
            size_t end_idx = (t == num_threads - 1) ? userPtrs.size() : (t + 1) * chunk_size;

            threads.emplace_back([&, start_idx, end_idx]()
                                 {
                for (size_t i = start_idx; i < end_idx; i++) {
                    auto& user = *userPtrs[i];
                    unordered_map<int, float> genreScores;
                    
                    for (const auto &[movieId, rating] : user.ratings) {
                        if (rating >= Config::MIN_RATING) {
                            auto movieIt = movies.find(movieId);
                            if (movieIt != movies.end()) {
                                uint32_t movieGenres = movieIt->second.genreBitmask;
                                for (int g = 0; g < 32 && movieGenres; g++) {
                                    if (movieGenres & (1 << g)) {
                                        genreScores[g] += rating - Config::MIN_RATING;
                                        movieGenres &= ~(1 << g);
                                    }
                                }
                            }
                        }
                    }
                    
                    if (!genreScores.empty()) {
                        vector<pair<float, int>> sortedGenres;
                        sortedGenres.reserve(genreScores.size());
                        for (const auto& [genreId, score] : genreScores) {
                            sortedGenres.push_back({score, genreId});
                        }
                        
                        int topN = min(5, (int)sortedGenres.size());
                        partial_sort(sortedGenres.begin(), sortedGenres.begin() + topN, sortedGenres.end(), greater<pair<float, int>>());
                        
                        user.preferredGenres = 0;
                        for (int j = 0; j < topN; j++) {
                            user.preferredGenres |= (1 << sortedGenres[j].second);
                        }
                    }
                    
                    int count = ++processed;
                    if (count % 50000 == 0) {
                        cout << "Processadas preferências de " << count << " usuários..." << endl;
                    }
                } });
        }
        for (auto &t : threads)
        {
            t.join();
        }
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Preferências calculadas em " << duration.count() << "ms" << endl;
}

vector<uint32_t> DataLoader::loadUsersToRecommend(const string &filename)
{
    vector<uint32_t> userIds;
    ifstream file(filename);

    if (!file.is_open())
    {
        cerr << "Erro ao abrir " << filename << endl;
        return userIds;
    }

    string line;
    while (getline(file, line))
    {
        userIds.push_back(stoul(line));
    }

    return userIds;
}