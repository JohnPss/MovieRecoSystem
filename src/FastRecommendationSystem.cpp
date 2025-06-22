#include "FastRecommendationSystem.hpp"
#include "Config.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace std;
using namespace chrono;

FastRecommendationSystem::FastRecommendationSystem() : globalAvgRating(0.0f)
{
    dataLoader = new DataLoader(
        users, movies, genreToId, movieToUsers, genreToMovies,
        globalAvgRating, movieAvgRatings, moviePopularity);

    similarityCalculator = new SimilarityCalculator(users);

    recommendationEngine = new RecommendationEngine(
        users, movies, movieToUsers, genreToMovies,
        movieAvgRatings, moviePopularity, globalAvgRating,
        *similarityCalculator);
}

FastRecommendationSystem::~FastRecommendationSystem()
{
    delete dataLoader;
    delete similarityCalculator;
    delete recommendationEngine;
}

void FastRecommendationSystem::loadData()
{
    dataLoader->loadRatings(Config::RATINGS_FILE);
    dataLoader->loadMovies(Config::MOVIES_FILE);
}

void FastRecommendationSystem::processRecommendations(const string &filename)
{
    vector<uint32_t> userIds = dataLoader->loadUsersToRecommend(filename);

    cout << "\nGerando recomendações para " << userIds.size() << " usuários..." << endl;
    auto totalStart = high_resolution_clock::now();

    for (uint32_t userId : userIds)
    {
        auto start = high_resolution_clock::now();

        vector<Recommendation> recommendations = recommendForUser(userId);

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        cout << "\nUsuário " << userId << " (" << duration.count() << "ms):" << endl;
        printRecommendations(userId, recommendations);
    }

    auto totalEnd = high_resolution_clock::now();
    auto totalDuration = duration_cast<milliseconds>(totalEnd - totalStart);
    cout << "\nTempo de recomendações: " << totalDuration.count() << "ms" << endl;
    cout << "Tempo médio por usuário: " << totalDuration.count() / userIds.size() << "ms" << endl;
}

vector<Recommendation> FastRecommendationSystem::recommendForUser(uint32_t userId)
{
    return recommendationEngine->recommendForUser(userId);
}

void FastRecommendationSystem::printRecommendations(
    uint32_t userId,
    const vector<Recommendation> &recommendations)
{
    int count = 0;
    for (const auto &rec : recommendations)
    {
        auto movieIt = movies.find(rec.movieId);
        if (movieIt != movies.end() && count < 10)
        {
            cout << "  " << (count + 1) << ". " << movieIt->second.title
                 << " (Score: " << fixed << setprecision(2)
                 << rec.score << ")" << endl;
            count++;
        }
    }
}