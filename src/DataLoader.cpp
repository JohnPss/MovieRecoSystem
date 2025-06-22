#include "DataLoader.hpp"
#include "Config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

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

    string line;
    int totalRatings = 0;
    globalAvgRating = 0.0f;

    while (getline(file, line))
    {
        istringstream iss(line);
        uint32_t userId;
        iss >> userId;

        UserProfile &user = users[userId];
        user.ratings.clear();

        string ratingPair;
        float sumRatings = 0.0f;

        while (iss >> ratingPair)
        {
            size_t colonPos = ratingPair.find(':');
            if (colonPos != string::npos)
            {
                uint32_t movieId = stoul(ratingPair.substr(0, colonPos));
                float rating = stof(ratingPair.substr(colonPos + 1));

                user.ratings.push_back({movieId, rating});
                movieToUsers[movieId].push_back({userId, rating});

                sumRatings += rating;
                globalAvgRating += rating;
                movieAvgRatings[movieId] += rating;
                moviePopularity[movieId]++;
                totalRatings++;
            }
        }

        user.avgRating = user.ratings.empty() ? 0.0f : sumRatings / user.ratings.size();

        // Ordena ratings do usuário para busca binária
        sort(user.ratings.begin(), user.ratings.end());
    }

    globalAvgRating /= totalRatings;

    // Calcula médias dos filmes
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

        if (title.front() == '"')
            title = title.substr(1, title.length() - 2);

        uint32_t movieId = stoul(movieIdStr);
        Movie &movie = movies[movieId];
        movie.title = title;
        movie.genreBitmask = 0;

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

    // Calcula gêneros preferidos dos usuários
    calculateUserPreferences();

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Filmes carregados em " << duration.count() << "ms" << endl;
}

void DataLoader::calculateUserPreferences()
{
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