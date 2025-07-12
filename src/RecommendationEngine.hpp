#ifndef RECOMMENDATION_ENGINE_H
#define RECOMMENDATION_ENGINE_H

#include "Config.hpp"
#include "DataStructures.hpp"
#include "SimilarityCalculator.hpp"
#include "LSHIndex.hpp"

using namespace std;

class RecommendationEngine
{
private:
    const std::unordered_map<uint32_t, UserProfile> &users;
    const std::unordered_map<uint32_t, Movie> &movies;
    const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &movieToUsers;
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &genreToMovies;
    const std::unordered_map<uint32_t, float> &movieAvgRatings;
    const std::unordered_map<uint32_t, int> &moviePopularity;
    float globalAvgRating;

    SimilarityCalculator &similarityCalc;
    LSHIndex &lshIndex;

public:
    RecommendationEngine(
        const std::unordered_map<uint32_t, UserProfile> &u,
        const std::unordered_map<uint32_t, Movie> &m,
        const std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, float>>> &mtu,
        const std::unordered_map<uint32_t, std::vector<uint32_t>> &gtm,
        const std::unordered_map<uint32_t, float> &mar,
        const std::unordered_map<uint32_t, int> &mp,
        float gar,
        SimilarityCalculator &sc,
        LSHIndex &lshIndex);

    std::vector<Recommendation> recommendForUser(uint32_t userId);

private:
    std::vector<std::pair<uint32_t, int>> findCandidateUsers(
        uint32_t userId,
        const UserProfile &user);

    std::vector<std::pair<uint32_t, float>> calculateSimilarities(
        uint32_t userId,
        const std::vector<std::pair<uint32_t, int>> &candidates);

    std::unordered_map<uint32_t, float> collaborativeFiltering(
        const UserProfile &user,
        const std::vector<std::pair<uint32_t, float>> &similarUsers,
        const std::unordered_set<uint32_t> &watchedMovies);

    void contentBasedBoost(
        const UserProfile &user,
        const std::unordered_set<uint32_t> &watchedMovies,
        std::unordered_map<uint32_t, float> &scores);

    void popularityFallback(
        const std::unordered_set<uint32_t> &watchedMovies,
        std::unordered_map<uint32_t, float> &scores);

    std::vector<std::pair<uint32_t, int>> findCandidateUsersLSH(
        uint32_t userId,
        const UserProfile &user);
};

#endif 