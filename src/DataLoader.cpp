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

using namespace std;
using namespace chrono;

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
    cout << "Carregando ratings..." << endl;
    auto start = high_resolution_clock::now();

    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir " << filename << endl;
        return;
    }

    // Lê todas as linhas do arquivo
    vector<string> lines;
    string line;
    while (getline(file, line))
    {
        lines.push_back(line);
    }
    file.close();

    // Determina número de threads (mínimo 1, máximo NUM_THREADS)
    int num_threads = min(Config::NUM_THREADS, max(1, (int)lines.size() / 10000));
    cout << "Usando " << num_threads << " threads para processar " << lines.size() << " usuários" << endl;

    // Estruturas temporárias para cada thread
    struct ThreadData
    {
        unordered_map<uint32_t, UserProfile> users;
        unordered_map<uint32_t, vector<pair<uint32_t, float>>> movieToUsers;
        unordered_map<uint32_t, float> movieSums;
        unordered_map<uint32_t, int> movieCounts;
        float ratingSum = 0;
        int ratingCount = 0;
    };

    vector<ThreadData> threadData(num_threads);
    vector<thread> threads;

    // Divide o trabalho entre threads
    size_t chunk_size = lines.size() / num_threads;

    for (int t = 0; t < num_threads; t++)
    {
        size_t start_idx = t * chunk_size;
        size_t end_idx = (t == num_threads - 1) ? lines.size() : (t + 1) * chunk_size;

        threads.emplace_back([&, t, start_idx, end_idx]()
                             {
            auto& data = threadData[t];
            
            // Processa as linhas deste chunk
            for (size_t i = start_idx; i < end_idx; i++) {
                istringstream iss(lines[i]);
                uint32_t userId;
                iss >> userId;

                UserProfile &user = data.users[userId];
                user.ratings.clear();

                string ratingPair;
                float sumRatings = 0.0f;

                while (iss >> ratingPair) {
                    size_t colonPos = ratingPair.find(':');
                    if (colonPos != string::npos) {
                        uint32_t movieId = stoul(ratingPair.substr(0, colonPos));
                        float rating = stof(ratingPair.substr(colonPos + 1));

                        user.ratings.push_back({movieId, rating});
                        data.movieToUsers[movieId].push_back({userId, rating});

                        sumRatings += rating;
                        data.ratingSum += rating;
                        data.movieSums[movieId] += rating;
                        data.movieCounts[movieId]++;
                        data.ratingCount++;
                    }
                }

                user.avgRating = user.ratings.empty() ? 0.0f : sumRatings / user.ratings.size();
                sort(user.ratings.begin(), user.ratings.end());
            } });
    }

    // Aguarda todas as threads
    for (auto &t : threads)
    {
        t.join();
    }

    // Merge dos resultados
    int totalRatings = 0;
    globalAvgRating = 0.0f;

    for (const auto &data : threadData)
    {
        // Merge users
        users.insert(data.users.begin(), data.users.end());

        // Merge movieToUsers
        for (const auto &[movieId, ratings] : data.movieToUsers)
        {
            movieToUsers[movieId].insert(
                movieToUsers[movieId].end(),
                ratings.begin(),
                ratings.end());
        }

        // Acumula estatísticas
        globalAvgRating += data.ratingSum;
        totalRatings += data.ratingCount;

        // Merge somas dos filmes
        for (const auto &[movieId, sum] : data.movieSums)
        {
            movieAvgRatings[movieId] += sum;
        }

        for (const auto &[movieId, count] : data.movieCounts)
        {
            moviePopularity[movieId] += count;
        }
    }

    // Finaliza cálculos
    globalAvgRating /= totalRatings;

    for (auto &[movieId, sum] : movieAvgRatings)
    {
        sum /= moviePopularity[movieId];
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Ratings carregados em " << duration.count() << "ms" << endl;
    cout << "Usuários: " << users.size() << ", Ratings: " << totalRatings << endl;
}

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

    // OTIMIZAÇÃO 1: Reserve espaço
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
        movie.genres.reserve(5); // OTIMIZAÇÃO 2: Reserve espaço para gêneros

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

    // OTIMIZAÇÃO 3: Mostra tempo do parse para debug
    auto parseEnd = high_resolution_clock::now();
    cout << "Parse dos filmes: " << duration_cast<milliseconds>(parseEnd - start).count() << "ms" << endl;

    // Calcula gêneros preferidos dos usuários
    calculateUserPreferences();

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Filmes carregados em " << duration.count() << "ms" << endl;
}

void DataLoader::calculateUserPreferences()
{
    cout << "Calculando preferências de " << users.size() << " usuários..." << endl;
    auto start = high_resolution_clock::now();

    // Se poucos usuários, não vale a pena paralelizar
    if (users.size() < 10000)
    {
        // Versão original
        for (auto &[userId, user] : users)
        {
            unordered_map<int, float> genreScores;

            for (const auto &[movieId, rating] : user.ratings)
            {
                if (rating >= Config::MIN_RATING && movies.find(movieId) != movies.end())
                {
                    for (const auto &genre : movies[movieId].genres)
                    {
                        genreScores[genreToId[genre]] += rating - Config::MIN_RATING;
                    }
                }
            }

            // Top 5 gêneros
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
        // VERSÃO PARALELA para muitos usuários

        // Coleta ponteiros para usuários (para divisão eficiente)
        vector<pair<uint32_t, UserProfile *>> userPtrs;
        userPtrs.reserve(users.size());
        for (auto &[userId, profile] : users)
        {
            userPtrs.push_back({userId, &profile});
        }

        // Determina número de threads
        const int num_threads = min(Config::NUM_THREADS, max(1, (int)users.size() / 5000));
        cout << "Usando " << num_threads << " threads para calcular preferências..." << endl;

        vector<thread> threads;
        threads.reserve(num_threads);

        // Contador atômico para progresso
        atomic<int> processed(0);

        size_t chunk_size = userPtrs.size() / num_threads;

        for (int t = 0; t < num_threads; t++)
        {
            size_t start_idx = t * chunk_size;
            size_t end_idx = (t == num_threads - 1) ? userPtrs.size() : (t + 1) * chunk_size;

            threads.emplace_back([&, start_idx, end_idx]()
                                 {
                for (size_t i = start_idx; i < end_idx; i++) {
                    auto& user = *userPtrs[i].second;
                    unordered_map<int, float> genreScores;
                    
                    // Calcula scores dos gêneros
                    for (const auto &[movieId, rating] : user.ratings) {
                        if (rating >= Config::MIN_RATING) {
                            auto movieIt = movies.find(movieId);
                            if (movieIt != movies.end()) {
                                // Usa bitmask para eficiência
                                uint32_t movieGenres = movieIt->second.genreBitmask;
                                
                                // Processa cada bit setado
                                for (int g = 0; g < 32 && movieGenres; g++) {
                                    if (movieGenres & (1 << g)) {
                                        genreScores[g] += rating - Config::MIN_RATING;
                                        movieGenres &= ~(1 << g); // Clear bit processado
                                    }
                                }
                            }
                        }
                    }
                    
                    // Encontra top 5 gêneros
                    if (!genreScores.empty()) {
                        vector<pair<float, int>> sortedGenres;
                        sortedGenres.reserve(genreScores.size());
                        
                        for (const auto& [genreId, score] : genreScores) {
                            sortedGenres.push_back({score, genreId});
                        }
                        
                        // partial_sort é mais eficiente para pegar só top 5
                        int topN = min(5, (int)sortedGenres.size());
                        partial_sort(sortedGenres.begin(), 
                                   sortedGenres.begin() + topN,
                                   sortedGenres.end(),
                                   greater<pair<float, int>>());
                        
                        user.preferredGenres = 0;
                        for (int j = 0; j < topN; j++) {
                            user.preferredGenres |= (1 << sortedGenres[j].second);
                        }
                    }
                    
                    // Atualiza progresso
                    int count = ++processed;
                    if (count % 50000 == 0) {
                        cout << "Processadas preferências de " << count << " usuários..." << endl;
                    }
                } });
        }

        // Aguarda todas as threads
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